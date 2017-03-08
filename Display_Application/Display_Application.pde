
thermal_view view;

void setup(){
  view = new thermal_view();
  
  
  size(700,700); 
}

void draw(){
  view.draw();
  
  
  
}

class thermal_view{
  int EXIT_X = width*3/4;
  int EXIT_Y = height*93/100;
  int EXIT_RAD = 50;
  GridDisplay grid;
  
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