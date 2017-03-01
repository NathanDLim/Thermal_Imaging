final int gridWidth = 500;
final int gridHeight = 500;
final int squareSize = 10; //this controls the resolution of the image

class GridDisplay{
  int[][] array;
  int x,y;
  
  public GridDisplay(int x, int y){
    array = new int[gridWidth/squareSize][gridHeight/squareSize];
    
    for(int i = 0;i<array.length;i++){
      for(int j = 0;j<array.length;j++){
        array[i][j] = (i+j);
      }
    }
    
    this.x=x;
    this.y=y;
    
  }
  
  void draw(){
    noStroke();
    for(int i = 0;i<array.length;i++){
      for(int j = 0;j<array.length;j++){
        fill(array[i][j]);
        rect(x+i*squareSize,y+j*squareSize,squareSize,squareSize);
      }
    }
  }
  
}