#include <Arduino.h>
#include <OneButton.h>
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define PZEM_SERIAL1 Serial1
#define PZEM_SERIAL2 Serial2
#define THERMISTOR_CHANNEL A0
#define BUTTON1 8
#define BUTTON2 9
#define BUTTON3 10
#define BUTTON4 11
#define BUTTON5 12
#define RELAY1 6
#define RELAY2 7
#define LED 5

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
float temperature = 0;
unsigned long lastMillis = 0;
unsigned long nextReadMillis = 3000;

String lcd_empty_row = "                    ";

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
      lcd.print(String(th_current[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_CURRENT)
    {
      param_index = PARAM_POWER;
      lcd.print(F("POWER"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_power[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_POWER)
    {
      param_index = PARAM_ENERGY;
      lcd.print(F("ENERGY"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_energy[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_ENERGY)
    {
      param_index = PARAM_TEMPERATURE;
      lcd.print(F("TEMPERATURE"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_temperature[LEVEL_SELECT_PARAM1 - 1]));
    }
    else
    {
      param_index = PARAM_VOLTAGE;
      lcd.print(F("VOLTAGE"));
      lcd.setCursor(0, 2);
      lcd.print(String(th_voltage[LEVEL_SELECT_PARAM1 - 1]));
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
    lcd.print(String(th_voltage[LEVEL_SELECT_PARAM1 - 1]));
  }
}

void click3()
{
  Serial.println(F("Button 3"));
  if (level_index == LEVEL_SELECT_PARAM1 || level_index == LEVEL_SELECT_PARAM2)
  {
    lcd.setCursor(0, 2);
    lcd.print(lcd_empty_row);

    int starting_index = 0;
    if (level_index == LEVEL_SELECT_PARAM2) starting_index = 10;

    if (param_index == PARAM_VOLTAGE)
    {
      if (th_voltage[LEVEL_SELECT_PARAM1 - 1] > 0)
      {
        th_voltage[LEVEL_SELECT_PARAM1 - 1]--;
        EEPROM.write(starting_index + 1, th_voltage[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_voltage[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_CURRENT)
    {
      if (th_current[LEVEL_SELECT_PARAM1 - 1] > 0)
      {
        th_current[LEVEL_SELECT_PARAM1 - 1]--;
        EEPROM.write(starting_index + 2, th_current[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_current[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_POWER)
    {
      if (th_power[LEVEL_SELECT_PARAM1 - 1] > 0)
      {
        th_power[LEVEL_SELECT_PARAM1 - 1]--;
        EEPROM.write(starting_index + 3, th_power[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_power[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_ENERGY)
    {
      if (th_energy[LEVEL_SELECT_PARAM1 - 1] > 0)
      {
        th_energy[LEVEL_SELECT_PARAM1 - 1]--;
        EEPROM.write(starting_index + 4, th_energy[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_energy[LEVEL_SELECT_PARAM1 - 1]));
    }
    else
    {
      if (th_temperature[LEVEL_SELECT_PARAM1 - 1] > 0)
      {
        th_temperature[LEVEL_SELECT_PARAM1 - 1]--;
        EEPROM.write(starting_index + 5, th_temperature[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_temperature[LEVEL_SELECT_PARAM1 - 1]));
    }
  }
}

void click4()
{
  Serial.println(F("Button 4"));
  if (level_index == LEVEL_SELECT_PARAM1 || level_index == LEVEL_SELECT_PARAM2)
  {
    lcd.setCursor(0, 2);
    lcd.print(lcd_empty_row);

    int starting_index = 0;
    if (level_index == LEVEL_SELECT_PARAM2) starting_index = 10;

    if (param_index == PARAM_VOLTAGE)
    {
      if (th_voltage[LEVEL_SELECT_PARAM1 - 1] < 255)
      {
        th_voltage[LEVEL_SELECT_PARAM1 - 1]++;
        EEPROM.write(starting_index + 1, th_voltage[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_voltage[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_CURRENT)
    {
      if (th_current[LEVEL_SELECT_PARAM1 - 1] < 255)
      {
        th_current[LEVEL_SELECT_PARAM1 - 1]++;
        EEPROM.write(starting_index + 2, th_current[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_current[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_POWER)
    {
      if (th_power[LEVEL_SELECT_PARAM1 - 1] < 255)
      {
        th_power[LEVEL_SELECT_PARAM1 - 1]++;
        EEPROM.write(starting_index + 3, th_power[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_power[LEVEL_SELECT_PARAM1 - 1]));
    }
    else if (param_index == PARAM_ENERGY)
    {
      if (th_energy[LEVEL_SELECT_PARAM1 - 1] < 255)
      {
        th_energy[LEVEL_SELECT_PARAM1 - 1]++;
        EEPROM.write(starting_index + 4, th_energy[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_energy[LEVEL_SELECT_PARAM1 - 1]));
    }
    else
    {
      if (th_temperature[LEVEL_SELECT_PARAM1 - 1] < 255)
      {
        th_temperature[LEVEL_SELECT_PARAM1 - 1]++;
        EEPROM.write(starting_index + 5, th_temperature[LEVEL_SELECT_PARAM1 - 1]);
      }
      lcd.setCursor(0, 2);
      lcd.print(String(th_temperature[LEVEL_SELECT_PARAM1 - 1]));
    }
  }
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
  }
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
    Serial.println(pf[i]);
  }
  Serial.print(F("Temperature: "));
  Serial.println(temperature);
}

void display_pzem_lcd()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("V1:"));
  lcd.print(String(voltage[0], 2));

  lcd.print(F(" C^:"));
  lcd.print(String(temperature, 2));

  lcd.setCursor(0, 1);
  lcd.print(F("I1:"));
  lcd.print(String(current[0], 2));
  lcd.print(F(" P1:"));
  lcd.print(String(power[0], 2));

  lcd.setCursor(0, 2);
  lcd.print(F("V2:"));
  lcd.print(String(voltage[1], 2));
  lcd.setCursor(0, 3);
  lcd.print(F("I2:"));
  lcd.print(String(current[1], 2));
  lcd.print(F(" P2:"));
  lcd.print(String(power[1], 2));
}

void read_thermistor()
{
  int sensorValue = analogRead(THERMISTOR_CHANNEL);

  float resistance = nominalResistance * (referenceVoltage / (sensorValue * (referenceVoltage / 1023.0) - 1));

  // Calculate temperature using the Steinhart-Hart equation or a simpler approximation (Beta parameter method)
  temperature = 1.0 / (1.0 / (nominalTemperature + 273.15) + log(resistance / nominalResistance) / bCoefficient);

  // Convert from Kelvin to Celsius
  temperature -= 273.15;
  temperature = abs(temperature);
}

void function_normal()
{
  if (millis() - lastMillis >= nextReadMillis)
  {
    read_pzem();
    read_thermistor();
    display_pzem();
    display_pzem_lcd();
    lastMillis = millis();
  }
}

void function_set_pzem1()
{
}

void function_set_pzem2()
{
}

void relay1_on()
{
  Serial.println(F("D8"));
  digitalWrite(RELAY1, HIGH);
  digitalWrite(LED, HIGH);
}

void relay1_off()
{
  Serial.println(F("D9"));
  digitalWrite(RELAY1, LOW);
  digitalWrite(LED, LOW);
}

void relay2_on()
{
  Serial.println(F("D10"));
  digitalWrite(RELAY2, HIGH);
  digitalWrite(LED, HIGH);
}

void relay2_off()
{
  Serial.println(F("D11"));
  digitalWrite(RELAY2, LOW);
  digitalWrite(LED, LOW);
}

void setup()
{
  Serial.begin(9600);
  PZEM_SERIAL1.begin(9600);
  PZEM_SERIAL2.begin(9600);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(LED, OUTPUT);

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

  button1.attachClick(relay1_on);
  button2.attachClick(relay1_off);
  button3.attachClick(relay2_on);
  button4.attachClick(relay2_off);
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
  if (function_index == FUNC_PZEM1)
  {
    function_set_pzem1();
  }
  else if (function_index == FUNC_PZEM2)
  {
    function_set_pzem2();
  }
  else
  {
    function_normal();
  }

  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();
  button5.tick();
  delay(10);
}