#include "../include/Display.h"

Display::Display() : _lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES)
{
  //
}

void Display::setup()
{
  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    _lcd.begin(DISPLAY_CHARS, 2);
  } else {
    Serial.print(F("LCD not found. Error "));
    Serial.println(error);
    Serial.println(F("Check connections and configuration. Reset to try again!"));
    while (true)
      delay(1);
  }

  _lcd.setBacklight(50);
  _lcd.home();
  _lcd.clear();
}

void Display::writeFirstRow(const String &line)
{
  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print(line);
}

void Display::writeSecondRow(const String &line)
{
  _lcd.setCursor(0, 1);
  _lcd.print(line);
}

void Display::clear()
{
  _lcd.clear();
}

bool Display::isOff()
{
  return _state == OFF;
}

bool Display::isOn()
{
  return _state == ON;
}

void Display::turnOff()
{
  _state = OFF;
  _lcd.noDisplay();
  _lcd.setBacklight(0);
}

void Display::turnOn()
{
  _state = ON;
  _lcd.display();
  _lcd.setBacklight(50);
}
