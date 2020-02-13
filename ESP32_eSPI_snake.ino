/////////////////////////////////////////////////////////////////////////////
//
//  A Simple Game of Snake
//  ESP32 + TFT_eSPI on TTGO T-Display

float gameSpeed = 5;  //Higher faster

//#include <Adafruit_GFX.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define BUTTON_1        35
#define BUTTON_2        0

#define blue_width 129
#define blue_heigh 216

boolean start = false; //will not start without say-so
unsigned long offsetT = 0; //time delay for touch
unsigned long offsetM = 0; //time delay for main loop

float gs;
int headX = 1;        //coordinates for head
int headY = 1;
int beenHeadX[170];   //coordinates to clear later
int beenHeadY[170];
int changeX = 0;      //the direction of the snake
int changeY = 1;
boolean lastMoveH = false; //to keep from going back on oneself
int score = 1;
int foodX;            //coordinates of food
int foodY;
boolean eaten = true; //if true a new food will be made
int loopCount = 0; //number of times the loop has run
int clearPoint = 0;  //when the loopCount is reset
boolean clearScore = false;

//initialize the display
TFT_eSPI tft = TFT_eSPI(135, 240);

void setup() {
  gs = 1000 / gameSpeed; //calculated gameSpeed in milliseconds
  
  memset(beenHeadX, 0, sizeof(beenHeadX)); //initiate beenHead with a bunch of zeros
  memset(beenHeadY, 0, sizeof(beenHeadY));
  
  tft.begin();           //turn on display
  tft.setRotation(0);
  
  tft.fillScreen(TFT_WHITE);                //sets background
  tft.fillRect(3, 21, blue_width, blue_heigh, TFT_BLUE);
  
  tft.setTextColor(TFT_WHITE); //Start notification
  tft.setTextSize(3);
  tft.setCursor(0, 120);
  tft.print(">START<");

  tft.setTextColor(TFT_BLACK); //Score keeper
  tft.setTextSize(2);
  tft.setCursor(5, 3);
  tft.print("Score: ");
  printScore();

  randomSeed(analogRead(6)); //make every game unique

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_1),right, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2),left, FALLING);
}

void loop() {
  if (clearScore and start) { //resets score from last game, won't clear
    score = 1;                //until new game starts so you can show off
    printScore();             //your own score
    clearScore = false;
  }
  if (millis() - offsetM > gs and start) {
    beenHeadX[loopCount] = headX;  //adds current head coordinates to be
    beenHeadY[loopCount] = headY;  //covered later
    
    headX = headX + (changeX);  //head moved
    headY = headY + (changeY); 
    
    if (headX - foodX == 0 and headY - foodY == 0) { //food
      score += 1;
      printScore();
      eaten = true;
    }

    loopCount += 1; //loopCount used for addressing, mostly
    
    if (loopCount > 169) {            //if loopCount exceeds size of
      clearPoint = loopCount - score; //beenHead arrays, reset to zero
      loopCount = 0;
    }
    
    drawDot(headX, headY); //head is drawn
    
    if (loopCount - score >= 0) { //if array has not been reset
      eraseDot(beenHeadX[loopCount - score], beenHeadY[loopCount - score]);
    }  //covers end of tail
    else {
      eraseDot(beenHeadX[clearPoint], beenHeadY[clearPoint]);
      clearPoint += 1;
    }
 
    if (eaten) {     //randomly create a new piece of food if last was eaten
      do {           //but not inside the snake's body
        foodX = random(2, 9);
        foodY = random(2, 17);
      } while (belongsToBody(foodX, foodY));
      eaten = false;
    }

    drawDotRed(foodX, foodY); //draw the food

    if (headX > 10 or headX < 1 or headY < 1 or headY > 18) { //Boudaries
      endGame();
    }

    if (belongsToBody(headX, headY)) {
      endGame();
    }

    offsetM = millis(); //reset game loop timer
  }
}

void endGame() {
  tft.fillRect(3, 21, blue_width, blue_heigh, TFT_BLUE); //deletes the old game
  
  eaten = true; //new food will be created
  
  tft.setCursor(30, 100);       //Retry message
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.print("GAME");
  tft.setCursor(30, 130);
  tft.print("OVER!");
  tft.setTextColor(TFT_BLACK); //sets back to scoreboard settings
  tft.setTextSize(2);
  
  tft.setCursor(5, 3);
  tft.print("Score: ");
  
  headX = 1;              //reset snake
  headY = 1;
  changeX = 0;
  changeY = 1;
  lastMoveH = false;

  memset(beenHeadX, 0, sizeof(beenHeadX)); //clear the beenHead arrays
  memset(beenHeadY, 0, sizeof(beenHeadY)); //probably not necessary

  loopCount = 0;
  clearScore = true;
  start = false;     //stops game
}

void drawDot(int x, int y) {
  tft.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, TFT_WHITE);
}

void drawDotRed(int x, int y) {
  tft.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, TFT_RED);
}

void eraseDot(int x, int y) {
  tft.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, TFT_BLUE);
}

void printScore() {
  tft.fillRect(88, 3, 50, 16, TFT_WHITE);//clears old score
  tft.setCursor(88, 3);
  tft.print(score);                            //prints current score
}

bool belongsToBody(int x, int y)
{
  if (loopCount - score < 0) {         //check to see if head is on tail
    for (int j = 0; j < loopCount; j++) {
      if (x == beenHeadX[j] and y == beenHeadY[j]) {
        return true;
      }
    }
    for (int k = clearPoint; k < 152; k++) {
      if (x == beenHeadX[k] and y == beenHeadY[k]) {
        return true;
      }
    }
  }
  else {
    for (int i = loopCount - (score - 1); i < loopCount; i++) {
      if (x == beenHeadX[i] and y == beenHeadY[i]) {
        return true;
      }
    }
  }

  return false;
}

void left() {
  if (start)
  {
    if (millis() - offsetT > gs and !lastMoveH) {
      //if (changeY==-1)
      //{
        changeX = -1;
        changeY = 0;
        offsetT = millis();
        lastMoveH = true;
      //}
    }
    else if (millis() - offsetT > gs and lastMoveH) {
      if (changeX==1)
      {
        changeX = 0;
        changeY = -1;
        offsetT = millis();
        lastMoveH = false;
      }
      else if (changeX==-1)
      {
        changeX = 0;
        changeY = 1;
        offsetT = millis();
        lastMoveH = false;
      }
    }
  }
  else if (millis() - offsetT > gs and !start)
  {
    tft.fillRect(3, 100, 126, 60, TFT_BLUE); //Erase start message
    start = true;                                //allows loop to start
    offsetT = millis();
  }
}

void right() {
  if (millis() - offsetT > gs and !lastMoveH) {
    //if (changeY==-1)
    //{
      changeX = 1;
      changeY = 0;
      offsetT = millis();
      lastMoveH = true;
    //}
  }
  else if (millis() - offsetT > gs and lastMoveH) {
    if (changeX==1)
    {
      changeX = 0;
      changeY = 1;
      offsetT = millis();
      lastMoveH = false;
    }
    else if (changeX==-1)
    {
      changeX = 0;
      changeY = -1;
      offsetT = millis();
      lastMoveH = false;
    }
  }
}
