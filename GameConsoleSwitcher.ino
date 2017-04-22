#include "FastLED.h"
#include <ClickEncoder.h>
#include <TimerOne.h>

// ### CONFIG LED ###
#define LED_NUMBER 50 // Total number of LED
#define LED_DATA_PIN 6 // Data pin that led data will be written out over
CRGB ledsDefaultColor = CRGB::White;

// ### CONFIG Encoder ###
#define ENCODER_SWITCH_PIN A2
#define ENCODER_CLOCK_PIN A0
#define ENCODER_DATA_PIN A1

// ### CONFIG GameConsoles ###
typedef struct
{
  int portNr;
  String deviceName;
  bool syncStripper;
  int ledColorRed;
  int ledColorGreen;
  int ledColorBlue;
  int ledStart;
  int ledEnd;
}  GameConsole;
GameConsole gameConsoleList[12] = { 
  { 1, "NES", false, 255, 0, 0, 0, 3}, 
  { 2, "SNES", true, 255, 0, 0, 4, 7},
  { 3, "N64", true, 255, 0, 0, 8, 11},
  { 4, "Game Cube", true, 255, 0, 0, 12, 15},
  { 5, "SNES NTSC", true, 255, 0, 0, 16, 19},
  { 6, "Atari", true, 0, 255, 0, 20, 23},
  { 7, "Master System", true, 0, 0, 255, 24, 27},
  { 8, "Mega Drive", true, 0, 0, 255, 28, 31},
  { 9, "Saturn", true, 0, 0, 255, 32, 35},
  { 10, "Dreamcast", true, 0, 0, 255, 36, 39},
  { 11, "PlayStation 1", true, 106, 90, 205, 40, 43},
  { 12, "PlayStation 2", true, 106, 90, 205, 44, 49} // only till 47 needed
};

// Global Variables
CRGB leds[LED_NUMBER];
ClickEncoder *encoder;
int16_t encoderLastValue, encoderCurrentValue;
uint8_t gameConsoleActivePort;
uint8_t gameConsoleListSize;

void setup() {
  Serial.begin(115200);
  Serial.println("start setup");
  
  gameConsoleListSize = sizeof(gameConsoleList) / sizeof(gameConsoleList[0]);
  gameConsoleActivePort = 1;
  
  encoderSetup();
  ledSetup();
 
  Serial.println("setup finished");
}

void loop() {
  encoderLoop();


}

void encoderTimerIsr() {
  encoder->service();
}

void encoderSetup() {
  encoder = new ClickEncoder(ENCODER_DATA_PIN, ENCODER_CLOCK_PIN, ENCODER_SWITCH_PIN, 4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(encoderTimerIsr); 
  encoderLastValue = -1;
}

void encoderLoop() {  
  encoderCurrentValue += encoder->getValue();

  if (encoderCurrentValue != encoderLastValue) {
    //Serial.print("encoderCurrentValue: ");
    //Serial.print(encoderCurrentValue);
    //Serial.print(" - encoderLastValue: ");
    //Serial.println(encoderLastValue);
    
    if (encoderLastValue != -1) {
      if (encoderCurrentValue > encoderLastValue) {
        Serial.println("forward");
        if (gameConsoleActivePort >= gameConsoleListSize) {
          gameConsoleActivePort = 1;
        } else {
          gameConsoleActivePort++;
        }
      } else {
        Serial.println("back");
        if (gameConsoleActivePort == 1) {
          gameConsoleActivePort = 12;
        } else {
          gameConsoleActivePort--;
        }
      }
      Serial.print("gameConsoleActivePort: ");
      Serial.println(gameConsoleActivePort);

      ledUpdate();
    }
    
    encoderLastValue = encoderCurrentValue;
    Serial.print("encoderCurrentValue: ");
    Serial.println(encoderCurrentValue);
  }

  ClickEncoder::Button buttonState = encoder->getButton();
  if (buttonState != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (buttonState) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    }
  } 
}

void ledSetup() {
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  //delay(2000);  
  FastLED.addLeds<WS2811, LED_DATA_PIN, BRG>(leds, LED_NUMBER);
  ledUpdate();
}

void ledUpdate() {
  for (int i = 0; i < gameConsoleListSize; i++) {
    //Serial.print("i: ");
    //Serial.println(i);    
    
    CRGB colorToSet;
    if ((i + 1) == gameConsoleActivePort) {
      Serial.print("port matches: ");
      Serial.println(i);    
      colorToSet = CRGB(
        gameConsoleList[i].ledColorRed,
        gameConsoleList[i].ledColorGreen,
        gameConsoleList[i].ledColorBlue
      );      
    } else {
      colorToSet = ledsDefaultColor;
    }

    for (int j = gameConsoleList[i].ledStart; j <= gameConsoleList[i].ledEnd; j++) {
      //Serial.print("j: ");
      //Serial.println(j);      
      
      leds[j] = colorToSet;
    }
  }
  FastLED.show();
}

void testUpdateLeds() {
  leds[0] = CRGB(255,0,0);
  leds[1] = CRGB(255,0,0);
  leds[2] = CRGB(255,0,0);
  leds[3] = CRGB(255,0,0);
  leds[4] = CRGB(255,0,0);
  leds[5] = CRGB(255,0,0);
  leds[6] = CRGB(255,0,0);
  leds[7] = CRGB(255,0,0);
  leds[8] = CRGB(255,0,0);
  leds[9] = CRGB(255,0,0);
  
  leds[10] = CRGB(0,255,0);
  leds[11] = CRGB(0,255,0);
  leds[12] = CRGB(0,255,0);
  leds[13] = CRGB(0,255,0);
  leds[14] = CRGB(0,255,0);
  leds[15] = CRGB(0,255,0);
  leds[16] = CRGB(0,255,0);
  leds[17] = CRGB(0,255,0);
  leds[18] = CRGB(0,255,0);
  leds[19] = CRGB(0,255,0);

  leds[20] = CRGB(0,0,255);
  leds[21] = CRGB(0,0,255);
  leds[22] = CRGB(0,0,255);
  leds[23] = CRGB(0,0,255);
  leds[24] = CRGB(0,0,255);
  leds[25] = CRGB(0,0,255);
  leds[26] = CRGB(0,0,255);
  leds[27] = CRGB(0,0,255);
  leds[28] = CRGB(0,0,255);
  leds[29] = CRGB(0,0,255);

  leds[30] = CRGB(255,0,255);
  leds[31] = CRGB(255,0,255);
  leds[32] = CRGB(255,0,255);
  leds[33] = CRGB(255,0,255);
  leds[34] = CRGB(255,0,255);
  leds[35] = CRGB(255,0,255);
  leds[36] = CRGB(255,0,255);
  leds[37] = CRGB(255,0,255);
  leds[38] = CRGB(255,0,255);
  leds[39] = CRGB(255,0,255);

  leds[40] = CRGB(255,0,255);
  leds[41] = CRGB(255,0,255);
  leds[42] = CRGB(255,0,255);
  leds[43] = CRGB(255,0,255);
  leds[44] = CRGB(255,0,255);
  leds[45] = CRGB(255,0,255);
  leds[46] = CRGB(255,0,255);
  leds[47] = CRGB(255,0,255);
  leds[48] = CRGB(255,0,255);
  leds[49] = CRGB(255,0,255);
  FastLED.show();
}
