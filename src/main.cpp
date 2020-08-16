#include "main.h"

// ### CONFIG LED ###
#define LED_NUMBER 48  // Total number of LED
#define LED_DATA_PIN 6 // Data pin that led data will be written out over
CRGB ledsDefaultColor = CRGB::White;

// ### CONFIG Rotary Encoder ###
#define ENCODER_SWITCH_PIN A1 //A2
#define ENCODER_DATA_PIN A2   //A1
#define ENCODER_CLOCK_PIN A3  //A0

// ### CONFIG Display ###
#define OLED_RESET 4 // not used (optional)

uint8_t displayWidth = 128;
uint8_t displayHeight = 32;
//#define DRAW_DELAY 118
//#define D_NUM 47

// ### CONFIG Shift Register ###
#define SHIFT_REGISTER_DS_PIN 8
#define SHIFT_REGISTER_STCP_PIN 9
#define SHIFT_REGISTER_SHCP_PIN 10

bool gameConsoleSwitcherTable[8][3] = {
	/*1*/ {1, 1, 1},
	/*2*/ {1, 1, 0},
	/*3*/ {1, 0, 1},
	/*4*/ {1, 0, 0},
	/*5*/ {0, 1, 1},
	/*6*/ {0, 1, 0},
	/*7*/ {0, 0, 1},
	/*8*/ {0, 0, 0}
};
bool gameConsoleSwitcherOn[2]  = {0, 1};
bool gameConsoleSwitcherOff[2] = {1, 0};

// ### CONFIG GameConsoles ###
struct GameConsole {
	char deviceName[14];
	bool syncStripper;
	uint8_t ledColorRed;
	uint8_t ledColorGreen;
	uint8_t ledColorBlue;
	uint8_t ledStart;
	uint8_t ledEnd;
};

GameConsole gameConsoleList[16] = {
	/* 1*/ {"NES", false, 255, 0, 0, 0, 3},
	/* 2*/ {"SNES", true, 255, 0, 0, 4, 7},
	/* 3*/ {"Wii", false, 255, 0, 0, 8, 11},
	/* 4*/ {"Game Cube", true, 255, 0, 0, 12, 15},
	/* 5*/ {"N64", true, 255, 0, 0, 16, 19},
	/* 6*/ {"SNES NTSC", true, 0, 255, 0, 20, 23},

	/* 7*/ {"Master System", false, 0, 0, 255, 44, 47},
	/* 8*/ {"Mega Drive", true, 0, 0, 255, 40, 43},
	/* 9*/ {"Saturn", true, 0, 0, 255, 36, 39},
	/*10*/ {"Dreamcast", true, 0, 0, 255, 32, 35},
	/*11*/ {"PlayStation 1", true, 106, 90, 205, 28, 31},
	/*12*/ {"PlayStation 2", true, 106, 90, 205, 24, 27},

	/*13*/ {"Console 13", true, 0, 0, 0, 0, 0},
	/*14*/ {"Console 14", true, 0, 0, 0, 0, 0},
	/*15*/ {"Console 15", true, 0, 0, 0, 0, 0},
	/*16*/ {"Console 16", true, 0, 0, 0, 0, 0}
};

// Global Variables
CRGB leds[LED_NUMBER];
ClickEncoder *encoder;
int16_t encoderLastValue, encoderCurrentValue;
uint8_t gameConsoleActivePort;
uint8_t gameConsoleListSize;
Adafruit_SSD1306 display(OLED_RESET);
//Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
int16_t lastConfigUpdate;

void setup() {
	//Serial.begin(115200);
	Serial.begin(9600);
	Serial.println(F("start setup"));

	gameConsoleListSize = sizeof(gameConsoleList) / sizeof(gameConsoleList[0]);
	gameConsoleActivePort = 1;

	encoderSetup();
	ledSetup();
	displaySetup();

shiftRegisterSetup();

	//EEPROM.write(0, 3);
	//uint8_t savedValue = EEPROM.read(1);

	Serial.println(F("setup finished"));
}

void loop() {
	encoderLoop();
	displayGameConsole();
	//Serial.println(millis());
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

			shiftRegisterUpdate();

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
			VERBOSECASE(ClickEncoder::Closed)
			VERBOSECASE(ClickEncoder::Open)
			VERBOSECASE(ClickEncoder::Pressed)
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
		if (gameConsoleList[i].ledEnd == 0) {
			continue;
		}

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

void displayGameConsole() {
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);

	char *gameConsoleName = gameConsoleList[gameConsoleActivePort - 1].deviceName;
	display.setCursor(
		displayCalcCenterX(1, gameConsoleName),
		displayCalcCenterY(1, gameConsoleName));
	display.println(gameConsoleName);

	display.setCursor(0, 0);
	display.println(gameConsoleActivePort);
	display.display();
}

uint8_t displayCalcCenterX(uint8_t textSize, String stringToShow) {
	uint8_t charWidth = 0;
	switch (textSize) {
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
	switch (textSize) {
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
	shiftRegisterUpdate();
}

void shiftRegisterSetup() {
	pinMode(SHIFT_REGISTER_DS_PIN, OUTPUT);
	pinMode(SHIFT_REGISTER_STCP_PIN, OUTPUT);
	pinMode(SHIFT_REGISTER_SHCP_PIN, OUTPUT);
	shiftRegisterUpdate();
}

void shiftRegisterUpdate() {
	// Write Relais Shift Register
	digitalWrite(SHIFT_REGISTER_STCP_PIN, LOW);
	Serial.print(F("ShiftRegister: "));

	// Game Consoles, determine which switcher is ON and OFF
	bool (*currentSwitchTable)[3];
	bool (*switch1Status)[2];
	bool (*switch2Status)[2];
	if (gameConsoleActivePort <= 8) {
		currentSwitchTable = &gameConsoleSwitcherTable[gameConsoleActivePort - 1];
		switch1Status = &gameConsoleSwitcherOn;
		switch2Status = &gameConsoleSwitcherOff;
	} else {
		currentSwitchTable = &gameConsoleSwitcherTable[gameConsoleActivePort - 1 - 8];
		switch1Status = &gameConsoleSwitcherOff;
		switch2Status = &gameConsoleSwitcherOn;
	}

	// Fill unused ports   
	// Register 18-24 (7)
	for (int j = 1; j <= 7; j++) {
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW); // open shift register
		digitalWrite(SHIFT_REGISTER_DS_PIN, false);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);  // close shift register
		Serial.print(0);
	}

	// Sync Stripper
	// Register 17 (1)
	digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW);
	digitalWrite(SHIFT_REGISTER_DS_PIN, gameConsoleList[gameConsoleActivePort - 1].syncStripper);
	digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);
	Serial.print(gameConsoleList[gameConsoleActivePort - 1].syncStripper ? 1 : 0);
	Serial.print(F("-"));

	// Fill unused ports
	// Register 8-16 (9)
	for (int j = 1; j <= 9; j++) {
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW); // open shift register
		digitalWrite(SHIFT_REGISTER_DS_PIN, false);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);  // close shift register
		Serial.print(0);
	}

	// Switch Table
	// Register 5-7 (3)
	Serial.print(F("-T:"));
	for (int  i = 2; i >= 0; i--) {
		bool shiftRegisterBit = (*currentSwitchTable)[i];
		Serial.print(shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW);
		digitalWrite(SHIFT_REGISTER_DS_PIN, shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);
	}

	// Game Switcher 2
	// Register 3-4 (2)
	Serial.print(F("-S2:"));
	for (uint8_t i = 0; i < (sizeof(*switch2Status) / sizeof(*switch2Status[0])); i++) {
		bool shiftRegisterBit = (*switch2Status)[i];
		Serial.print(shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW);
		digitalWrite(SHIFT_REGISTER_DS_PIN, shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);
	}

	// Game Switcher 1
	// Register 1-2 (2)
	Serial.print(F("-S1:"));
	for (uint8_t i = 0; i < (sizeof(*switch1Status) / sizeof(*switch1Status[0])); i++) {
		bool shiftRegisterBit = (*switch1Status)[i];
		Serial.print(shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, LOW);
		digitalWrite(SHIFT_REGISTER_DS_PIN, shiftRegisterBit);
		digitalWrite(SHIFT_REGISTER_SHCP_PIN, HIGH);
	}

	// save to shift register
	Serial.println();
	digitalWrite(SHIFT_REGISTER_STCP_PIN, HIGH);
}

// @TODO
void configSave() {
}

// @TODO
void configRead() {
}
