#include <Arduino.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

#define PZEM_TX 2
#define PZEM_RX 3

SoftwareSerial SoftSerial(PZEM_TX, PZEM_RX);
PZEM004Tv30 pzem(SoftSerial);

void setup() {
  Serial.begin(9600);
  SoftSerial.begin(9600);
}

void loop() {
  // Read the data from the sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Check if the data is valid
  if(isnan(voltage)){
      Serial.println("Error reading voltage");
  } else if (isnan(current)) {
      Serial.println("Error reading current");
  } else if (isnan(power)) {
      Serial.println("Error reading power");
  } else if (isnan(energy)) {
      Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
  } else {

      // Print the values to the Serial console
      Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
      Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
      Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
      Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
      Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
      Serial.print("PF: ");           Serial.println(pf);
  }

  Serial.println();
  delay(2000);
}