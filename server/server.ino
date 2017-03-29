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

#define JOY_PAN_PIN                   A1 //=Pin 15. These are the analog pin numbers for the joystick potentiometers. 
#define JOY_TILT_PIN                  A0 //=Pin 14
#define JOY_BUTTON_IRQ_PIN             3 // digital pin for the joystick button, used as an interrupt.
#define PAN_PWM_PIN                    6 /* Shamoon says, "Theres no PWM on PIN 4 !!! " */
#define TILT_PWM_PIN                   5
#define AUTO_READY_MODE_PIN_LED       17
#define AUTO_BUSY_MODE_PIN_LED         7
#define MANUAL_MODE_PIN_LED           16  

#define MANUAL_SPEED       4 //sets how fast the joystick changes the servos angle

// #define AUTO_MODE_PIN      8 //digital pins for displaying the current mode of operation
// #define MANUAL_MODE_PIN    9  /*MIGHT NOT WORK FOR JEENODE, REVISE THESE TWO*/

//#define OTHER_MODE_PIN    10 //to be renamed if we get to it

//#define MODE_BUTTON_PIN   11 // controls the switching of modes

#define NODE               2 
#define GROUP            212 

#define REQUEST_INIT_CODE     0xFF 
#define INIT_RESPONSE_CODE    0xCC
#define MANUAL_CODE           0xDD
#define CONTAINS_SINGLE_CODE  0xAF

#define MultiplyFactor      10.000        /*Increases the decimal of the
                                          temperature float value. This allows sending
                                          the temperature values as int while keeping decimal places
                                          e.g. 45.93 degrees celcius sent as 4593 if factor is 10.00*/ 

#define PRE_ROWSEND_DELAY      500        /*Waiting time in ms, before calling the SendRow() functions*/
#define PRE_RRESPONSE_DELAY    1800       /*Waiting time in ms, before calling the sendRowResponse() function*/
#define IRQ_DELAY              500000        /*in microseconds !!! */                      

//************************************************************************************************

enum mode_t{ //This is the enum for keeping track of the current mode
  AUTO_READY,
  AUTO_BUSY,
  MANUAL,
};

Servo panServo;  // create servo objects to control the servos
Servo tiltServo;  

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // create the temp sensor object

//byte readings[PAN_RES][TILT_RES]; //The array of readings for the automatic picture mode
//float RowReadings[TILT_RES] ;
int RowReadings[TILT_RES] ;
int SingleReading ;

volatile int panPos;
volatile int tiltPos;

volatile mode_t mode;
bool changeMode; // boolean for when the mode should be changed

void setup() {
  // panServo.attach(4);  // attaches the servo on pin 4 to the servo object
  // tiltServo.attach(5);
  panServo.attach(PAN_PWM_PIN);  // attaches the servo on pin 4 to the servo object
  tiltServo.attach(TILT_PWM_PIN);
  mlx.begin();  //start the mlx IR sensor using the I2C pins

  //Set the pin modes
  // pinMode(JOY_BUTTON_PIN, INPUT);
  // pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(AUTO_READY_MODE_PIN_LED, OUTPUT);
  pinMode(AUTO_BUSY_MODE_PIN_LED, OUTPUT);
  pinMode(MANUAL_MODE_PIN_LED, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(JOY_BUTTON_IRQ_PIN), joy_ISR, RISING); /*Confirm that the joystick button is falling edge*/

  panPos = PAN_DEFAULT;
  tiltPos = TILT_DEFAULT;
  //mode = mode_t::AUTO_READY;
  mode = MANUAL ;

  rf12_initialize(NODE, RF12_915MHZ, GROUP); // initialize RF module

  update_modeLED() ;
  Serial.begin(9600);
}

void loop() {
  
  //runs the loop corresponding to the current mode
  switch(mode){
    case AUTO_BUSY: /*unconditionally runs the code of the next case*/
    case AUTO_READY: 
      auto_service();
      break;
    case MANUAL:
      manual_service();
      break;
    default:
      break;
  }


  //check if the user wants to change the mode
//  if(digitalRead(MODE_BUTTON_PIN))
//    updateMode();
    
  //delay(15);

}

/*
 * This function checks if the button to change modes was pressed and updates the LEDs
 * The mode cycle is as follows: AUTO -> MANUAL -> AUTO
 */
// void updateMode(){
//   //check if the mode button was pressed, and update the mode as well as the corresponding LED
//   switch(mode){
//       case AUTO:
//         mode = mode_t::MANUAL;
//         digitalWrite(AUTO_MODE_PIN,0);
//         digitalWrite(MANUAL_MODE_PIN,1);
//         digitalWrite(OTHER_MODE_PIN,0);
//         break;
//       case MANUAL:
//         if(changeMode)
//           mode = mode_t::AUTO;
//         digitalWrite(AUTO_MODE_PIN,1);
//         digitalWrite(MANUAL_MODE_PIN,0);
//         digitalWrite(OTHER_MODE_PIN,0);
//         break;
//       default:
//         mode = mode_t::AUTO;
//     }
// }

/*
 * This function controls all that happens in manual mode
 * 
 * It samples the potentiometers and sets the motors to the corresponding angle
 */
// void manualLoop(){
//   //read the joystick potentiometers and map the analog readings (0-1023) to the servo angles (1-179). We exclude angle 0 and 180 because they have some artifacts.
//   // int p = analogRead(JOY_PAN_PIN)*178/1023 + 1;
//   // int t = analogRead(JOY_TILT_PIN)*178/1023 + 1;

//   // p = p>179 || p < 1? 90:p;
//   // t = t>179 || t < 1? 90:t;
  
//   // panServo.write(p);
//   // tiltServo.write(t);
//   // delay(15); //wait for servos to respond

//   Serial.println("MANUAL service start LOOP Start");
//   manual_service() ;

// }

/*
 * This function controls all that happens in automatic mode
 */
// void autoLoop(){
//   // if (Serial.available() > 0){
//   //   if(Serial.read() == '!'){
//   //     makePicture();
//   //     //sendPicture();
//   //   }
//   // }
//   Serial.println("AUTO Service Start");
//   auto_service() ;
// }

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
  //Serial.println("AUTO Service Start..");

  // static int count ;

  // count ++ ;
  mode_t prev_mode = mode ;
    
  //if (mode == AUTO_READY) mode = AUTO_BUSY ;  /*Put mode to busy*/
  //update_modeLED();

  if ( CheckRowRequest_init() == 0 ){
    // delay(1800) ;
    if (mode == AUTO_READY) mode = AUTO_BUSY ;  /*Put mode to busy*/
    update_modeLED();
    
    delay(PRE_RRESPONSE_DELAY) ;

    if (prev_mode == AUTO_READY){
      if ( sendRowResponse(INIT_RESPONSE_CODE) ){
        Serial.println("ERROR 1");
        if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
        update_modeLED();
        return -1 ;
      }
      delay(20) ;

    } else {   /*Deny Initialization request*/
        if ( sendRowResponse(0) ){
          Serial.println("ERROR 63");
          if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
          update_modeLED();
          return -1 ;
        }
        Serial.println("Initialization request denied, not in AUTO_READY mode");
        delay(20) ;
        if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
        update_modeLED();
        return -1 ;
    }
  
    if ( rcv_RowRequest() ){
      Serial.println("ERROR 2");
      if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
      update_modeLED();
      return -1 ;
    }
    delay(20) ;

    getRow() ;
    // delay(500) ;
    delay(PRE_ROWSEND_DELAY) ;

    if ( sendRow() ){
      Serial.println("ERROR 3");
      if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
      return -1 ;
    }
    delay(20) ;
    if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
    update_modeLED() ;
    return 0 ;

  } else if ( CheckRowRequest_init() == -2 ) {  /*Unexpected Request Received*/
    delay(PRE_RRESPONSE_DELAY) ;
    sendRowResponse(0) ;
    Serial.println("Initialization denied, 88");
    if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
    update_modeLED() ;
    return -1;
  }

  if (mode == AUTO_BUSY) mode = AUTO_READY ;  /*Put mode back to ready*/
  return -1 ;
}

/* DESIGNED AS NON BLOCKING RECEIVE */
int CheckRowRequest_init(){
  /*Check for a received packet*/
  // if ( !rf12_recvDone() )
  //   return -1 ;

  /*Wait until receiving is complete*/
  // Serial.println("Waiting for Transmission");
  //while ( !( rf12_recvDone() ) );  /*--BLOCKING RECEIVE--*/

  if ( !( rf12_recvDone() ) ) return -1 ;   /*Implements Non-Blocking Receive*/
   
  Serial.println("RECEIVE DONE");
  /*Check for valid length of the received packet*/
  if ( rf12_len != 1 )
    return -2 ;

  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 )
    return -2 ;

  /*Check if request is as expc*/
  if ( *( (uint8_t*) rf12_data ) == REQUEST_INIT_CODE )
    return 0 ;
  else
    return -2 ;

  return -1 ;
}

int sendRowResponse(uint8_t code){
  //uint8_t code ;
  //code = INIT_RESPONSE_CODE ;
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


int manual_service(){
  int rv ;  /*Return value*/

  if (mode != MANUAL){
    Serial.println("Error! manual_service() called in a wrong mode");
    return -1 ;
  }

  /*read the joystick potentiometers and map the analog readings (0-1023) to the servo angles (1-179). We exclude angle 0 and 180 because they have some artifacts.*/
  int p = analogRead(JOY_PAN_PIN)*178/1023 + 1;
  int t = analogRead(JOY_TILT_PIN)*178/1023 + 1;

  p = p>179 || p < 1? 90:p;
  t = t>179 || t < 1? 90:t;
  
  panServo.write(p);
  tiltServo.write(t);
  delay(15); //wait for servos to respond

  rv = rcv_singleRequest() ;

  if ( rv == 0 ) {
    delay (PRE_RRESPONSE_DELAY);
    get_singleReading() ;
    Serial.println("Sending good response for single reading ..");
    send_singleResponse (CONTAINS_SINGLE_CODE) ;
  } else if ( rv == -2 )  {
    delay (PRE_RRESPONSE_DELAY);
    Serial.println("Sending Denial response for single reading ..");
    send_singleResponse (0x99) ;
  } else {
    //Serial.println("returning from manual service.. with no work done");
    return -1 ;
  }

  return rv ;
}

/* DESIGENED AS NON_BLOCKING RECEIVE */
int rcv_singleRequest(){

  //Serial.println("Waiting for Reception...rcvSingleRequest()");
  // while ( !( rf12_recvDone() ) );
  if ( !( rf12_recvDone() ) ) return -1 ;  /*Implements Non-Blocking Receive*/

  Serial.println("RECEIVE DONE...rcvSingleRequest() ");
  /*Check for valid length of the received packet*/
  if ( rf12_len != 1 )
    return -2 ;

  //Check for a valid CRC.
  //--It is a check for data integrity using some mathematical algorithms
  if ( rf12_crc != 0 ){
    Serial.println("BAD CRC !!! in rcv_singleRequest()");
    return -2 ;
  }
    

  /*Check if request is as expc*/
  if ( *( (uint8_t*) rf12_data ) == MANUAL_CODE )
    return 0 ;
  else
    return -2 ;

  return -1 ;
}

void get_singleReading(){
  float tempFloat ;

  tempFloat = mlx.readObjectTempC() ;
  tempFloat *= MultiplyFactor ;         
  SingleReading = int(tempFloat) ;

}

int send_singleResponse(uint8_t hdr_code){

  Serial.println("Send row entered");

  while(!rf12_canSend())
    rf12_recvDone(); // wait for any receiving to finish
  rf12_sendStart( hdr_code, &SingleReading, sizeof(int) );    /*Send a single temperature value*/
  rf12_sendWait ( 0 ) ; /*Wait for the send to finish, 0=NORMAL Mode*/
  Serial.println("Returning from send_singleResponse()");
  return 0 ;
}

void update_modeLED(){
  switch (mode){
    case AUTO_READY:
      digitalWrite(AUTO_READY_MODE_PIN_LED, HIGH) ;
      digitalWrite(AUTO_BUSY_MODE_PIN_LED, LOW) ;
      digitalWrite(MANUAL_MODE_PIN_LED, LOW) ;
      break ;
    case AUTO_BUSY:
      digitalWrite(AUTO_READY_MODE_PIN_LED, LOW) ;
      digitalWrite(AUTO_BUSY_MODE_PIN_LED, HIGH) ;
      digitalWrite(MANUAL_MODE_PIN_LED, LOW) ;
      break ;
    case MANUAL:
      digitalWrite(AUTO_READY_MODE_PIN_LED, LOW) ;
      digitalWrite(AUTO_BUSY_MODE_PIN_LED, LOW) ;
      digitalWrite(MANUAL_MODE_PIN_LED, HIGH) ;
      break ;
    default:
      digitalWrite(AUTO_READY_MODE_PIN_LED, HIGH) ;
      digitalWrite(AUTO_BUSY_MODE_PIN_LED, HIGH) ;
      digitalWrite(MANUAL_MODE_PIN_LED, HIGH) ;
      break ;
  }
}


/*Interrupt Service Routine triggered by the Joystick button*/
void joy_ISR(){
  noInterrupts();
  switch (mode){
    case AUTO_READY:
      mode = MANUAL ;
      Serial.println("switched to manual");
      break ;
    case AUTO_BUSY:
      Serial.println("currently in auto busy");
      interrupts();
      return ; /*Busy: Do not Disturb*/
      break ;
    case MANUAL:
      mode = AUTO_READY ;
      Serial.println("switched to auto ready");
      break ;
    default:
      
      break ;
  }
  update_modeLED() ;
  //delay (IRQ_DELAY) ; /*Usual delay function dont work inside ISRs*/
  delayMicroseconds(IRQ_DELAY) ;
  interrupts(); /*Re-ENABLE INTERRUPTS*/
}

