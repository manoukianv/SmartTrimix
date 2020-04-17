#include "main.h"
#define debug true

/* u8g2.begin() is required and will sent the setup/init sequence to the display */
void setup(void) {
  analyzerState = started;

  u8g2.begin();
  #ifdef debug
  Serial.begin(115200);
  Serial.println("screen init");
  #endif

  ads.begin();
  delay(500);
  ads.setGain(GAIN_FOUR); // 4x gain 1 bit = 0.03125mV
  #ifdef debug
  Serial.println("ads init");
  #endif

  pot.begin(CS,INC,UD);
  potValue = 50;
  pot.setPot(potValue,false);
  #ifdef debug
  Serial.println("pot init : 50");
  #endif

}

void drawAnalysis(float _o2, float _he, float _bat, float _mod) {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 63, 16);
    u8g2.drawBox(64, 0, 64, 16);
    u8g2.setFont(u8g2_font_courR08_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr(1, 11, "MOD");
    u8g2.drawStr(109, 11, "BAT");
    u8g2.setFont(u8g2_font_courB10_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr(30, 13, String(_mod,0).c_str());
    u8g2.drawStr(70, 13, String(_bat,0).c_str());
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_courB18_tr); //ncenB18
    u8g2.drawStr(10, 37, "O2");
    u8g2.drawStr(10, 64, "He");
    u8g2.drawStr(50, 37, String(_o2, 1).c_str());
    u8g2.drawStr(50, 64, String(_he, 1).c_str());

    u8g2.drawBox(120, 22, 6, 44);
    u8g2.setDrawColor(0);
    u8g2.drawBox(122, 24, 2, 15);

  } while ( u8g2.nextPage() );
}

void drawDetail() {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_courR08_tr);
    u8g2.drawStr(0, 8 , "BAT:");
    u8g2.drawStr(30, 8, "7.12");
    u8g2.drawStr(0, 28, "Vo2:       > C:");
    u8g2.drawStr(30, 28, "38.2");
    u8g2.drawStr(90, 28, "35.1");
    u8g2.drawStr(0, 38, "VHe:       > C:");
    u8g2.drawStr(30, 38, "245.4");
    u8g2.drawStr(90, 38, "666.2");
    u8g2.drawStr(0, 48, "VPt:");
    u8g2.drawStr(30, 48, "0.122");
  } while ( u8g2.nextPage() );
}

void drawCalibration(float o2Voltage, float heVoltage) {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(3, 0, 122, 16, 5);
    u8g2.setFont(u8g2_font_courB10_tr);
    u8g2.drawStr(14, 12, "SmartTrimix");
    u8g2.drawStr(14, 30, "Calibration");

    u8g2.setFont(u8g2_font_courR08_tr);
    u8g2.drawStr(0, 52, "O2:");
    if (!isnan(o2Voltage)) u8g2.drawStr(17, 52, String(o2Voltage,2).c_str());
    u8g2.drawStr(74, 52, "He:");
    if (!isnan(heVoltage)) u8g2.drawStr(74 + 17, 52, String(heVoltage,2).c_str());
    u8g2.drawStr(74, 62, "Pot:");
    u8g2.drawStr(74 + 34, 62, String(potValue).c_str());
  } while ( u8g2.nextPage() );
}

void drawSensorError() {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(3, 0, 122, 16, 5);
    u8g2.setFont(u8g2_font_courB10_tr);
    u8g2.drawStr(14, 12, "SmartTrimix");
    u8g2.drawStr(14, 38, "Sensor Error !");
  } while ( u8g2.nextPage() );
}

void readValue() {
  //ads.setGain(GAIN_EIGHT);
  ads.setGain(GAIN_FOUR);
  int16_t adc01 = ads.readADC_Differential_0_1(); //* GAIN_EIGHT_VOLTAGE;
  ra_O2.addValue(adc01 * GAIN_FOUR_VOLTAGE);

  //ads.setGain(GAIN_FOUR);
  int16_t adc23 = ads.readADC_Differential_2_3();
  ra_He.addValue(adc23 * GAIN_FOUR_VOLTAGE);

  #ifdef debug
  Serial.print(adc01);
  Serial.print("   (");
  Serial.print(adc01 * GAIN_FOUR_VOLTAGE);
  Serial.print("mV)    He <-> O2    (");
  Serial.print(adc23 * GAIN_FOUR_VOLTAGE);
  Serial.print("mV)   ");
  Serial.print(adc23);
  Serial.print(" pot ");
  Serial.println(potValue);
  #endif
}

/* draw something on the display with the `firstPage()`/`nextPage()` loop*/
void loop(void) {

  readValue();

  switch (analyzerState) {
    case started :
      analyzerState = calibrate;
      break;
    case calibrate : {
        float errorO2 = ra_O2.getStandardDeviation();
        float errorHe = ra_He.getStandardDeviation();
        float he = ra_He.getAverage();
        bool endCalibration = false;

        if (errorHe<0.2 && ra_He.getCount()>9) {   // Wait He readStabilisation
          if (abs(he) < minHeGateVoltage) { // if we have a min value for the gate, store it
            minHeGateVoltage = abs(he);
            minPotValue = potValue;
          }
          if (he<0 && potValue > 0) {  // try to get a new minimum and check pot range
            potValue += 1;
          } else if (he>0 && potValue <100) {
            potValue -=1;
          }

          endCalibration = potValue==minPotValue && minHeGateVoltage<1.5 && errorO2<0.05;
          pot.setPot(potValue, false);

          ra_He.clear();
        }

        drawCalibration(ra_O2.getAverage(), ra_He.getAverage());
        if (!endCalibration) { // wait the next calibration
          delay(1000);
        } else {
          analyzerState = ready;

          potValue = minPotValue;
          pot.setPot(potValue, false);
          for (int i=0; i<10; i++) {
            readValue();
            delay(200);
          }
          #ifdef debug
          Serial.print("Selected pot ");
          Serial.print(potValue);
          Serial.print(" init value ");
          Serial.println(ra_He.getAverage());
          #endif
          calibrationO2 = ra_O2.getAverage();
          calibrationHe = ra_He.getAverage();
        }
      }
      break;
    case ready : {
        float o2 = 20.9 * ra_O2.getAverage() / calibrationO2;
        float he = 100 * (ra_He.getAverage()-calibrationHe) / calibFullHe;
        if (o2 > 110 ) {//|| he > 110) {
          analyzerState = sensor_error;
        }

        if (he > 50) {
          he = he * (1 + (he - 50) * 0.4 / 100);
        }
        else if (he < 2) {
          he=0;
        }
        else if (he < 0) {
          analyzerState = sensor_error;
        }

        float mod = 10 * ((100 * 1.6 / o2) - 1);
        float ean = 8.42;
        drawAnalysis(o2, he, ean, mod);
        delay(100);
      }
      break;
    case sensor_error :
      drawSensorError();
      break;
    case technical_details:
      drawDetail();
      delay(5000);
      break;
    case battery_error :
      break;
    default :
      break;
  }

}
