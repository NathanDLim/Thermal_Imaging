/* 
 *  This is the code for the Jeenode on the sensor side. This Jeenode has multiple modes each of which run the motors and IR sensor differently.
 *  Manual Mode reads input from the joystick potentiometers and moves the motors accordingly. It can take an instantaneous temperature reading at the chosen position and send it over RF
 *  Automatic Mode will create an image using a 2D array of readings and send the data over RF
 *  
 *  TEAM CHARLIE - 3C
 *  STANDALONE CODE -Send IR data from sensor and send directly to a computer via COM port
 *  Can be used as an alternative to RF communication.
*/

#include <Servo.h>
#include <Wire.h>
#define RF69_COMPAT 1
#include <JeeLib.h>
#include <Adafruit_MLX90614.h>


//**********************************************************************************************

#define PAN_DEFAULT 50 //These are the angles that the pan and tilt motors will default to
#define TILT_DEFAULT 80

#define PAN_RES 40 //These are the number of points taken in automatic mode, and the 2D array size.
#define TILT_RES 40

#define ROW_PARTIAL 10 // the number of readings in one RF send. The resolution must be a multiple of this

#define MANUAL_SPEED 4 //sets how fast the joystick changes the servos angle

#define JOY_PAN_PIN                   A1 //=Pin 15. These are the analog pin numbers for the joystick potentiometers. 
#define JOY_TILT_PIN                  A0 //=Pin 14
#define JOY_BUTTON_IRQ_PIN             3 // digital pin for the joystick button, used as an interrupt.
#define PAN_PWM_PIN                    6 /* Shamoon says, "Theres no PWM on PIN 4 !!! " */
#define TILT_PWM_PIN                   5
#define AUTO_READY_MODE_PIN_LED       17
#define AUTO_BUSY_MODE_PIN_LED         7
#define MANUAL_MODE_PIN_LED           16  

#define MANUAL_SPEED       4 //sets how fast the joystick changes the servos angle

#define PC_AUTO_INIT         0xAA
#define PC_MANUAL_INIT       0xBB  

#define MultiplyFactor      100.000        /*Increases the decimal of the
                                          temperature float value. This allows sending
                                          the temperature values as int while keeping decimal places
                                          e.g. 45.93 degrees celcius sent as 4593 if factor is 10.00*/ 

#define PRE_ROWSEND_DELAY      1000//500//100 //3000//500        /*Waiting time in ms, before calling the SendRow() functions*/
#define PRE_RRESPONSE_DELAY    3000  //1800       /*Waiting time in ms, before calling the sendRowResponse() function*/
#define IRQ_DELAY              50000      /*in microseconds !!! */
#define IRQ_DELAY_TIMES        4         /*No. of times IRQ_DELAY should be made*/    

#define DEBUG

//************************************************************************************************

enum mode_t{ //This is the enum for keeping track of the current mode
  AUTO_READY,
  AUTO_BUSY,
  MANUAL,
};

volatile mode_t mode;

// float RowReadings[TILT_RES] ;
int RowReadings[TILT_RES] ;
int SingleReading ;

Servo panServo;  // create servo objects to control the servos
Servo tiltServo;  

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // create the temp sensor object

volatile int panPos;
volatile int tiltPos;

void setup() {

  panServo.attach(PAN_PWM_PIN);  // attaches the servo on pin 4 to the servo object
  tiltServo.attach(TILT_PWM_PIN);

  panServo.write(PAN_DEFAULT);
  tiltServo.write(TILT_DEFAULT);
  
  mlx.begin();  //start the mlx IR sensor using the I2C pins

  pinMode(AUTO_READY_MODE_PIN_LED, OUTPUT);
  pinMode(AUTO_BUSY_MODE_PIN_LED, OUTPUT);
  pinMode(MANUAL_MODE_PIN_LED, OUTPUT);
  //attachInterrupt(digitalPinToInterrupt(JOY_BUTTON_IRQ_PIN), joy_ISR, RISING); /*Confirm that the joystick button is falling edge*/
  attachInterrupt(digitalPinToInterrupt(JOY_BUTTON_IRQ_PIN), joy_ISR, FALLING); /*Confirm that the joystick button is falling edge*/

  panPos = PAN_DEFAULT;
  tiltPos = TILT_DEFAULT;
  //mode = mode_t::AUTO_READY;
  mode = AUTO_READY ;

    update_modeLED() ;
  Serial.begin(9600);

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



/*Call this to generate the thermal image onto the target display*/
int generate_image(){
  int i ;

#ifdef DEBUG
  Serial.print("Starting generate_image () ...");
#endif
  
  if (mode != AUTO_READY){
    Serial.print("error: not in AUTO_READY mode");
    return -1 ;
  }

  mode = AUTO_BUSY ; 

  for (i = 0 ; i < TILT_RES ; i++){
    panPos = PAN_DEFAULT - PAN_RES/2 ;
    panPos += i - 1;
    panServo.write(panPos);              // tell servo to go to position in variable 'pos'
    delay (50) ;
    getRow() ;
    // delay (1000) ;
    delay (PRE_ROWSEND_DELAY) ;
    sendRow_COM(i) ;    
  }
    
    delay(20) ;
    mode = AUTO_READY ;

}

/*Get values for one row, gets called upon request by client*/
void getRow(){
  int i ;
  float tempFloat ;

  /*Flush any previous values in the the Row array*/
  //for (i = 0 ; i < TILT_RES ; i++)
    //RowReadings = 0.0 ;
  int count = 0;
  tiltServo.write(TILT_DEFAULT - TILT_RES/2);
  delay(100);
  for (tiltPos = TILT_DEFAULT - TILT_RES/2; tiltPos < TILT_DEFAULT + TILT_RES/2; ++tiltPos) { // TILT loop. Goes around the default angle, res/2 below and res/2 above.
    tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
    delay(50);                       // waits 15ms for the servo to reach the position

//    Serial.println("Reading temp");
    tempFloat = mlx.readObjectTempC() ;
//    Serial.print("Temp = ");
//    Serial.println(tempFloat);
    tempFloat *= MultiplyFactor ;         
    RowReadings[count ++] = int(tempFloat) ;
    
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

int generate_single(){

#ifdef DEBUG
  Serial.print("Starting generate_single ...");
#endif

  if (mode != MANUAL){
    Serial.print("error: not in MANUAL mode");
    return -1 ;
  }
  get_singleReading() ;
  sendSingle_COM() ;
  return 0 ;
}

void get_singleReading(){
  float tempFloat ;

  tempFloat = mlx.readObjectTempC() ;
  tempFloat *= MultiplyFactor ;         
  SingleReading = int(tempFloat) ;

}

void sendSingle_COM(){

  Serial.print("!");
  Serial.println(SingleReading);
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
  int i;
  noInterrupts();   /*Disable Interrupts*/
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
  for (i = 0 ; i < IRQ_DELAY_TIMES ; i++){
    delayMicroseconds(IRQ_DELAY) ;
  }

  interrupts(); /*Re-ENABLE INTERRUPTS*/
}

