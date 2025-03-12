#include <Arduino.h>
#include <PZEM004Tv30.h>

#define PZEM_SERIAL1 Serial1
#define PZEM_SERIAL2 Serial2

PZEM004Tv30 pzems[] = { PZEM004Tv30(PZEM_SERIAL1), PZEM004Tv30(PZEM_SERIAL2) };

double voltage[] = {0, 0};
double current[] = {0, 0};
double power[] = {0, 0};
double energy[] = {0, 0};
double frequency[] = {0, 0};
double pf[] = {0, 0};
unsigned long lastMillis = 0;
unsigned long nextReadMillis = 3000;

void setup() {
  Serial.begin(9600);
  PZEM_SERIAL1.begin(9600);
  PZEM_SERIAL2.begin(9600);
}

void read_pzem() {
  for (int i = 0; i < 2; i++) {
    voltage[i] = pzems[i].voltage(); 
    current[i] = pzems[i].current(); 
    power[i] = pzems[i].power(); 
    energy[i] = pzems[i].energy(); 
    frequency[i] = pzems[i].frequency(); 
    pf[i] = pzems[i].pf(); 
    if (isnan(voltage[i])) voltage[i] = -1;
    if (isnan(current[i])) current[i] = -1;
    if (isnan(power[i])) power[i] = -1; 
    if (isnan(energy[i])) energy[i] = -1;
    if (isnan(frequency[i])) frequency[i] = -1;
    if (isnan(pf[i])) pf[i] = -1;
  }
}

void display_pzem() {
  for (int i = 0; i < 2; i++) {
    Serial.print("PZEM"); Serial.print(i + 1); Serial.println(" -> ");
    Serial.print("Voltage: "); Serial.print(voltage[i]); Serial.print(", ");
    Serial.print("Current: "); Serial.print(current[i]); Serial.print(", ");
    Serial.print("Power: "); Serial.print(power[i]); Serial.print(", ");
    Serial.print("Energy: "); Serial.print(energy[i]); Serial.print(", ");
    Serial.print("Frequency: "); Serial.print(frequency[i]); Serial.print(", ");
    Serial.print("PF: "); Serial.println(pf[i]);
  }
}

void loop() {
  if (millis() - lastMillis >= nextReadMillis) {
    read_pzem();
    display_pzem();
    lastMillis = millis();
  }
}