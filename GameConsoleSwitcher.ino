#include <avr/pgmspace.h>
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ### CONFIG LED ###
#define LED_NUMBER 50 // Total number of LED
#define LED_DATA_PIN 6 // Data pin that led data will be written out over
CRGB ledsDefaultColor = CRGB::White;

// ### CONFIG Encoder ###
#define ENCODER_SWITCH_PIN A2
#define ENCODER_CLOCK_PIN A0
#define ENCODER_DATA_PIN A1

// ### CONFIG Display ###
#define OLED_RESET 4 // not used (optional)
uint8_t displayWidth = 128;
uint8_t displayHeight = 32;
//#define DRAW_DELAY 118
//#define D_NUM 47

// ### CONFIG GameConsoles ###
typedef struct
{
	char deviceName[14];
	bool syncStripper;
	int ledColorRed;
	int ledColorGreen;
	int ledColorBlue;
	int ledStart;
	int ledEnd;
}  GameConsole;
GameConsole gameConsoleList[12] = {
	{ "NES", false, 255, 0, 0, 0, 3}, 
	{ "SNES", true, 255, 0, 0, 4, 7},
	{ "N64", true, 255, 0, 0, 8, 11},
	{ "Game Cube", true, 255, 0, 0, 12, 15},
	{ "SNES NTSC", true, 255, 0, 0, 16, 19},
	{ "Atari", true, 0, 255, 0, 20, 23},
	{ "Master System", true, 0, 0, 255, 24, 27},
	{ "Mega Drive", true, 0, 0, 255, 28, 31},
	{ "Saturn", true, 0, 0, 255, 32, 35},
	{ "Dreamcast", true, 0, 0, 255, 36, 39},
	{ "PlayStation 1", true, 106, 90, 205, 40, 43},
	{ "PlayStation 2", true, 106, 90, 205, 44, 49} // only till 47 needed
};

// Global Variables
CRGB leds[LED_NUMBER];
ClickEncoder *encoder;
int16_t encoderLastValue, encoderCurrentValue;
uint8_t gameConsoleActivePort;
uint8_t gameConsoleListSize;
Adafruit_SSD1306 display(OLED_RESET);

void setup() {
	Serial.begin(115200);
	Serial.println(F("start setup"));
  
	gameConsoleListSize = sizeof(gameConsoleList) / sizeof(gameConsoleList[0]);
	gameConsoleActivePort = 1;
  
	encoderSetup();
	ledSetup();
	displaySetup();
 
	Serial.println(F("setup finished"));
}

void loop() {
	encoderLoop();
	displayConsole();
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
		if (encoderLastValue != -1) {
			if (encoderCurrentValue > encoderLastValue) {
				Serial.println(F("forward"));
				if (gameConsoleActivePort >= gameConsoleListSize) {
					gameConsoleActivePort = 1;
				} else {
					gameConsoleActivePort++;
				}
			} else {
				Serial.println(F("back"));
				if (gameConsoleActivePort == 1) {
					gameConsoleActivePort = gameConsoleListSize;
				} else {
					gameConsoleActivePort--;
				}
			}
			Serial.print(F("gameConsoleActivePort: "));
			Serial.println(gameConsoleActivePort);

			ledUpdate();
		}  
		encoderLastValue = encoderCurrentValue;
	}

	ClickEncoder::Button buttonState = encoder->getButton();
	if (buttonState != ClickEncoder::Open) {
		Serial.print(F("Button: "));
		#define VERBOSECASE(label) case label: Serial.println(#label); break;
		switch (buttonState) {
			VERBOSECASE(ClickEncoder::Pressed);
			VERBOSECASE(ClickEncoder::Held)
			VERBOSECASE(ClickEncoder::Released)
			case ClickEncoder::Clicked:
				Serial.println(F("Clicked"));
				encoderSwitchSyncStripper();
				displaySwitchSyncStripper();
				break;

			case ClickEncoder::DoubleClicked:
				Serial.println(F("ClickEncoder::DoubleClicked"));
				encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
				//Serial.print("  Acceleration is ");
				//Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
				break;
		}
	} 
}

void ledSetup() {
	FastLED.addLeds<WS2811, LED_DATA_PIN, BRG>(leds, LED_NUMBER);
	ledUpdate();
}

void ledUpdate() {
	for (int i = 0; i < gameConsoleListSize; i++) {
		//Serial.print("i: ");
		//Serial.println(i);    
    
		CRGB colorToSet;
		if ((i + 1) == gameConsoleActivePort) {  
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

void displaySetup() {
	// initialize I2C addr 0x3C
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.clearDisplay();
}

void displayConsole() {
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);

	char *gameConsoleName = gameConsoleList[gameConsoleActivePort - 1].deviceName;
	display.setCursor(
		displayCalcCenterX(1, gameConsoleName),
		displayCalcCenterY(1, gameConsoleName)
	);
	display.println(gameConsoleName);
  
	display.setCursor(0, 0);
	display.println(gameConsoleActivePort);
	display.display();
}

uint8_t displayCalcCenterX(uint8_t textSize, String stringToShow) {
	uint8_t charWidth = 0;
	switch(textSize) {
		case 1:
			charWidth = 5;
			break;

		case 2:
			charWidth = 10;
			break;

		case 3:
			charWidth = 16;
			break;

		default:
			break;
	}
	return (displayWidth / 2) - ((stringToShow.length() * charWidth) / 2);
}

uint8_t displayCalcCenterY(uint8_t textSize, String stringToShow) {
	uint8_t charHeight = 0;
	switch(textSize) {
		case 1:
			charHeight = 6;
			break;

		case 2:
			charHeight = 13;
			break;

		case 3:
			charHeight = 20;
			break;

		default:
			break;
	}
	return (displayHeight / 2) - (charHeight / 2);
}

void displaySwitchSyncStripper() {
	display.clearDisplay();
	display.setTextColor(WHITE);

	display.setTextSize(1);
	String text1 = "SyncStripper";
	display.setCursor(displayCalcCenterX(1, text1), 3);
	display.println(text1);

	display.setTextSize(2);
	String text2 = gameConsoleList[gameConsoleActivePort - 1].syncStripper ? " ON" : " OFF";
	display.setCursor(displayCalcCenterX(2, text2), 15);
	display.println(text2);

	display.display();
	delay(1000);
}

void encoderSwitchSyncStripper() {
	bool currentState = gameConsoleList[gameConsoleActivePort - 1].syncStripper;
	gameConsoleList[gameConsoleActivePort - 1].syncStripper = !currentState;
}