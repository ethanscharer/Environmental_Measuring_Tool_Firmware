#include <math.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "rgb_lcd.h"
#include "Seeed_MCP9808.h"
#include "DHT.h"

#define sensor s_serial

const int buttonPin = 5;
const int ledPin =  13;
const int humidityPin =  A0;
const int pinCO2Rx = 3;
const int pinCO2Tx = 2;

const unsigned char cmd_get_sensor[] = {
  0xff, 0x01, 0x86, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x79
};
unsigned char dataRevice[9];

int buttonState = 0;
int mode = 0;
int co2temperature;
int co2ppm;

SoftwareSerial co2Sensor(pinCO2Tx, pinCO2Rx);
rgb_lcd lcd;
DHT dht(humidityPin, DHT11);
MCP9808 tempSensor;
/*
   Modes:
   0 = CO2
   1 = Temperature
   2 = Humidity
*/
int modesAmount = 3;
double startTime;
double heatingTime = 180000;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  co2Sensor.begin(9600);

  lcd.begin(16, 2);
  lcd.noCursor();
  lcdWrite("Starting...", "");

  tempSensor.init();
  dht.begin();

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(humidityPin, INPUT);

  startTime = millis();
}

void loop() {
  if (checkButton()) {
    switchMode();
    lcdWrite("Switching...", "");
    while (checkButton());
  }
  if (mode == 0) {
    if (co2DataReceive()) {
      double actualTime = millis();
      if (actualTime - startTime >= heatingTime) {
        String co2 = String(co2ppm);
        lcdWrite("CO2", co2 + "ppm");
      } else {
        String diff = String(round((heatingTime - (actualTime - startTime)) / 1000.0));
        lcdWrite("CO2", "Heating... " + diff + "s");
      }
    } else {
      lcdWrite("CO2", "Error");
    }
  } else if (mode == 1) {
    float temp = 0;
    tempSensor.get_temp(&temp);
    String tempString = cFloat(temp);
    lcdWrite("Temperature", tempString + (char)223 + "C");
  } else if (mode == 2) {
    float humidity = dht.readHumidity();
    String humidityString = cFloat(humidity);
    lcdWrite("Humidity", humidityString + "%");
  }
}

void switchMode() {
  mode++;
  if (mode >= modesAmount)
    mode = 0;
}

boolean checkButton() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    return true;
  } else {
    return false;
  }
}

void lcdWrite(String s1, String s2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(s1);
  lcd.setCursor(0, 1);
  lcd.print(s2);
  delay(500);
}

String cFloat(float val) {
  String valueString = "";
  char buff[10];
  dtostrf(val, 2, 2, buff);  //4 is mininum width, 6 is precision
  valueString += buff;
  return valueString;
}

bool co2DataReceive(void) {
  byte data[9];
  int i = 0;

  for (i = 0; i < sizeof(cmd_get_sensor); i++) {
    co2Sensor.write(cmd_get_sensor[i]);
  }
  Serial.flush();

  if (co2Sensor.available()) {
    while (co2Sensor.available()) {
      for (int i = 0; i < 9; i++) {
        data[i] = co2Sensor.read();
      }
    }
  }

  byte checksum = 0 ;
  for (int j = 1; j < 8; j++) {
    checksum += data[j];
  }
  checksum = 0xff - checksum;
  checksum += 1;

  if (checksum != data[8]) {
    return false;
  }
  if (data[0] != 0xFF) {
    return false;
  }
  if (data[1] != 0x86) {
    return false;
  }
  co2ppm = (int)data[2] * 256 + (int)data[3];
  co2temperature = (int)data[4] - 40;
  return true;
}
