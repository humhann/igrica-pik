#include "LedControl.h"
#include "pitches.h"

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to LOAD(CS)
 pin 10 is connected to the CLK 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,10,11,1);

/* we always wait a bit between updates of the display */
unsigned long delaytime2=32;

// Rotary module
const int Pin_CLK = 2;  // clk on encoder
const int Pin_DT = 3;  // DT on encoder
const int Pin_SW = 4;  // SW on encoder

int DotPosition = 0;
int x = 0;
int y = 0;
int rotation;  
int value;
boolean LeftRight;

// Map
const int xMap[64] = {
  0, 1, 2, 3, 4, 5, 6, 7,
  7, 7, 7, 7, 7, 7, 7,
  6, 5, 4, 3, 2, 1, 0,
  0, 0, 0, 0, 0, 0,
  1, 2, 3, 4, 5, 6,
  6, 6, 6, 6, 6,
  5, 4, 3, 2, 1,
  1, 1, 1, 1,
  2, 3, 4, 5,
  5, 5, 5,
  4, 3, 2,
  2, 2,
  3, 4,
  4, 3
};
const int yMap[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 2, 3, 4, 5, 6, 7,
  7, 7, 7, 7, 7, 7, 7,
  6, 5, 4, 3, 2, 1,
  1, 1, 1, 1, 1, 1,
  2, 3, 4, 5, 6,
  6, 6, 6, 6, 6,
  5, 4, 3, 2,
  2, 2, 2, 2,
  3, 4, 5,
  5, 5, 5,
  4, 3,
  3, 3,
  4, 4
};

// Enemies
struct Enemy {
  int position;
  boolean alive;
};

const int ENEMIES_SIZE = 5;
struct Enemy enemies[ENEMIES_SIZE] = {
  { 7, true },
  { 12, true },
  { 24, true },
  { 41, true },
  { 58, true },
};

// Non-blocking delays
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillisDotBlinking = 0;        // will store last time LED was updated
const long intervalDotBlinking = 300;

// Passive buzzer
const int Pin_Buzzer = 8;
const int WIN_MELODY_SIZE = 8;
const int LOOSE_MELODY_SIZE = 3;
int winMelody[WIN_MELODY_SIZE] = { NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6 };
int looseMelody[LOOSE_MELODY_SIZE] = { NOTE_E4, NOTE_D4, NOTE_C4 };

// Used for random seed
const int Pin_Empty = 13;

void setup() {
  // TODO: debugging only
  Serial.begin(9600);
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,2);
  /* and clear the display */
  // lc.clearDisplay(0);

  pinMode(Pin_CLK,INPUT);
  pinMode(Pin_DT,INPUT);
  pinMode(Pin_SW,INPUT);
  digitalWrite(Pin_SW, HIGH); // Pull-Up resistor for switch
  rotation = digitalRead(Pin_CLK);

  randomSeed(analogRead(Pin_Empty));
  initializeGame(true);
}

void initializeGame(boolean randomizeEnemyPositions) {
  lc.clearDisplay(0);
  DotPosition = 0;
  lc.setLed(0, xMap[DotPosition], yMap[DotPosition], true);
  // Set enemies
  for (int i=0; i<ENEMIES_SIZE; i++) {
    enemies[i].alive = true;

    if (randomizeEnemyPositions) {
      enemies[i].position = random(2, 63);
    }
    
    lc.setLed(0, xMap[enemies[i].position], yMap[enemies[i].position], true);
  }
}

void winSequence() {
  for (int thisNote = 0; thisNote < WIN_MELODY_SIZE; thisNote++) {
    // Pin_Buzzer output the voice, every scale is 0.5 sencond
    tone(Pin_Buzzer, winMelody[thisNote], 500);

    // Blink display
    for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(0,row,col,true);
      }
    }
  
    // Output the voice after several minutes
    delay(250);
  
    // Blink display
    for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(0,row,col,false);
      }
    }
  
    // Output the voice after several minutes
    delay(250);
  }
}

void looseSequence() {
  for (int thisNote = 0; thisNote < LOOSE_MELODY_SIZE; thisNote++) {
    // Pin_Buzzer output the voice, every scale is 0.5 sencond
    tone(Pin_Buzzer, looseMelody[thisNote], 500);

    // Output the voice after several minutes
    delay(500);
  }
}

/* 
 This function will light up every Led on the matrix.
 The led will blink along with the row-number.
 row number 4 (index==3) will blink 4 times etc.
 */
void single() {
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();
  
  value = digitalRead(Pin_CLK);
  if (value != rotation){ // we use the DT pin to find out which way we turning.
    if (digitalRead(Pin_DT) != value) {  // Clockwise
      LeftRight = true;

      if (DotPosition < 63) {
        DotPosition ++;
      }
    } else { //Counterclockwise
      LeftRight = false;

      if (DotPosition > 0) {
        DotPosition--;
      }
    }

    // Get x and y from DotPosition
    x = xMap[DotPosition];
    y = yMap[DotPosition];

    Serial.print("Dot position: ");
    Serial.print(DotPosition);
    Serial.print(", x");
    Serial.print(x);
    Serial.print(", y");
    Serial.println(y);
  }
  rotation = value;

  lc.setLed(0,x,y,true);

  boolean buttonPressed = false;
  if (!digitalRead(Pin_SW)) {
    // Serial.print("BUTTON PRESSED!");
    buttonPressed = true;
  }

  for (int i=0; i<ENEMIES_SIZE; i++) {
    // Shoot enemy
    if (buttonPressed && (DotPosition + 2 == enemies[i].position || DotPosition + 1 == enemies[i].position)) {
      lc.setLed(0, xMap[enemies[i].position], yMap[enemies[i].position], false);
      enemies[i].alive = false;
    }

    // Check if enemy is shooting
    if (enemies[i].alive && enemies[i].position <= DotPosition) {
      // Player dies, reset game
      looseSequence();
      initializeGame(false);
    }
  }

  // Win 
  if (DotPosition == 63) {
    winSequence();
    initializeGame(true);
  }
}

void loop() {
  single();
}
