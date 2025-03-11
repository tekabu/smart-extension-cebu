#include <Arduino.h>
#include <SoftwareSerial.h>

#define PZEM_TX 2
#define PZEM_RX 3

SoftwareSerial SoftSerial(PZEM_TX, PZEM_RX);

void setup() {
  Serial.begin(9600);
  SoftSerial.begin(9600);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    SoftSerial.write(c);
  }
  while (SoftSerial.available()) {
    char c = SoftSerial.read();
    Serial.write(c);
  }
}