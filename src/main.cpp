#include <Arduino.h>
#include <OneButton.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define PZEM_SERIAL1 Serial1
#define PZEM_SERIAL2 Serial2
#define THERMISTOR_CHANNEL1 0
#define THERMISTOR_CHANNEL2 1
#define BUTTON1 8
#define BUTTON2 9
#define BUTTON3 10
#define BUTTON4 11
#define BUTTON5 12
#define RELAY1 6
#define RELAY2 7
#define LED1 4
#define LED2 5

#define FUNC_NORMAL 0
#define FUNC_PZEM1 1
#define FUNC_PZEM2 2
#define LEVEL_SELECT_FUNC 0
#define LEVEL_SELECT_PARAM1 1
#define LEVEL_SELECT_PARAM2 2
#define PARAM_VOLTAGE 0
#define PARAM_CURRENT 1
#define PARAM_POWER 2
#define PARAM_ENERGY 3
#define PARAM_TEMPERATURE 4
#define PARAM_ALARM 5

PZEM004Tv30 pzems[] = {PZEM004Tv30(PZEM_SERIAL1), PZEM004Tv30(PZEM_SERIAL2)};
LiquidCrystal_I2C lcd(0x27, 20, 4);

OneButton button1(BUTTON1, false);
OneButton button2(BUTTON2, false);
OneButton button3(BUTTON3, false);
OneButton button4(BUTTON4, false);
OneButton button5(BUTTON5, false);

double voltage[] = {0, 0};
double current[] = {0, 0};
double power[] = {0, 0};
double energy[] = {0, 0};
double frequency[] = {0, 0};
double pf[] = {0, 0};
float temperature[] = {0, 0};
unsigned long lastMillis = 0;
unsigned long nextReadMillis = 3000;
unsigned long lastSendSettingsMillis = 0;
unsigned long nextSendSettingsMillis = 5000;

String lcd_empty_row = "                    ";
String esp_str = "";

// Constants for the thermistor (for a 10k NTC thermistor)
const float referenceVoltage = 5.0;  // Arduino reference voltage
const int nominalResistance = 10000; // 10k ohms
const int nominalTemperature = 25;   // 25°C is the nominal temperature for the thermistor
const int bCoefficient = 3950;       // B coefficient for the thermistor (often provided in datasheet)

int function_index = FUNC_NORMAL;
int level_index = LEVEL_SELECT_FUNC;
int param_index = PARAM_VOLTAGE;

unsigned int th_voltage[] = {0, 0};
unsigned int th_current[] = {0, 0};
unsigned int th_power[] = {0, 0};
unsigned int th_energy[] = {0, 0};
unsigned int th_temperature[] = {0, 0};
unsigned int th_alarm[] = {0, 0};

int relay_pins[] = {RELAY1, RELAY2};
int led_pins[] = {LED1, LED2};
int relay_state[] = {HIGH, HIGH};
int led_state[] = {LOW, LOW};

void add_subtract(int val)
{
  if (level_index == LEVEL_SELECT_PARAM1 || level_index == LEVEL_SELECT_PARAM2)
  {
    lcd.setCursor(0, 2);
    lcd.print(lcd_empty_row);

    int starting_index = 0;
    if (level_index == LEVEL_SELECT_PARAM2) starting_index = 10;

    Serial.print("Select parameters ");
    Serial.print(level_index);
    Serial.print(", starting index ");
    Serial.print(starting_index);
    Serial.println();

    if (param_index == PARAM_VOLTAGE)
    {
      if (th_voltage[level_index - 1] >= 0 && th_voltage[level_index - 1] < 255)
      {
        th_voltage[level_index - 1] += val;
        EEPROM.write(starting_index + 1, th_voltage[level_index - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_voltage[level_index - 1]));
    }
    else if (param_index == PARAM_CURRENT)
    {
      if (th_current[level_index - 1] >= 0 && th_current[level_index - 1] < 255)
      {
        th_current[level_index - 1] += val;
        EEPROM.write(starting_index + 2, th_current[level_index - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_current[level_index - 1]));
    }
    else if (param_index == PARAM_POWER)
    {
      if (th_power[level_index - 1] >= 0 && th_power[level_index - 1] < 255)
      {
        th_power[level_index - 1] += val;
        EEPROM.write(starting_index + 3, th_power[level_index - 1]);
      }
      lcd.setCursor(0, 2);
      float _pow = th_power[level_index - 1] * 100.0;
      lcd.print(String(_pow, 2));
    }
    else if (param_index == PARAM_ENERGY)
    {
      Serial.print("Current energy: ");
      Serial.println(th_energy[level_index - 1]);
      if (th_energy[level_index - 1] >= 0 && th_energy[level_index - 1] < 255)
      {
        th_energy[level_index - 1] += val;
        Serial.print("New energy: ");
        Serial.println(th_energy[level_index - 1]);
        EEPROM.write(starting_index + 4, th_energy[level_index - 1]);
      }
      lcd.setCursor(0, 2);
      float _ene = th_energy[level_index - 1] / 100.0;
      Serial.print("Display energy: ");
      Serial.println(String(_ene, 2));
      lcd.print(String(_ene, 2));
    }
    else if (param_index == PARAM_TEMPERATURE)
    {
      if (th_temperature[level_index - 1] >= 0 && th_temperature[level_index - 1] < 255)
      {
        th_temperature[level_index - 1] += val;
        EEPROM.write(starting_index + 5, th_temperature[level_index - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_temperature[level_index - 1]));
    }
    else if (param_index == PARAM_ALARM)
    {
      if (th_alarm[level_index - 1] == 1)
      {
        th_alarm[level_index - 1] = 0;
      }
      else
      {
        th_alarm[level_index - 1] = 1;
      }
      EEPROM.write(starting_index + 6, th_alarm[level_index - 1]);
      lcd.setCursor(0, 2);
      lcd.print(th_alarm[level_index - 1] == 1 ? "ON" : "OFF");
    }
  }
}

void click1()
{
  Serial.println(F("Button 1"));
  if (level_index == LEVEL_SELECT_FUNC)
  {
    if (function_index == FUNC_NORMAL)
    {
      Serial.println(F("Set parameters 1"));
      function_index = FUNC_PZEM1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("SET SOCKET 1"));
      lcd.setCursor(0, 1);
      lcd.print(F("PARAMETERS"));
    }
    else if (function_index == FUNC_PZEM1)
    {
      Serial.println(F("Set parameters 2"));
      function_index = FUNC_PZEM2;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("SET SOCKET 2"));
      lcd.setCursor(0, 1);
      lcd.print(F("PARAMETERS"));
    }
    else
    {
      Serial.println(F("Normal function"));
      function_index = FUNC_NORMAL;
    }
  }
  else if (level_index == LEVEL_SELECT_PARAM1 || level_index == LEVEL_SELECT_PARAM2)
  {
    lcd.clear();
    lcd.setCursor(0, 0);

    if (level_index == LEVEL_SELECT_PARAM1)
    {
      lcd.print(F("SET SOCKET 1"));
    }
    else
    {
      lcd.print(F("SET SOCKET 2"));
    }

    lcd.setCursor(0, 1);
    
    if (param_index == PARAM_VOLTAGE)
    {
      param_index = PARAM_CURRENT;
      lcd.print(F("CURRENT"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_current[level_index - 1]));
    }
    else if (param_index == PARAM_CURRENT)
    {
      param_index = PARAM_POWER;
      lcd.print(F("POWER"));
      lcd.setCursor(0, 2);
      double _pow = th_power[level_index - 1] * 100.0;
      lcd.print(String(_pow, 2));
    }
    else if (param_index == PARAM_POWER)
    {
      param_index = PARAM_ENERGY;
      lcd.print(F("ENERGY"));
      lcd.setCursor(0, 2);
      double _ene = th_energy[level_index - 1] / 100.0;
      lcd.print(String(_ene, 2));
    }
    else if (param_index == PARAM_ENERGY)
    {
      param_index = PARAM_TEMPERATURE;
      lcd.print(F("TEMPERATURE"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_temperature[level_index - 1]));
    }
    else if (param_index == PARAM_TEMPERATURE)
    {
      param_index = PARAM_ALARM;
      lcd.print(F("ALARM"));
      lcd.setCursor(0, 2);
      lcd.print(th_alarm[level_index - 1] == 1 ? "ON" : "OFF");
    }
    else
    {
      param_index = PARAM_VOLTAGE;
      lcd.print(F("VOLTAGE"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_voltage[level_index - 1]));
    }
  }
}

void click2()
{
  Serial.println(F("Button 2"));
  if (level_index == LEVEL_SELECT_FUNC)
  {
    lcd.clear();
    lcd.setCursor(0, 0);

    if (function_index == FUNC_PZEM1)
    {
      level_index = LEVEL_SELECT_PARAM1;
      lcd.print(F("SET SOCKET 1"));
    }
    else if (function_index == FUNC_PZEM2)
    {
      level_index = LEVEL_SELECT_PARAM2;
      lcd.print(F("SET SOCKET 2"));
    }
    lcd.setCursor(0, 1);
    lcd.print(F("VOLTAGE"));
    lcd.setCursor(0, 2);
    lcd.print(String(th_voltage[level_index - 1]));
  }
}

void click3()
{
  Serial.println(F("Button 3"));
  add_subtract(1);
}

void click4()
{
  Serial.println(F("Button 4"));
  add_subtract(-1);
}

void click5()
{
  function_index = FUNC_NORMAL;
  level_index = LEVEL_SELECT_FUNC;
}

void read_pzem()
{
  for (int i = 0; i < 2; i++)
  {
    voltage[i] = pzems[i].voltage();
    current[i] = pzems[i].current();
    power[i] = pzems[i].power();
    energy[i] = pzems[i].energy();
    frequency[i] = pzems[i].frequency();
    pf[i] = pzems[i].pf();
    
    if (isnan(voltage[i]))
      voltage[i] = -1;
    if (isnan(current[i]))
      current[i] = -1;
    if (isnan(power[i]))
      power[i] = -1;
    if (isnan(energy[i]))
      energy[i] = -1;
    if (isnan(frequency[i]))
      frequency[i] = -1;
    if (isnan(pf[i]))
      pf[i] = -1;

    if (voltage[i] >= th_voltage[i]) {
      Serial.print(F("Socket"));
      Serial.print(i+1);
      Serial.print(F("voltage threshold reached: "));
      Serial.print(voltage[i]);
      Serial.print(F("/"));
      Serial.print(th_voltage[i]);
      Serial.println();
      relay_state[i] = LOW;
    }
    if (current[i] >= th_current[i]) {
      Serial.print(F("Socket"));
      Serial.print(i+1);
      Serial.print(F("current threshold reached: "));
      Serial.print(current[i]);
      Serial.print(F("/"));
      Serial.print(th_current[i]);
      Serial.println();
      relay_state[i] = LOW;
    }
    if (power[i] >= th_power[i] * 100) {
      Serial.print(F("Socket"));
      Serial.print(i+1);
      Serial.print(F(" power threshold reached: "));
      Serial.print(power[i]);
      Serial.print(F("/"));
      Serial.print(th_power[i] * 100, 2);
      Serial.println();
      relay_state[i] = LOW;
    }
    if (energy[i] >= th_energy[i] / 100.0) {
      Serial.print(F("Socket"));
      Serial.print(i+1);
      Serial.print(F(" energy threshold reached: "));
      Serial.print(energy[i]);
      Serial.print(F("/"));
      Serial.print(th_energy[i] / 100.0, 2);
      Serial.println();
      relay_state[i] = LOW;
    }
    led_state[i] = not(relay_state[i]);
  }

  String dat = "$";
  for (int i = 0; i < 2; i++)
  {
    dat += String(voltage[i], 2);
    dat += ",";
    dat += String(current[i], 2);
    dat += ",";
    dat += String(power[i], 2);
    dat += ",";
    dat += String(energy[i], 2);
    dat += ",";
  }
  if (isnan(temperature[0]))
    temperature[0] = -1;
  if (isnan(temperature[1]))
    temperature[1] = -1;
    
  dat += String(temperature[0], 2);
  dat += ",";
  dat += String(temperature[1], 2);
  dat += ",0,0,1";
  dat += "#";

  Serial.println(dat);
  Serial3.println(dat);
}

void display_pzem()
{
  for (int i = 0; i < 2; i++)
  {
    Serial.print(F("PZEM"));
    Serial.print(i + 1);
    Serial.println(F(" -> "));
    Serial.print(F("Voltage: "));
    Serial.print(voltage[i]);
    Serial.print(F(", "));
    Serial.print(F("Current: "));
    Serial.print(current[i]);
    Serial.print(F(", "));
    Serial.print(F("Power: "));
    Serial.print(power[i]);
    Serial.print(F(", "));
    Serial.print(F("Energy: "));
    Serial.print(energy[i]);
    Serial.print(F(", "));
    Serial.print(F("Frequency: "));
    Serial.print(frequency[i]);
    Serial.print(F(", "));
    Serial.print(F("PF: "));
    Serial.print(pf[i]);
    Serial.print(F(", "));
    Serial.print(F("Temp: "));
    Serial.print(temperature[i]);
    Serial.println();
  }
}

void display_pzem_lcd()
{
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("V1:"));
  lcd.print(String(voltage[0], 2));

  lcd.print(F(" C^:"));
  lcd.print(String(temperature[0], 2));

  lcd.setCursor(0, 1);
  lcd.print(F("I1:"));
  lcd.print(String(current[0], 2));
  lcd.print(F(" P1:"));
  lcd.print(String(power[0], 2));

  lcd.setCursor(0, 2);
  lcd.print(F("V2:"));
  lcd.print(String(voltage[1], 2));

  lcd.print(F(" C^:"));
  lcd.print(String(temperature[1], 2));

  lcd.setCursor(0, 3);
  lcd.print(F("I2:"));
  lcd.print(String(current[1], 2));
  lcd.print(F(" P2:"));
  lcd.print(String(power[1], 2));
}

float read_temp(int Vo)
{
  float R1 = 10000;
  float logR2, R2, T, Tc;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  Tc = T - 273.15;
  return Tc;
}

void read_thermistor()
{
  int sensorValue = analogRead(THERMISTOR_CHANNEL1);
  temperature[0] = read_temp(sensorValue);

  sensorValue = analogRead(THERMISTOR_CHANNEL2);
  temperature[1] = read_temp(sensorValue);
}

void check_temp() {
  if (temperature[0] >= th_temperature[0])
  {
    relay_state[0] = LOW;
    if (th_alarm[0] > 0)
      led_state[0] = not(relay_state[0]);
    else
    led_state[0] = LOW;
  
    Serial.print(F("Socket1"));
    Serial.print(F(" temperature threshold reached: "));
    Serial.print(temperature[0]);
    Serial.print(F("/"));
    Serial.print(th_temperature[0]);
    Serial.println();
  }
  if (temperature[1] >= th_temperature[1])
  {
    relay_state[1] = LOW;
    if (th_alarm[1])
      led_state[1] = not(relay_state[1]);
    else
    led_state[1] = LOW;
  
    Serial.print(F("Socket2"));
    Serial.print(F(" temperature threshold reached: "));
    Serial.print(temperature[1]);
    Serial.print(F("/"));
    Serial.print(th_temperature[1]);
    Serial.println();
  }
}

void sendSettingsToESP() {
  if (millis() - lastSendSettingsMillis >= nextSendSettingsMillis)
  {
    String dat = "$";
    for (int i = 0; i < 2; i++)
    {
      dat += String(th_voltage[i]);
      dat += ",";
      dat += String(th_current[i]);
      dat += ",";
      dat += String(th_power[i]);
      dat += ",";
      dat += String(th_energy[i]);
      dat += ",";
    }
    dat += String(th_temperature[0]);
    dat += ",";
    dat += String(th_temperature[1]);
    dat += ",";
    dat += String(th_alarm[0]);
    dat += ",";
    dat += String(th_alarm[1]);
    dat += ",2";
    dat += "#";

    Serial.println(F("Sending settings"));
    Serial.println(dat);
    Serial3.println(dat);
    lastSendSettingsMillis = millis();
  }
}

void readSettingsFromESP() {
  int expectedCount = 7;
  int valueCount = 0;
  float values[expectedCount];
  int commaIndex = 0;
  int lastCommaIndex = -1;

  while (commaIndex != -1 && valueCount < expectedCount) {
    commaIndex = esp_str.indexOf(',', lastCommaIndex + 1);
    if (commaIndex == -1) {
      // Last value
      values[valueCount] = esp_str.substring(lastCommaIndex + 1).toFloat();
    } else {
      values[valueCount] = esp_str.substring(lastCommaIndex + 1, commaIndex).toFloat();
    }
    lastCommaIndex = commaIndex;
    valueCount++;
  }

  // Check for correct number of values
  if (valueCount != expectedCount) {
    Serial.println("Error: Incorrect data format");
    return;
  }

  int starting_index = -1;
  int esp_level_index = values[0];
  if (esp_level_index == 1)
    starting_index = 0;
  else if (esp_level_index == 2)
    starting_index = 10;

  if (starting_index < 0)
    return;

  Serial.println(F("Saving settings from ESP"));
  Serial.println(esp_str);

  th_voltage[esp_level_index - 1] = values[1];
  EEPROM.write(starting_index + 1, th_voltage[esp_level_index - 1]);

  th_current[esp_level_index - 1] = values[2];
  EEPROM.write(starting_index + 2, th_current[esp_level_index - 1]);

  th_power[esp_level_index - 1] = values[3];
  EEPROM.write(starting_index + 3, th_power[esp_level_index - 1]);

  th_energy[esp_level_index - 1] = values[4];
  EEPROM.write(starting_index + 4, th_energy[esp_level_index - 1]);

  th_temperature[esp_level_index - 1] = values[5];
  EEPROM.write(starting_index + 5, th_temperature[esp_level_index - 1]);

  th_alarm[esp_level_index - 1] = values[6];
  EEPROM.write(starting_index + 6, th_alarm[esp_level_index - 1]);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Socket ");
  lcd.print(esp_level_index);
  lcd.print(" Saved");
  delay(3000);
}

void function_normal()
{
  read_thermistor();

  if (millis() - lastMillis >= nextReadMillis)
  {
    read_pzem();
    check_temp();
    display_pzem();
    display_pzem_lcd();
    lastMillis = millis();
  }
  sendSettingsToESP();
  
  while (Serial3.available()) {
    char c = Serial3.read();
    if (c == '$') {
      esp_str = "";
    } else if (c == '#') {
      readSettingsFromESP();
      esp_str = "";
    } else {
      esp_str += String(c);
    }
  }
}

void setup()
{
  Serial.begin(9600);

  // for (unsigned int i = 0; i < 50; i++) {
  //   Serial.print("Cleaning room: ");
  //   Serial.println(i);
  //   EEPROM.write(i, 0);
  // }

  Serial3.begin(9600);
  PZEM_SERIAL1.begin(9600);
  PZEM_SERIAL2.begin(9600);

  for (int i = 0; i < 2; i++) {
    pinMode(relay_pins[i], OUTPUT);  
    digitalWrite(relay_pins[i], HIGH);

    pinMode(led_pins[i], OUTPUT);  
    digitalWrite(led_pins[i], LOW);
  }

  th_voltage[0] = EEPROM.read(1);
  th_voltage[1] = EEPROM.read(11);
  th_current[0] = EEPROM.read(2);
  th_current[1] = EEPROM.read(12);
  th_power[0] = EEPROM.read(3);
  th_power[1] = EEPROM.read(13);
  th_energy[0] = EEPROM.read(4);
  th_energy[1] = EEPROM.read(14);
  th_temperature[0] = EEPROM.read(5);
  th_temperature[1] = EEPROM.read(15);
  th_alarm[0] = EEPROM.read(6);
  th_alarm[1] = EEPROM.read(16);

  if (th_alarm[0] > 1) th_alarm[0] = 1;
  if (th_alarm[1] > 1) th_alarm[1] = 1;

  button1.attachClick(click1);
  button2.attachClick(click2);
  button3.attachClick(click3);
  button4.attachClick(click4);
  button5.attachClick(click5);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Smart Customizable");
  lcd.setCursor(0, 1);
  lcd.print("  Extension Outlet");
  delay(3000);
}

void loop()
{
  if (function_index == FUNC_NORMAL)
  {
    function_normal();

    for (int i = 0; i < 2; i++) {
      digitalWrite(relay_pins[i], relay_state[i]);
      if (th_alarm[i] > 0)
        digitalWrite(led_pins[i], led_state[i]);
      else
        digitalWrite(led_pins[i], LOW);
    }
  }
  else
  {
    for (int i = 0; i < 2; i++) {
      digitalWrite(relay_pins[i], LOW);
      digitalWrite(led_pins[i], LOW);
    }
  }

  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();
  button5.tick();
  delay(10);
}