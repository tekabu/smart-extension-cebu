#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

#define PZEM_TX 2
#define PZEM_RX 3
#define I2C_SLAVE_ADDRESS 0x08

SoftwareSerial SoftSerial(PZEM_TX, PZEM_RX);
PZEM004Tv30 pzem(SoftSerial);

double voltage = 0;
double current = 0;
double power = 0;
double energy = 0;
double frequency = 0;
double pf = 0;
unsigned long lastMillis = 0;

void requestEvent() {
  char buffer[150];

  if (voltage < 0) {
    snprintf(buffer, sizeof(buffer), "$-1,-1,-1,-1,-1,-1#");
  } else {
    String bufferStr = "$";
    bufferStr += String(voltage, 2);
    bufferStr += ",";
    bufferStr += String(current, 2);
    bufferStr += ",";
    bufferStr += String(power, 2);
    bufferStr += ",";
    bufferStr += String(energy, 2);
    bufferStr += ",";
    bufferStr += String(frequency, 2);
    bufferStr += ",";
    bufferStr += String(pf, 2);
    bufferStr += "#";
    
    snprintf(buffer, sizeof(buffer), bufferStr.c_str());
  }
  Wire.write(buffer);

  Serial.print("Final Formatted Buffer: ");
  Serial.println(buffer);
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