import processing.serial.*;


thermal_view view;

Serial myPort;

void setup(){
  view = new thermal_view();
  
  
  size(700,700); 
  
  
  if(Serial.list().length >= 3){
    myPort = new Serial(this, Serial.list()[Serial.list().length - 1], 9600); 
    myPort.bufferUntil('\n'); 
    delay(100);
  }else{
    println("No Arduino Connected");
    myPort = null; 
  }
  
  textAlign(CENTER, CENTER);
  System.out.println("Working Directory = " +
              System.getProperty("user.dir"));
  try{
    println(readLines("test.txt"));
  }catch(IOException e){println("no file found");}
}

void draw(){
  background(0x60);
  view.draw();
  
  if(keyPressed){
     println("sent !");
     myPort.write("!"); 
     delay(100);
  }
  
}

void serialEvent (Serial myPort) {
  // get the ASCII string:
  String inString = myPort.readStringUntil('\n');
  //println(inString);
  int row=0;
  
  if(inString.substring(0,3).equals("ROW")){
    row = int(inString.substring(3,inString.indexOf(':')));
    //println("row #" + row);
  }
  inString = inString.substring(inString.indexOf(':')+1);
  String[] values = inString.split(",");
  float fVals[] = new float[values.length];
  
  for(int i = 0; i< values.length; i++){
     fVals[i] = float(values[i])/10;
     //print(fVals[i] + " ");
  }
  view.addRow(row, fVals);
}

void mouseReleased(){
  view.mouseClick(); 
}

class thermal_view{
  int EXIT_X = width*3/4;
  int EXIT_Y = height*93/100;
  int EXIT_RAD = 50;
  
  int TAKE_IMG_X = width * 1/4;
  int TAKE_IMG_Y = height*90/100;
  int TAKE_IMG_WID = 120;
  int TAKE_IMG_HEI = 40;
  
  int TAKE_TEM_X = width * 2/4;
  int TAKE_TEM_Y = height*90/100;
  int TAKE_TEM_WID = 120;
  int TAKE_TEM_HEI = 40;
  GridDisplay grid;
  
  void addRow(int rn, float[] row){
     grid.addRow(rn,row);
  }
  
  public thermal_view(){
     grid = new GridDisplay(100,100); 
  }
  
  void draw(){
    grid.draw();
    
    fill(0x9f);
    ellipse(EXIT_X,EXIT_Y,EXIT_RAD,EXIT_RAD); 
    rect(TAKE_IMG_X,TAKE_IMG_Y,TAKE_IMG_WID,TAKE_IMG_HEI,5);
    rect(TAKE_TEM_X,TAKE_TEM_Y,TAKE_TEM_WID,TAKE_TEM_HEI,5);
    
    fill(0);
    text("Exit",EXIT_X,EXIT_Y);
    text("Take Image",TAKE_IMG_X + TAKE_IMG_WID/2,TAKE_IMG_Y + TAKE_IMG_HEI/2);
    text("Take Temp",TAKE_TEM_X + TAKE_IMG_WID/2,TAKE_TEM_Y + TAKE_TEM_HEI/2);
  }
  
  void mouseClick(){
   if((mouseX-EXIT_X)*(mouseX-EXIT_X) + (mouseY- EXIT_Y)*(mouseY- EXIT_Y) <= EXIT_RAD/2*EXIT_RAD/2)
     exitButton();
   else if(mouseX > TAKE_IMG_X && mouseX < TAKE_IMG_X + TAKE_IMG_WID && mouseY > TAKE_IMG_Y && mouseY < TAKE_IMG_Y + TAKE_IMG_HEI){
     println("take im pressed");
   }else if(mouseX > TAKE_TEM_X && mouseX < TAKE_TEM_X + TAKE_TEM_WID && mouseY > TAKE_TEM_Y && mouseY < TAKE_IMG_Y + TAKE_TEM_HEI){
     println("take TEMP pressed");
   }
  }
  
  void exitButton(){
     exit(); 
  }
  
  void writeToFile(){
    //FileWriter write = new FileWriter(, false);
  }
}
