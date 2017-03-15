final int gridWidth = 500;
final int gridHeight = 500;
final int squareSize = 500/30; //this controls the resolution of the image

class GridDisplay{
  float[][] array;
  int x,y;
  
  public GridDisplay(int x, int y){
    array = new float[gridWidth/squareSize][gridHeight/squareSize];
    
    this.x=x;
    this.y=y;
    
    makeGrey();
  }
  
  void makeGrey(){
    for(int i = 0;i<array.length;i++){
      for(int j = 0;j<array.length;j++){
        array[i][j] = (i+j);
      }
    }
  }
  
  void addRow(int rn, float[] row){
     array[rn] = row;  
  }
  
  void draw(){
    noStroke();
    for(int i = 0;i<array.length;i++){
      for(int j = 0;j<array[i].length;j++){
        fill(array[i][j]);
        rect(x+i*squareSize,y+j*squareSize,squareSize,squareSize);
      }
    }
  }
  
}
