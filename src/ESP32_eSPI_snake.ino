/////////////////////////////////////////////////////////////////////////////
//
//  A Simple Game of Snake
//  ESP32 + TFT_eSPI on TTGO T-Display

float gameSpeed = 5;  //Higher faster

//#include <Adafruit_GFX.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define ADC_EN          14
#define ADC_PIN         34
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
int score = 1;
int foodX;            //coordinates of food
int foodY;
boolean eaten = true; //if true a new food will be made
int loopCount = 0; //number of times the loop has run
int clearPoint = 0;  //when the loopCount is reset
boolean clearScore = false;
uint32_t suspendCount = 0;

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
  if (suspendCount++>10000000) suspend(); //auto suspend after ~10 sec.
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

void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void suspend()
{
  suspendCount=0;
  int r = digitalRead(TFT_BL);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Press again to wake up", tft.width() / 2, tft.height() / 2);
  espDelay(4000);
  digitalWrite(TFT_BL, !r);

  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
  delay(200);
  esp_deep_sleep_start();
}

void left() {
  suspendCount=0;
  if (start)
  {
    if (millis() - offsetT > gs ) {
      if (changeX==1)
      {
        changeX = 0;
        changeY = -1;
        offsetT = millis();
      }
      else if (changeX==-1)
      {
        changeX = 0;
        changeY = 1;
        offsetT = millis();
      }
      else if (changeY==1)
      {
        changeX = 1;
        changeY = 0;
        offsetT = millis();
      }
      else if (changeY==-1)
      {
        changeX = -1;
        changeY = 0;
        offsetT = millis();
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
  suspendCount=0;
  if (millis() - offsetT > gs ) {
    if (changeX==1)
    {
      changeX = 0;
      changeY = 1;
      offsetT = millis();
    }
    else if (changeX==-1)
    {
      changeX = 0;
      changeY = -1;
      offsetT = millis();
    }
    else if (changeY==1)
    {
      changeX = -1;
      changeY = 0;
      offsetT = millis();
    }
    else if (changeY==-1)
    {
      changeX = 1;
      changeY = 0;
      offsetT = millis();
    }
  }
}
