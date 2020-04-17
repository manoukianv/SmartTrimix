#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <RunningAverage.h>
#include <X9C.h>

#define GAIN_FOUR_VOLTAGE  0.03125
#define GAIN_EIGHT_VOLTAGE 0.015625

#define INC 2   // D1 Mini D4 - pulled up in H/W (10k) ->  chip pin 1
#define UD 3   // D1 Mini D8                          ->  chip pin 2
#define CS 4   // D1 Mini D0 - pulled up in H/W (10k) ->  2nd chip pin 7

/* Constructor */
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_1_4W_SW_I2C u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
Adafruit_ADS1115 ads;

X9C pot;                           // create a pot controller

enum AnalyzerState {
  started,
  calibrate,
  sensor_error,
  ready,
  technical_details,
  battery_error
};

float calibrationO2, calibrationHe, calibFullHe = 600, minHeGateVoltage = 999999;
int potValue, minPotValue;
AnalyzerState analyzerState = started;
RunningAverage ra_O2(10);       // moyennage O2 sur 10 valeurs
RunningAverage ra_He(10);       // moyennage He sur 10 valeurs
