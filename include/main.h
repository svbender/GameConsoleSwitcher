// Includes
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Declares
void encoderSetup();
void ledSetup();
void displaySetup();
void shiftRegisterSetup();
void encoderLoop();
void displayGameConsole();
void shiftRegisterUpdate();
uint8_t displayCalcCenterX(uint8_t textSize, String stringToShow);
uint8_t displayCalcCenterY(uint8_t textSize, String stringToShow);
void ledUpdate();
void encoderSwitchSyncStripper();
void displaySwitchSyncStripper();