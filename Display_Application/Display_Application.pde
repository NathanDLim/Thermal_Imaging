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
  

}

void draw(){
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
  println(inString);
  int row=0;
  if(inString.substring(0,3) == "ROW"){
    row = int(inString.substring(2,inString.indexOf(':')));
    println("row #" + row);
  }
  //inString = inString.substring(inString.indexOf(':')+1);
  //String[] values = inString.split(",");
  //float fVals[] = new float[values.length];
  
  //for(int i = 0; i< values.length; i++){
  //   fVals[i] = float(values[i])/10;
  //   print(fVals[i] + " ");
  //}
  //view.addRow(row, fVals);
}

class thermal_view{
  int EXIT_X = width*3/4;
  int EXIT_Y = height*93/100;
  int EXIT_RAD = 50;
  GridDisplay grid;
  
  void addRow(int rn, float[] row){
     grid.addRow(rn,row);
  }
  
  public thermal_view(){
     grid = new GridDisplay(100,100); 
  }
  
  void draw(){
    grid.draw();
    
    fill(0);
    ellipse(EXIT_X,EXIT_Y,EXIT_RAD,EXIT_RAD); 
  }
  
  void exitButton(){
     exit(); 
  }
}
