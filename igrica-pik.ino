#include "LedControl.h"
#include "pitches.h"

// MAX7219 LED Module
const int Pin_LED_DIN = 12;
const int Pin_LED_CLK = 10;
const int Pin_LED_CS = 11;
LedControl lc=LedControl(Pin_LED_DIN, Pin_LED_CLK, Pin_LED_CS, 1);

// Rotary encoder module
const int Pin_Rotary_CLK = 2;
const int Pin_Rotary_DT = 3;
const int Pin_Rotary_SW = 4;

// Passive buzzer
const int Pin_Buzzer = 8;
const int WIN_MELODY_SIZE = 8;
const int LOOSE_MELODY_SIZE = 3;
int winMelody[WIN_MELODY_SIZE] = { NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6 };
int looseMelody[LOOSE_MELODY_SIZE] = { NOTE_E4, NOTE_D4, NOTE_C4 };

// Player position, rotary encoder module movement and button
int playerPosition = 0;
int rotation;  
int value;
boolean isClockwise;
boolean buttonPressed = false;

// Map path (spiral pattern)
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
  4, 3,
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
  4, 4,
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

// Used for random seed
const int Pin_Empty = 13;

void setup() {
  // Debugging
  // Serial.begin(9600);
  
  // The MAX72XX is in power-saving mode on startup, we have to do a wakeup call
  lc.shutdown(0, false);
  
  // Set LED brightness
  lc.setIntensity(0, 1);

  // Pins
  pinMode(Pin_Rotary_CLK, INPUT);
  pinMode(Pin_Rotary_DT, INPUT);
  pinMode(Pin_Rotary_SW, INPUT);
  digitalWrite(Pin_Rotary_SW, HIGH); // Pull-Up resistor for switch
  rotation = digitalRead(Pin_Rotary_CLK);
  randomSeed(analogRead(Pin_Empty));

  // New game
  initializeGame(true);
}

void initializeGame(boolean randomizeEnemyPositions) {
  lc.clearDisplay(0);
  playerPosition = 0;
  lc.setLed(0, xMap[playerPosition], yMap[playerPosition], true);

  // Set enemies
  for (int i = 0; i < ENEMIES_SIZE; i++) {
    enemies[i].alive = true;

    if (randomizeEnemyPositions) {
      enemies[i].position = random(2, 63);
    }
    
    lc.setLed(0, xMap[enemies[i].position], yMap[enemies[i].position], true);
  }
}

void winSequence() {
  const int TONE_DURATION = 500;
  
  for (int thisNote = 0; thisNote < WIN_MELODY_SIZE; thisNote++) {
    tone(Pin_Buzzer, winMelody[thisNote], TONE_DURATION);

    // Blink display
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        lc.setLed(0, x, y, true);
      }
    }
  
    delay(TONE_DURATION / 2);
  
    // Blink display
    for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
        lc.setLed(0, x, y, false);
      }
    }
  
    delay(TONE_DURATION / 2);
  }
}

void looseSequence() {
  const int TONE_DURATION = 500;
 
  for (int thisNote = 0; thisNote < LOOSE_MELODY_SIZE; thisNote++) {
    tone(Pin_Buzzer, looseMelody[thisNote], TONE_DURATION);
    delay(TONE_DURATION);
  }
}

void loop() {
  // we use the DT pin to find out which way we turning
  value = digitalRead(Pin_Rotary_CLK);

  if (value != rotation) {
    if (digitalRead(Pin_Rotary_DT) != value) {
      // Clockwise
      isClockwise = true;

      if (playerPosition < 63) {
        playerPosition ++;
      }
    } else {
      // Counterclockwise
      isClockwise = false;

      if (playerPosition > 0) {
        playerPosition--;
      }
    }

    // Debugging
    /* Serial.print("Player position: ");
    Serial.print(playerPosition);
    Serial.print(", x");
    Serial.print(xMap[playerPosition]);
    Serial.print(", y");
    Serial.println(yMap[playerPosition]); */
  }

  rotation = value;

  // Draw player position
  lc.setLed(0, xMap[playerPosition], yMap[playerPosition], true);

  if (!digitalRead(Pin_Rotary_SW)) {
    buttonPressed = true;
  } else {
    buttonPressed = false;
  }

  for (int i = 0; i < ENEMIES_SIZE; i++) {
    // Shoot enemy
    if (buttonPressed && (playerPosition + 2 == enemies[i].position || playerPosition + 1 == enemies[i].position)) {
      lc.setLed(0, xMap[enemies[i].position], yMap[enemies[i].position], false);
      enemies[i].alive = false;
    }

    // Check if enemy is shooting
    if (enemies[i].alive && enemies[i].position <= playerPosition) {
      // Player dies, reset game using same enemy positions (no randomizeEnemyPositions)
      looseSequence();
      initializeGame(false);
    }
  }

  // Win 
  if (playerPosition == 63) {
    // New game
    winSequence();
    initializeGame(true);
  }
}
