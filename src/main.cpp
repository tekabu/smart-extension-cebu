#include <Arduino.h>
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>

#define PZEM_SERIAL1 Serial1
#define PZEM_SERIAL2 Serial2
#define THERMISTOR_CHANNEL A0

PZEM004Tv30 pzems[] = { PZEM004Tv30(PZEM_SERIAL1), PZEM004Tv30(PZEM_SERIAL2) };
LiquidCrystal_I2C lcd(0x27, 20, 4);

double voltage[] = {0, 0};
double current[] = {0, 0};
double power[] = {0, 0};
double energy[] = {0, 0};
double frequency[] = {0, 0};
double pf[] = {0, 0};
double temperature = 0;
unsigned long lastMillis = 0;
unsigned long nextReadMillis = 3000;

void setup() {
  Serial.begin(9600);
  PZEM_SERIAL1.begin(9600);
  PZEM_SERIAL2.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("Hello, world!");
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
    Serial.print(F("PZEM")); Serial.print(i + 1); Serial.println(F(" -> "));
    Serial.print(F("Voltage: ")); Serial.print(voltage[i]); Serial.print(F(", "));
    Serial.print(F("Current: ")); Serial.print(current[i]); Serial.print(F(", "));
    Serial.print(F("Power: ")); Serial.print(power[i]); Serial.print(F(", "));
    Serial.print(F("Energy: ")); Serial.print(energy[i]); Serial.print(F(", "));
    Serial.print(F("Frequency: ")); Serial.print(frequency[i]); Serial.print(F(", "));
    Serial.print(F("PF: ")); Serial.println(pf[i]);
  }
  Serial.print(F("Temperature: ")); Serial.println(temperature);
}

void display_pzem_lcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PZEM1 V:"));
  lcd.print(voltage[0]);
  lcd.setCursor(0, 1);
  lcd.print(F("I:"));
  lcd.print(current[0]);
  lcd.print(F(" P:"));
  lcd.print(power[0]);

  lcd.setCursor(0, 2);
  lcd.print(F("PZEM2 V:"));
  lcd.print(voltage[1]);
  lcd.setCursor(0, 3);
  lcd.print(F("I:"));
  lcd.print(current[1]);
  lcd.print(F(" P:"));
  lcd.print(power[1]);
  lcd.print(F(" T:"));
  lcd.print(temperature);
}

void read_thermistor() {
  int thermistor_adc_val;
  double output_voltage;
  double thermistor_resistance;
  double therm_res_ln;
  thermistor_adc_val = analogRead(THERMISTOR_CHANNEL);
  output_voltage = ( (thermistor_adc_val * 5.0) / 1023.0 );
  thermistor_resistance = ( ( 5 * ( 10.0 / output_voltage ) ) - 10 ); /* Resistance in kilo ohms */
  thermistor_resistance = thermistor_resistance * 1000 ; /* Resistance in ohms   */
  therm_res_ln = log(thermistor_resistance);
  temperature = ( 1 / ( 0.001129148 + ( 0.000234125 * therm_res_ln ) + ( 0.0000000876741 * therm_res_ln * therm_res_ln * therm_res_ln ) ) ); /* Temperature in Kelvin */
  temperature = temperature - 273.15; /* Temperature in degree Celsius */
}

void loop() {
  if (millis() - lastMillis >= nextReadMillis) {
    read_pzem();
    read_thermistor();
    display_pzem();
    display_pzem_lcd();
    lastMillis = millis();
  }
}