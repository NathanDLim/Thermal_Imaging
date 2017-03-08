/* 
 *  This is the code for the Jeenode on the sensor side. This Jeenode has multiple modes each of which run the motors and IR sensor differently.
 *  Manual Mode reads input from the joystick potentiometers and moves the motors accordingly. It can take an instantaneous temperature reading at the chosen position and send it over RF
 *  Automatic Mode will create an image using a 2D array of readings and send the data over RF
 *  
 *  TEAM CHARLIE
*/

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>

//**********************************************************************************************

#define PAN_DEFAULT 50 //These are the angles that the pan and tilt motors will default to
#define TILT_DEFAULT 80

#define PAN_RES 30 //These are the number of points taken in automatic mode, and the 2D array size.
#define TILT_RES 30

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

#define ROW_REQUEST_CODE    0xFF 
#define INIT_RESPONSE_CODE   0xCC

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

float RowReadings[TILT_RES] ;



void setup() {

  rf12_initialize(NODE, RF12_915MHZ, GROUP); // initialize RF

  /*DEBUGGING ONLY, CALLING ONCE*/
  generate_image() ;
  Serial.begin(9600);

}

void loop() {
  
}

/*Send a request to check if the client could issue a row request*/
int sendRowRequest_init(){
  //if ( !rf12_canSend() )
    //return -1 ;
  //rf12_recvDone(); // wait for any receiving to finish
  
  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 

  rf12_sendStart( 0, REQUEST_INIT_CODE, 1);    /*Send a row of readings data*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

/*Call this function after SendRowRequest_init() . */
int CheckRowResponse(){
  /*Check for a received packet*/
  // if ( !rf12_recvDone() )
  //   return -1 ;

  /*Wait until receiving is complete*/
  while ( !( rf12_recvDone() ) ) ;

  /*Check for valid length of the received packet*/
  if ( rf12_len != 1 )
    return -1 ;

  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -1 ;

  /*Check if response is as expc*/
  if ( *( (uint8_t*) rf12_data ) == INIT_RESPONSE_CODE )
    return 0 ;
  else
    return -1 ;

  return -1 ;
}

/* Send a Row Request, telling the row number that is required*/
int sendRowRequest(uint8_t RowNumber){
  // if ( !rf12_canSend() )
  //   return -1 ;
  // rf12_recvDone(); // wait for any receiving to finish

  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 

  rf12_sendStart( 0, RowNumber, 1);    /*Send a row of readings data*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

/*Store the received Row data*/
int rcvRow(){

  /*Check for a received packet*/
  if ( !rf12_recvDone() )
    return -1 ;

  /*Check for valid length of the received packet*/
  if ( !(rf12_len == TILT_RES*sizeof(float)) )
    return -1 ;

  /*Check for a valid CRC.
  --It is a check for data integrity using some mathematical algorithms*/
  if ( !(rf12_crc == 0) )
    return -1 ;

  /*Save the received data into the row Array*/
  memcpy(RowReadings, (float*) rf12_data, TILT_RES*sizeof(float) ) ;

  return 0 ;
}


/*Call this to generate the thermal image onto the target display*/
int generate_image(){

  int i ;
  if (PC_mode != AUTO_READY)
    return -1 ;

  for (i = 1 ; i <= 1 ; i++){
    if ( sendRowRequest_init() )
      return -1 ;
    delay (20) ;
    if ( CheckRowResponse() )
      return -1 ;
    delay(20) ;
    if ( sendRowRequest(i) )
      return -1 ;
    delay(20) ;
    if ( rcvRow() )
      return -1 ;
    delay(20) ;
    sendRow_COM(i) ;
    delay(20) ;
  }
}

/*DEBUGGING ONLY *** Output Row Readings onto the serial COM port */
void sendRow_COM(){
  for(int i=0;i < TILT_RES;i++){
    Serial.print(RowReadings[i]);
    Serial.print(" ");
  }
  Serial.println();

}














