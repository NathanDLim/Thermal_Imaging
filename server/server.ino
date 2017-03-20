/* 
 *  This is the code for the Jeenode on the sensor side. This Jeenode has multiple modes each of which run the motors and IR sensor differently.
 *  Manual Mode reads input from the joystick potentiometers and moves the motors accordingly. It can take an instantaneous temperature reading at the chosen position and send it over RF
 *  Automatic Mode will create an image using a 2D array of readings and send the data over RF
 *  
 *  TEAM CHARLIE
*/
#define RF69_COMPAT 1
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <JeeLib.h>

//**********************************************************************************************

#define PAN_DEFAULT       50 //These are the angles that the pan and tilt motors will default to
#define TILT_DEFAULT      80

#define PAN_RES           30 //These are the number of points taken in automatic mode, and the 2D array size.
#define TILT_RES          30

#define JOY_PAN_PIN       A1 //=Pin 15. These are the analog pin numbers for the joystick potentiometers. 
#define JOY_TILT_PIN      A0 //=Pin 14
#define JOY_BUTTON_PIN     7 // digital pin for the joystick button
#define PAN_PWM_PIN        4 /* Shamoon says, "Theres no PWM on PIN 4 !!! " */
#define TILT_PWM_PIN       5
#define AUTO_MODE_PIN      17
#define MANUAL_MODE_PIN    7  

#define MANUAL_SPEED       4 //sets how fast the joystick changes the servos angle

// #define AUTO_MODE_PIN      8 //digital pins for displaying the current mode of operation
// #define MANUAL_MODE_PIN    9  /*MIGHT NOT WORK FOR JEENODE, REVISE THESE TWO*/




#define OTHER_MODE_PIN    10 //to be renamed if we get to it

#define MODE_BUTTON_PIN   11 // controls the switching of modes

#define NODE               2 
#define GROUP            212 

#define REQUEST_INIT_CODE     0xFF 
#define INIT_RESPONSE_CODE    0xCC

#define MultiplyFactor      10.000        /*Increases the decimal of the
                                          temperature float value. This allows sending
                                          the temperature values as int while keeping decimal places
                                          e.g. 45.93 degrees celcius sent as 4593 if factor is 10.00*/ 

#define PRE_ROWSEND_DELAY      500        /*Waiting time in ms, before calling the SendRow() functions*/
#define PRE_RRESPONSE_DELAY    1800       /*Waiting time in ms, before calling the sendRowResponse() function*/                               

//************************************************************************************************

enum mode_t{ //This is the enum for keeping track of the current mode
  AUTO,
  MANUAL
};

Servo panServo;  // create servo objects to control the servos
Servo tiltServo;  

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // create the temp sensor object

//byte readings[PAN_RES][TILT_RES]; //The array of readings for the automatic picture mode
//float RowReadings[TILT_RES] ;
int RowReadings[TILT_RES] ;

volatile int panPos;
volatile int tiltPos;

mode_t mode;
bool changeMode; // boolean for when the mode should be changed

void setup() {
  // panServo.attach(4);  // attaches the servo on pin 4 to the servo object
  // tiltServo.attach(5);
  panServo.attach(PAN_PWM_PIN);  // attaches the servo on pin 4 to the servo object
  tiltServo.attach(TILT_PWM_PIN);
  mlx.begin();  //start the mlx IR sensor using the I2C pins

  //Set the pin modes
  pinMode(JOY_BUTTON_PIN, INPUT);
  pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(AUTO_MODE_PIN, OUTPUT);
  pinMode(MANUAL_MODE_PIN, OUTPUT);

  panPos = PAN_DEFAULT;
  tiltPos = TILT_DEFAULT;
  mode = mode_t::AUTO;

  rf12_initialize(NODE, RF12_915MHZ, GROUP); // initialize RF module

  
  Serial.begin(9600);
}

void loop() {
  
  //runs the loop corresponding to the current mode
  switch(mode){
    case AUTO: 
      autoLoop();
      break;
    case MANUAL:
      manualLoop();
      break;
    default:
      break;
  }


  //check if the user wants to change the mode
//  if(digitalRead(MODE_BUTTON_PIN))
//    updateMode();
    
  delay(15);

}

/*
 * This function checks if the button to change modes was pressed and updates the LEDs
 * The mode cycle is as follows: AUTO -> MANUAL -> AUTO
 */
void updateMode(){
  //check if the mode button was pressed, and update the mode as well as the corresponding LED
  switch(mode){
      case AUTO:
        mode = mode_t::MANUAL;
        digitalWrite(AUTO_MODE_PIN,0);
        digitalWrite(MANUAL_MODE_PIN,1);
        digitalWrite(OTHER_MODE_PIN,0);
        break;
      case MANUAL:
        if(changeMode)
          mode = mode_t::AUTO;
        digitalWrite(AUTO_MODE_PIN,1);
        digitalWrite(MANUAL_MODE_PIN,0);
        digitalWrite(OTHER_MODE_PIN,0);
        break;
      default:
        mode = mode_t::AUTO;
    }
}

/*
 * This function controls all that happens in manual mode
 * 
 * It samples the potentiometers and sets the motors to the corresponding angle
 */
void manualLoop(){
  //read the joystick potentiometers and map the analog readings (0-1023) to the servo angles (1-179). We exclude angle 0 and 180 because they have some artifacts.
  int p = analogRead(JOY_PAN_PIN)*178/1023 + 1;
  int t = analogRead(JOY_TILT_PIN)*178/1023 + 1;

  p = p>179 || p < 1? 90:p;
  t = t>179 || t < 1? 90:t;
  
  panServo.write(p);
  tiltServo.write(t);
  delay(15); //wait for servos to respond
}

/*
 * This function controls all that happens in automatic mode
 */
void autoLoop(){
  // if (Serial.available() > 0){
  //   if(Serial.read() == '!'){
  //     makePicture();
  //     //sendPicture();
  //   }
  // }
  Serial.println("AUTO LOOP Start");
  auto_service() ;
}

/*
 * This function will send the 2D array over RF
 * It currently prints through the Serial port.
 */
//void sendPicture(){
//  for(int i = 0; i < PAN_RES; i++){
//    for (int k = 0; k < TILT_RES; k++){
//      Serial.print(readings[i][k]);
//      Serial.print(" ");
//    }
//    Serial.println();
//  }  
//}

/*
 * This function iterates through the array of points specified by the servo defaults and resolutions (ie. PAN_DEFAULT, PAN_RES)
 */
//void makePicture(){
//  
//  for (panPos = PAN_DEFAULT - PAN_RES/2; panPos < PAN_DEFAULT + PAN_RES/2; ++panPos) { // PAN loop. Goes around the default angle, res/2 below and res/2 above.
//    // in steps of 1 degree
//    panServo.write(panPos);              // tell servo to go to position in variable 'pos'
//      for (tiltPos = TILT_DEFAULT - TILT_RES/2; tiltPos < TILT_DEFAULT + TILT_RES/2; ++tiltPos) { // TILT loop. Goes around the default angle, res/2 below and res/2 above.
//        tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
//        delay(15);                       // waits 15ms for the servo to reach the position
//       
//        readings[panPos-75][tiltPos-75] = byte(mlx.readObjectTempC());
//
////        Serial.print(int(tiltPos-75));
////        Serial.print(":");
////        Serial.print(String(readings[panPos-75][tiltPos-75]));
////        Serial.print(" ");
//      }
////      Serial.println(int(panPos-75));
////      Serial.print("\n");
//  }
//}

/*Waits for Row Request and sends row values if request is received. */
int auto_service(){
  // static int count ;

  // count ++ ; 

    if ( CheckRowRequest_init() == 0 ){
    // delay(1800) ;
    delay(PRE_RRESPONSE_DELAY) ;

    if ( sendRowResponse() ){
      Serial.println("ERROR 1");
      return -1 ;
    }
    delay(20) ;

    if ( rcv_RowRequest() ){
      Serial.println("ERROR 2");
      return -1 ;
    }
    delay(20) ;

    getRow() ;
    // delay(500) ;
    delay(PRE_ROWSEND_DELAY) ;


    if ( sendRow() ){
      Serial.println("ERROR 3");
      return -1 ;
    }
    delay(20) ;
    return 0 ;
  }

  return -1 ;
}

int CheckRowRequest_init(){
  /*Check for a received packet*/
  // if ( !rf12_recvDone() )
  //   return -1 ;

  /*Wait until receiving is complete*/
   Serial.println("Waiting for Transmission");
  while ( !( rf12_recvDone() ) );
  Serial.println("RECEIVE DONE");
  /*Check for valid length of the received packet*/
  if ( rf12_len != 1 )
    return -1 ;

  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -1 ;

  /*Check if request is as expc*/
  if ( *( (uint8_t*) rf12_data ) == REQUEST_INIT_CODE )
    return 0 ;
  else
    return -1 ;

  return -1 ;
}

int sendRowResponse(){
  uint8_t code ;
  code = INIT_RESPONSE_CODE ;
  //if ( !rf12_canSend() )
    //return -1 ;
  //rf12_recvDone(); // wait for any receiving to finish
  
  while(!rf12_canSend()) rf12_recvDone(); // wait for any receiving to finish 
  Serial.println("sendRowResponse line A .......");     /*DEBUGGING ONLY !! */
  rf12_sendStart( 0, &code, sizeof (uint8_t));    /*Send a row of readings data*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  return 0 ;
}

/*Receive a row request, and process it to update the Pan Position*/
int rcv_RowRequest(){
  uint8_t rowN ;

  panPos = PAN_DEFAULT - PAN_RES/2 ;

  /*Wait until receiving is complete*/
  Serial.println("rcv_RowRequest waiting");
  while ( !( rf12_recvDone() ) ) ;
  Serial.println("rcv_RowRequest received");

  /*Check for valid length of the received packet*/
  if ( rf12_len != 1 )
    return -1 ;

  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -1 ;

  rowN = *( (uint8_t*) rf12_data ) ;
  Serial.print("Received row: ");
  Serial.println(rowN);
  panPos += rowN - 1;
  panServo.write(panPos);              // tell servo to go to position in variable 'pos'
  return 0 ;
}


/*Get values for one row, gets called upon request by client*/
void getRow(){
  int i ;
  float tempFloat ;

  /*Flush any previous values in the the Row array*/
  //for (i = 0 ; i < TILT_RES ; i++)
    //RowReadings = 0.0 ;
  int count = 0;
  for (tiltPos = TILT_DEFAULT - TILT_RES/2; tiltPos < TILT_DEFAULT + TILT_RES/2; ++tiltPos) { // TILT loop. Goes around the default angle, res/2 below and res/2 above.
    tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position

//    Serial.println("Reading temp");
    tempFloat = mlx.readObjectTempC() ;
//    Serial.print("Temp = ");
//    Serial.println(tempFloat);
    tempFloat *= MultiplyFactor ;         
    RowReadings[count ++] = int(tempFloat) ;
    
  }

}


/*-Sends a row of IR sensor data to the client.
---Should be called after getRow() has finished execution. */
int sendRow(){
  Serial.println("Send row entered");
//  for (int i = 0; i< TILT_RES; i++){
//    Serial.print(" ");
//    Serial.print(RowReadings[i]);
//  }
//  int fake[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17 ,18,19,20,21,22,23,24,25,26,27,28,29};
  while(!rf12_canSend())
    rf12_recvDone(); // wait for any receiving to finish
  rf12_sendStart( 0, RowReadings, TILT_RES*sizeof(int) );    /*Send a row of readings data*/
//  rf12_sendStart( 0, fake, TILT_RES*sizeof(int) );
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  Serial.println("Exit send Row");
  return 0 ;
}

/*might not be needed*/
int canSendRow(){

}
