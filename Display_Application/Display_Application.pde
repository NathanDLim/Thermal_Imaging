GridDisplay grid;

void setup(){
  
  grid = new GridDisplay(100,100);
  
  size(700,700); 
}

void draw(){
  grid.draw();
}