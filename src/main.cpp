#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

#define PZEM_TX 2
#define PZEM_RX 3
#define I2C_SLAVE_ADDRESS 0x08

SoftwareSerial SoftSerial(PZEM_TX, PZEM_RX);
PZEM004Tv30 pzem(SoftSerial);

float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float frequency = 0;
float pf = 0;
unsigned long lastMillis = 0;

void requestEvent() {
  char buffer[100];

  if (voltage < 0) {
    snprintf(buffer, sizeof(buffer), "$-1,-1,-1,-1,-1,-1#");
  } else {
    char buffer[100];
    char voltageStr[10];
    char currentStr[10];
    char powerStr[10];
    char energyStr[10];
    char frequencyStr[10];
    char pfStr[10];
    
    dtostrf(voltage, 6, 2, voltageStr); // width: 6, precision: 2
    dtostrf(current, 6, 2, currentStr);
    dtostrf(power, 6, 2, powerStr);
    dtostrf(energy, 6, 2, energyStr);
    dtostrf(frequency, 6, 2, frequencyStr);
    dtostrf(pf, 6, 2, pfStr);
    
    snprintf(buffer, sizeof(buffer), "$%s,%s,%s,%s,%s,%s#",
             voltageStr, currentStr, powerStr, energyStr, frequencyStr, pfStr);
  }
  
  Wire.write(buffer);
}

void setup() {
  Serial.begin(9600);
  SoftSerial.begin(9600);
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
}

void loop() {
  if (millis() - lastMillis >= 2000) {
    voltage = pzem.voltage();
    if (isnan(voltage)) voltage = -1;

    current = pzem.current();
    if (isnan(current)) current = -1;

    power = pzem.power();
    if (isnan(power)) power = -1;

    energy = pzem.energy();
    if (isnan(energy)) energy = -1;

    frequency = pzem.frequency();
    if (isnan(frequency)) frequency = -1;

    pf = pzem.pf();
    if (isnan(pf)) pf = -1;

    lastMillis = millis();

    Serial.print("Voltage: "); Serial.print(voltage); Serial.print(", ");
    Serial.print("Current: "); Serial.print(current); Serial.print(", ");
    Serial.print("Power: "); Serial.print(power); Serial.print(", ");
    Serial.print("Energy: "); Serial.print(energy); Serial.print(", ");
    Serial.print("Frequency: "); Serial.print(frequency); Serial.print(", ");
    Serial.print("PF: "); Serial.println(pf);
  }
}