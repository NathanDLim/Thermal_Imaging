


class GridDisplay{
  final int gridWidth = 500;
  final int gridHeight = 500;
  float squareSize; //this controls the resolution of the image
  
  
  float[][] array;
  int x,y;
  float max,  min;
  
  //color blue = color(0,0,255); //0
  //color cyan = color(0,128,128); //0.25
  //color green = color(0,255,0); //0.5
  //color yellow = color(255,255,0); //0.75
  //color red = color(255,0,0); //1
  
  public GridDisplay(int x, int y, int res){
    squareSize = gridWidth/float(res);
    array = new float[floor(gridWidth/squareSize)][floor(gridHeight/squareSize)];
    this.x=x;
    this.y=y;
    max = 58;
    min = 0;
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
    if(rn ==0){
      max = 0; min = 100;
    }
     array[rn] = row;  
    for(int i = 0;i< row.length;i++){
      if(row[i] > max)
        max = row[i];
      else if(row[i] < min)
        min = row[i];
    }
  }
  
  float normalize(float val){
    return (val-min)/(max-min);
  }
  
  color heatMapping(float val){
   int r=0, g=0,b=0;
   if(val >=0 && val < 0.25){ //blue - cyan
      r = 0;
      g = int(val*512);
      b = int(255-512*val);
   }else if(val >= 0.25 && val <0.5){ //cyan -green
      r = 0;
      g = int(val*512);
      b = int(255-512*val);
   }else if(val >= 0.5 && val < 0.75){ //green - yellow
      r = int(1024*val-512);
      g = 255;
      b = 0;
   }else if(val >=0.75){ //yellow - red
      r = 255;
      g = int(-1024*val+1024);
      b = 0;
   }
   
   return color(r,g,b);
  }
  
  void draw(){
    fill(0xe0);
    rect(x-5,y-5,gridHeight+10,gridWidth+10);
    noStroke();
    for(int i = 0;i<array.length;i++){
      for(int j = 0;j<array[i].length;j++){
        fill(heatMapping(normalize(array[i][j])));
        rect(x+j*squareSize,y+i*squareSize,squareSize,squareSize);
      }
    }
    
    for(int i = 0; i < 20;i++){
       fill(heatMapping(i/20.0));
       rect(x+ gridWidth + 40,y + gridHeight*0.75 - i*squareSize, squareSize,squareSize);
    }
    
    fill(0xff);
    text(String.format("%.1f",max), x+ gridWidth + 75,y + gridHeight*0.75 - 19*squareSize);
    text(String.format("%.1f",(max-min)/2), x+ gridWidth + 75,y + gridHeight*0.75 - 9*squareSize);
    text(String.format("%.1f",min), x+ gridWidth + 75,y + gridHeight*0.75 + squareSize);
  }
  
  float[][] getGrid(){
    return array;
  }
  
  float getMaxTemp(){
    return max; 
  }
  
}
