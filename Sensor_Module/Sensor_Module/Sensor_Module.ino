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

//************************************************************************************************

enum mode_t{ //This is the enum for keeping track of the current mode
  AUTO,
  MANUAL
};

Servo panServo;  // create servo objects to control the servos
Servo tiltServo;  

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // create the temp sensor object

byte readings[PAN_RES][TILT_RES]; //The array of readings for the automatic picture mode

int panPos;
int tiltPos;

mode_t mode;
bool changeMode; // boolean for when the mode should be changed

void setup() {
  panServo.attach(4);  // attaches the servo on pin 9 to the servo object
  tiltServo.attach(5);
  mlx.begin();  //start the mlx IR sensor using the I2C pins

  //Set the pin modes
  pinMode(JOY_BUTTON_PIN, INPUT);
  pinMode(MODE_BUTTON_PIN, INPUT);
  pinMode(AUTO_MODE_PIN, OUTPUT);
  pinMode(MANUAL_MODE_PIN, OUTPUT);

  panPos = PAN_DEFAULT;
  tiltPos = TILT_DEFAULT;
  mode = mode_t::AUTO;
  
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
  if(digitalRead(MODE_BUTTON_PIN))
    updateMode();
    
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
  if (Serial.available() > 0){
    if(Serial.read() == '!'){
      makePicture();
      sendPicture();
    }
  }
}

/*
 * This function will send the 2D array over RF
 * It currently prints through the Serial port.
 */
void sendPicture(){
  for(int i = 0; i < PAN_RES; i++){
    for (int k = 0; k < TILT_RES; k++){
      Serial.print(readings[i][k]);
      Serial.print(" ");
    }
    Serial.println();
  }  
}


/*
 * This function iterates through the array of points specified by the servo defaults and resolutions (ie. PAN_DEFAULT, PAN_RES)
 */
void makePicture(){
  
  for (panPos = PAN_DEFAULT - PAN_RES/2; panPos < PAN_DEFAULT + PAN_RES/2; ++panPos) { // PAN loop. Goes around the default angle, res/2 below and res/2 above.
    // in steps of 1 degree
    panServo.write(panPos);              // tell servo to go to position in variable 'pos'
      for (tiltPos = TILT_DEFAULT - TILT_RES/2; tiltPos < TILT_DEFAULT + TILT_RES/2; ++tiltPos) { // TILT loop. Goes around the default angle, res/2 below and res/2 above.
        tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position

        
        readings[panPos-75][tiltPos-75] = byte(mlx.readObjectTempC());
//        Serial.print(int(tiltPos-75));
//        Serial.print(":");
//        Serial.print(String(readings[panPos-75][tiltPos-75]));
//        Serial.print(" ");
      }
//      Serial.println(int(panPos-75));
//      Serial.print("\n");
  }
}




