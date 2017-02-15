/* Sweep
 by BARRAGAN <http://barraganstudio.com>
 This example code is in the public domain.

 modified 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Sweep
*/

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>


Servo panServo;  // create servo objects to control the servos
Servo tiltServo;  
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // create the temp sensor object

bool runMotors = false;

int panPos = 50;    // variable to store the servo position
int tiltPos = 80;

byte readings[30][30];

void setup() {
  panServo.attach(4);  // attaches the servo on pin 9 to the servo object
  tiltServo.attach(5);

  mlx.begin();  
  Serial.begin(9600);

}

void loop() {
  if (Serial.available()>0){
    if(Serial.read() == '!')
      makePicture();
      printPicture();
  }
  
//  if(runMotors){
//    tiltPos++;
//    tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
//    delay(15);
//    if(tiltPos == 100){
//      tiltPos = 80;
//      panPos++;
//      panServo.write(panPos);              // tell servo to go to position in variable 'pos'
//      delay(15);
//      if(panPos == 130){
//        panPos = 50;
//      }
//    }
//  }

  delay(15);


}

void printPicture(){
//  String out = "";

  for(int i = 0; i < 30; i++){
    for (int k = 0; k<30; k++){
      Serial.print(readings[i][k]);
      Serial.print(" ");
//      out += String(i) + "," + String(k) + ":" + String(readings[i][k]) + " ";
    }
    Serial.println();
//    out += "\n";
  }

//  Serial.print(out);
  
}


void makePicture(){
  
  for (panPos = 75; panPos < 105; panPos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    panServo.write(panPos);              // tell servo to go to position in variable 'pos'
      for (tiltPos = 75; tiltPos < 105; tiltPos += 1) { // goes from 180 degrees to 0 degrees
        tiltServo.write(tiltPos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
        Serial.print(int(tiltPos-75));
        Serial.print(":");
        
        readings[panPos-75][tiltPos-75] = byte(mlx.readObjectTempC());
        Serial.print(String(readings[panPos-75][tiltPos-75]));
         Serial.print(" ");
      }
      Serial.println(int(panPos-75));
      Serial.print("\n");
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}




