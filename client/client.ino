/* 
 *  This is the code for the Jeenode on the sensor side. This Jeenode has multiple modes each of which run the motors and IR sensor differently.
 *  Manual Mode reads input from the joystick potentiometers and moves the motors accordingly. It can take an instantaneous temperature reading at the chosen position and send it over RF
 *  Automatic Mode will create an image using a 2D array of readings and send the data over RF
 *  
 *  TEAM CHARLIE - 3C
 *  CLIENT CODE --- SUPPOSED TO CONNECT THIS NODE WITH DISPLAY COMPUTER
*/

#include <Servo.h>
#include <Wire.h>
#define RF69_COMPAT 1
#include <JeeLib.h>

//**********************************************************************************************

#define PAN_DEFAULT 50 //These are the angles that the pan and tilt motors will default to
#define TILT_DEFAULT 80

#define PAN_RES 30 //These are the number of points taken in automatic mode, and the 2D array size.
#define TILT_RES 30

#define ROW_PARTIAL 10 // the number of readings in one RF send. The resolution must be a multiple of this

#define JOY_PAN_PIN A1 //These are the analog pin numbers for the joystick potentiometers. 
#define JOY_TILT_PIN A0
#define JOY_BUTTON_PIN 7 // digital pin for the joystick button

#define MANUAL_SPEED 4 //sets how fast the joystick changes the servos angle

#define AUTO_MODE_PIN 8 //digital pins for displaying the current mode of operation
#define MANUAL_MODE_PIN 9
#define OTHER_MODE_PIN 10 //to be renamed if we get to it

#define MODE_BUTTON_PIN 11 // controls the switching of modes

#define NODE    3
#define GROUP   212

/* Acknowledgment Codes */
#define PC_AUTO_INIT         0xAA
#define PC_MANUAL_INIT       0xBB 
#define REQUEST_INIT_CODE    0xFF 
#define INIT_RESPONSE_CODE   0xCC
#define MANUAL_CODE          0xDD
#define CONTAINS_SINGLE_CODE 0xA3

#define DEBUG

//************************************************************************************************

enum mode_t{ //This is the enum for keeping track of the current mode
  AUTO,
  MANUAL
};

enum PC_mode_t{
  AUTO_READY,       /*The PC is ready to accept data to create the thermal image*/
  AUTO_BUSY,        /*The PC is busy in creating the thermal image*/
  MANUAL_READY,     /*Same as above for manual mode*/
  MANUAL_BUSY       /*Same as above for manual mode*/
} ;

PC_mode_t PC_mode ;

// float RowReadings[TILT_RES] ;
int RowReadings[TILT_RES] ;
int SingleReading ;


void setup() {

  rf12_initialize(NODE, RF12_915MHZ, GROUP); // initialize RF

  /*DEBUGGING ONLY, CALLING ONCE*/
  Serial.begin(9600);
  
//  generate_image() ;

}

void loop() {
    if (! (Serial.available() >0) ) return ;

    int read_value = Serial.read() ;
    if( read_value == PC_AUTO_INIT ){
      generate_image() ;
    } else if ( read_value == PC_MANUAL_INIT ){
      // CODE HERE
      generate_single() ;
    }
  
}

/*Send a request to check if the client could issue a row request*/
int sendRowRequest_init(){
  uint8_t code ;
  code = REQUEST_INIT_CODE ;
  //if ( !rf12_canSend() )
    //return -1 ;
  //rf12_recvDone(); // wait for any receiving to finish
  
  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 
  
#ifdef DEBUG
  Serial.print("checking cansend() ....  ....");
#endif

//  if ( !(rf12_canSend()) )
//    return -1 ;

#ifdef DEBUG
  Serial.print("Starting to send init ....");
#endif

  rf12_sendStart( 0, &code, 1);    /*Send a row of readings data*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

/*Call this function after SendRowRequest_init() . */
int CheckRowResponse(){
  /*Check for a received packet*/
  // if ( !rf12_recvDone() )
  //   return -1 ;
  
#ifdef DEBUG
  Serial.println("CheckRowResponse  line A");
#endif

  /*Wait until receiving is complete*/
  long count=0;
  while ( !( rf12_recvDone() ) ) {
    if(count++ == 2000000){
      Serial.println("CheckRowResponse timeout");
      return -1;
    }
    //Serial.println("waiting for Row response ....");
   } ;
   
#ifdef DEBUG
  Serial.println("CheckRowResponse  line B");
#endif

  /*Check for valid length of the received packet*/
  if ( rf12_len != sizeof(uint8_t) ){
    Serial.print("received length = ");
    Serial.println(rf12_len);
    return -1 ;}

#ifdef DEBUG
  Serial.println("CheckRowResponse  line C");
#endif
  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -1 ;

#ifdef DEBUG
  Serial.println("CheckRowResponse  line D");
#endif

  /*Check if response is as expc*/
  if ( *( (uint8_t*) rf12_data ) == INIT_RESPONSE_CODE )
    return 0 ;
  else
    return -1 ;

#ifdef DEBUG
  Serial.println("CheckRowResponse  line E");
#endif

  return -1 ;
}

/* Send a Row Request, telling the row number that is required*/
int sendRowRequest(uint8_t RowNumber){
  // if ( !rf12_canSend() )
  //   return -1 ;
  // rf12_recvDone(); // wait for any receiving to finish

  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 

  rf12_sendStart( 0, &RowNumber, 1);    /*Send a row of readings data*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

/*Store the received Row data*/
int rcvRow(){

 //We split up the row into partial rows, and combine them after
  for(int i = 0; i<TILT_RES/ROW_PARTIAL;i++){
    
#ifdef DEBUG
  Serial.println("rcvRow  line A");
#endif

    /*Check for a received packet*/
  /*Wait until receiving is complete*/
  long count=0;
  while ( !( rf12_recvDone() ) ) {
    if(count++ == 2000000){
      Serial.println("rcv_Row timeout");
      return -1;
    }
    //Serial.println("waiting for Row response ....");
   } ;

#ifdef DEBUG
  Serial.println("rcvRow  line B");
#endif

    /*Check for valid length of the received packet*/
    if ( !(rf12_len == ROW_PARTIAL*sizeof(int)) ){
      Serial.print("Received data, length: ");
      Serial.println(rf12_len);
      return -1 ;
    }

#ifdef DEBUG
  Serial.println("rcvRow  line C");
#endif

    /*Check for a valid CRC.
    --It is a check for data integrity using some mathematical algorithms*/
    if ( !(rf12_crc == 0) ){
      Serial.println("Bad CRC");
      return -3 ;
    }

#ifdef DEBUG
  Serial.println("rcvRow  line D");
#endif

   for(int j =  0; j<10;j++)
    Serial.print(((int*) rf12_data)[j]);
    /*Save the received data into the row Array*/
    memcpy(RowReadings+ROW_PARTIAL*i, (int*) rf12_data, ROW_PARTIAL*sizeof(int) ) ;
  }

#ifdef DEBUG
  Serial.println("rcvRow  line E");
#endif

  return 0 ;
}


/*Call this to generate the thermal image onto the target display*/
int generate_image(){

#ifdef DEBUG
  Serial.print("Starting generate_image () ...");
#endif
  
  int i ;
  if (PC_mode != AUTO_READY){
    Serial.print("error 1");
    return -1 ;
  }

  int numTries = 0;

  for (i = 0 ; i < TILT_RES ; i++){
     numTries++;
     
     if(numTries == 5){ //only try to get the same row 5 times before cancelling the entire thing
      Serial.println("Cancelling generate image");
      break;
     }
      
#ifdef DEBUG
    Serial.print("iterating in generate_image  .... ");
#endif

    if ( sendRowRequest_init() ){
      Serial.print("error 2");
      i--;
      delay(1000);
      continue;
      //return -1 ;
    }
    delay (20) ;
    if ( CheckRowResponse() ){
      Serial.print("error 3");
      i--;
      delay(3000);
      continue;
      //return -1 ;
    }
    delay(20) ;
    if ( sendRowRequest(i) ){
      Serial.print("error 4");
      return -1 ;}
    delay(20) ;
    int ret = rcvRow();
    if (ret != 0){ //This is a bad CRC, we try to get that row again
      i--;
      delay(1000);
      continue;
    }
//    else if(ret != 0){
//      Serial.print("error 5");
//      return -1 ;}
    delay(20) ;
    sendRow_COM(i) ;
    delay(1000);
    delay(20) ;

    numTries = 0;
  }
}

/*DEBUGGING ONLY *** Output Row Readings onto the serial COM port */
void sendRow_COM(int rowNum){
    Serial.print("ROW");
    Serial.print(rowNum);
    Serial.print(":");
    
    Serial.print(RowReadings[0]);
  for(int i=1;i < TILT_RES;i++){
    Serial.print(",");
    Serial.print(RowReadings[i]);
  }
  Serial.println();

}

int generate_single()
{

#ifdef DEBUG
  Serial.print("Starting generate_single ...");
#endif

  if ( sendSingleRequest() ){
    Serial.print("error 22");
    return -1 ;}
  delay (20) ;
  if ( rcvSingleResponse() ){
    Serial.print("error 44");
    return -1 ;}
  delay(20) ;
  sendSingle_COM() ;
  delay(20) ;

  return 0 ;
}

int sendSingleRequest() {
  uint8_t code ;
  code = MANUAL_CODE ;
  //if ( !rf12_canSend() )
    //return -1 ;
  //rf12_recvDone(); // wait for any receiving to finish
  
  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 
  
#ifdef DEBUG
  Serial.print("checking cansend() ....  ....");
#endif

//  if ( !(rf12_canSend()) )
//    return -1 ;

#ifdef DEBUG
  Serial.print("Starting to send singleRequest ....");
#endif

  rf12_sendStart( 0, &code, 1);    /*Send a request for a single reading*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

int rcvSingleResponse() {


  #ifdef DEBUG
  Serial.println("rcvSingleResponse  line A");
#endif

  /*Wait until receiving is complete*/
  long count=0;
  while ( !( rf12_recvDone() ) ) {
    if(count++ == 2000000){
      Serial.println("rcv_Single timeout");
      return -1;
    }
    //Serial.println("waiting for Row response ....");
   } ;
   
#ifdef DEBUG
  Serial.println("rcvSingleResponse  line B");
#endif

  /*Check for valid length of the received packet*/
  if ( rf12_len != sizeof(int) ){
    Serial.print("received length = ");
    Serial.println(rf12_len);
    return -1 ;}

#ifdef DEBUG
  Serial.println("rcvSingleResponse  line C");
#endif
  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -1 ;

#ifdef DEBUG
  Serial.println("rcvSingleResponse  line D");
#endif



memcpy(&SingleReading, (int*) rf12_data, sizeof(int) ) ;
    return 0 ;

//  /*Check if response is as expc*/
//  if ( rf12_hdr == CONTAINS_SINGLE_CODE ){
//#ifdef DEBUG
//  Serial.println("rcvSingleResponse  line E");
//#endif
//    memcpy(&SingleReading, (int*) rf12_data, sizeof(int) ) ;
//    return 0 ;
//  }
//  else{
//#ifdef DEBUG
//  Serial.println("rcvSingleResponse  line F");
//#endif
//    return -1 ;
//  }


}

void sendSingle_COM(){

  Serial.print("!");
  Serial.println(SingleReading);
}
