//for serial communcation
import processing.serial.*;

//for email notification. Must install the Temboo library
import com.temboo.core.*;
import com.temboo.Library.Google.Gmail.*;

import java.net.URL;
import java.util.GregorianCalendar; //for timestamp
import java.util.Calendar;


int LOAD_IMG_X;
int LOAD_IMG_Y;

int SAVE_IMG_X;
int SAVE_IMG_Y;

int BUT_WID = 120;
int BUT_HEI = 40;

int EXIT_X;
int EXIT_Y;
int EXIT_RAD;

int TAKE_IMG_X;
int TAKE_IMG_Y;
int TAKE_IMG_WID;
int TAKE_IMG_HEI;

int TAKE_TEM_X;
int TAKE_TEM_Y;
int TAKE_TEM_WID;
int TAKE_TEM_HEI;

//These are set by the config file
boolean logging = false;
boolean sendEmail = false;
String emailAddress = "";
int loggingDelay = 1000; //in seconds
float threshold = 0;

float pointTemp = 0;
long lastLog;


GridDisplay grid;
Serial myPort;

// Create a session using your Temboo account application details
TembooSession session = new TembooSession("thermalimaging", "myFirstApp", "QnHzPXgjU69sevWkCWhOJ9N3TMDulcH0");
int resolution = 30;

boolean heatMapInProgress;

//setup function creates the objects
void setup(){
  
  
  size(900,800); 
  surface.setTitle("Thermal Imaging Device");
  
    
  // Self- adjusting button locations based on app size
  LOAD_IMG_X = width/20;
  LOAD_IMG_Y = height/3;
  
  SAVE_IMG_X = width/20;
  SAVE_IMG_Y = height/4;
  
  EXIT_X = width*3/4;
  EXIT_Y = height*93/100;
  EXIT_RAD = 50;
  
  TAKE_IMG_X = width * 1/4;
  TAKE_IMG_Y = height*90/100;
  TAKE_IMG_WID = 120;
  TAKE_IMG_HEI = 40;
  
  TAKE_TEM_X = width * 2/4;
  TAKE_TEM_Y = height*90/100;
  TAKE_TEM_WID = 120;
  TAKE_TEM_HEI = 40;
  
  //create the grid, at x=200, y=100, with resolution
  grid = new GridDisplay(200,100,resolution); 
  
  heatMapInProgress = false;
  
  //This chooses the serial port of the Client Jeenode
  if(Serial.list().length >= 3){
    myPort = new Serial(this, Serial.list()[Serial.list().length - 1], 9600); 
    myPort.bufferUntil('\n'); 
    delay(100);
  }else{
    println("No Arduino Connected");
    myPort = null; 
  }
 
  textAlign(CENTER, CENTER);
  
  //Read the config file and set the variables
  readConfig();
  
}

//loop function, draws the graphics
void draw(){
  background(0x40);
  textSize(12);
  grid.draw();
  
  //Draw the buttons
  fill(0x9f);
  rect(SAVE_IMG_X,SAVE_IMG_Y,BUT_WID,BUT_HEI,5);
  rect(LOAD_IMG_X,LOAD_IMG_Y,BUT_WID,BUT_HEI,5);
  ellipse(EXIT_X,EXIT_Y,EXIT_RAD,EXIT_RAD); 
  rect(TAKE_IMG_X,TAKE_IMG_Y,TAKE_IMG_WID,TAKE_IMG_HEI,5);
  rect(TAKE_TEM_X,TAKE_TEM_Y,TAKE_TEM_WID,TAKE_TEM_HEI,5);
  
  fill(0xd0);
  rect(width*0.35,height*79/100, 300,55,5);
    
  fill(0);
  
  //Add text to buttons
  text("Exit",EXIT_X,EXIT_Y);
  text("Take Image",TAKE_IMG_X + TAKE_IMG_WID/2,TAKE_IMG_Y + TAKE_IMG_HEI/2);
  text("Take Temp",TAKE_TEM_X + TAKE_IMG_WID/2,TAKE_TEM_Y + TAKE_TEM_HEI/2);
  text("Save Img",SAVE_IMG_X + BUT_WID/2,SAVE_IMG_Y + BUT_HEI/2);
  text("Load Img",LOAD_IMG_X + BUT_WID/2,LOAD_IMG_Y + BUT_HEI/2);
  
  //Keep track of when the last log was and when the next should be
  if(logging)
    if(millis()-lastLog> loggingDelay*1000){
      lastLog = millis();

      requestHeatMap();
    }
    
  textSize(20);
  text("Last Point Reading: " + pointTemp, width*0.5,height*82/100); 
  
}

//Once a heat map has been read in from the client, we can log it and send an alert email if that is necessary
void endOfHeatMap(){
  if(logging){
    saveToFile();
    if(sendEmail && grid.getMaxTemp() > threshold){
      runSendEmail();
    }
  }
}

//Called at setup. loads the data from the config file and stores it in local variables
void readConfig(){
  String[] configFile = loadStrings("config.txt");
  if(configFile.length != 2)
    return;
  
  configFile = split(configFile[1],',');
  logging = configFile[0].equals("1");
  loggingDelay = int(configFile[1]);
  threshold = int(configFile[2]);
  sendEmail = configFile[3].equals("1");
  emailAddress = configFile[4];
  
  println(logging);
  println(loggingDelay);
  println(threshold);
  println(sendEmail);
  println(emailAddress);
}

//handles the incoming data from the serial Port. 
void serialEvent (Serial myPort) {
  // get the ASCII string:
  String inString = myPort.readStringUntil('\n');
  
  if(inString.substring(0,1).equals("!")){
    pointTemp = float(inString.substring(1));
    return;
  }
  
  //println(inString);
  parseRow(inString,true);
  if(heatMapInProgress)
    endOfHeatMap();
}

//Reads a logged file and parses the data
void readFile(File f){
  if(f == null){
    println("No file found");
    return;
  }

  String[] lines = loadStrings(f.getPath());
  for(String line :lines){
    //println(line);
    if(!parseRow(line,false)){
      println("error Reading file");
      break;
    }
  }
}

//Sends the appropriate command to the Client Jeenode in order to get a heat map
void requestHeatMap(){
   println("Requesting Heat Map");
   if(myPort != null)
     myPort.write(0xAA); 
}

// Sends the appropriate command to the Client JeeNode in order to read one temperature point
void requestCurrentTemp(){
  println("Requesting Point Temp");
  if(myPort != null)
    myPort.write(0xBB);
}

/*
 * This function is for button handling. Checks the position of the mouse when it is released and performs the corresponding function
 */
void mouseReleased(){
  if(mouseX > LOAD_IMG_X && mouseX < LOAD_IMG_X + BUT_WID && mouseY > LOAD_IMG_Y && mouseY < LOAD_IMG_Y + BUT_HEI){
    selectInput("Select a file to process:", "readFile");
  }else if(mouseX > SAVE_IMG_X && mouseX < SAVE_IMG_X + BUT_WID && mouseY > SAVE_IMG_Y && mouseY < SAVE_IMG_Y + BUT_HEI){
    saveToFile();
  }else if((mouseX-EXIT_X)*(mouseX-EXIT_X) + (mouseY- EXIT_Y)*(mouseY- EXIT_Y) <= EXIT_RAD/2*EXIT_RAD/2){
    exit();
  }else if(mouseX > TAKE_IMG_X && mouseX < TAKE_IMG_X + TAKE_IMG_WID && mouseY > TAKE_IMG_Y && mouseY < TAKE_IMG_Y + TAKE_IMG_HEI){
    requestHeatMap();
  }else if(mouseX > TAKE_TEM_X && mouseX < TAKE_TEM_X + TAKE_TEM_WID && mouseY > TAKE_TEM_Y && mouseY < TAKE_IMG_Y + TAKE_TEM_HEI){
    requestCurrentTemp();
  }

}

//Function that sends the email out
void runSendEmail() {
  
  if(session == null){
    println("Error sending Email");
    return;
  }
  // Create the Choreo object using your Temboo session
  SendEmail sendEmailChoreo = new SendEmail(session);

  // Set inputs
  sendEmailChoreo.setFromAddress("thermalimagingresponse@gmail.com");
  sendEmailChoreo.setUsername("thermalimagingresponse@gmail.com");
  sendEmailChoreo.setSubject("Thermal Imaging Device Warning");
  sendEmailChoreo.setToAddress("nathanlim@cmail.carleton.ca");
  String text = "Your thermal imaging device detected a maximum temperature of " + grid.getMaxTemp() + " degrees C which is higher than the threshold temperature of " + threshold + " degrees C";
  sendEmailChoreo.setMessageBody(text);
  sendEmailChoreo.setPassword("gdmw wymx aynd omca");

  // Run the Choreo and store the results
  SendEmailResultSet sendEmailResults = sendEmailChoreo.run();
  
  // Print results
  println(sendEmailResults.getSuccess());
  println("Sent Email");

}

/*
 * Line is a string that represents a row. format is "ROWX:data1,data2,data3,..." X is the row #. data can be divided by ten or not, chosen by the bool 'divideByTen'
 */
boolean parseRow(String line, boolean divideByTen){
  int row = 0;
  
  if(line.length() < 3)
    return false;
  
  if(line.substring(0,3).equals("ROW")){
    row = int(line.substring(3,line.indexOf(':')));
    
    heatMapInProgress = row == resolution-1? false:true;
    
  }else{
    return false; 
  }
  
  if(line.indexOf(':') == -1)
    return false;
  
  line = line.substring(line.indexOf(':')+1);
 
  String[] values = line.split(",");
  
  float fVals[] = new float[values.length];
  
  for(int i = 0; i< values.length; i++){
     fVals[i] = float(values[i]);
     if(divideByTen)
       fVals[i] /= 10;
  }

  grid.addRow(row, fVals);
  
  return true;
}

/*
 * This function takes the array from the grid and saves it to a file
 */
void saveToFile(){
  
  //setup the timestamp for the filename
  GregorianCalendar cal = new GregorianCalendar();
  String stamp = str(cal.get(Calendar.YEAR) % 1000) + "-" + str(cal.get(Calendar.MONTH)) + "-" + str(cal.get(Calendar.DAY_OF_MONTH)) + "-" + str(cal.get(Calendar.HOUR)) + "-" + str(cal.get(Calendar.MINUTE)) + "-" + str(cal.get(Calendar.SECOND));
  PrintWriter out = createWriter("data/heatmap" + stamp + ".txt");
  
  float[][] arr =  grid.getGrid();
  String line;
  for(int i=0;i<arr.length;i++){
     line = join(str(arr[i]),",");
     out.println("ROW" + i + ":" +line);
  }
  out.flush();
  out.close();
}

