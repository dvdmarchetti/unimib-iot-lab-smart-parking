#ifndef __DISPLAY__
#define __DISPLAY__

#define DISPLAY_CHARS 16    // number of characters on a line
#define DISPLAY_LINES 2     // number of display lines
#define DISPLAY_ADDR 0x27   // display address on I2C bus

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>   // display library
#include <Wire.h>                // I2C library

class Display
{
public:
  Display();

  void setup();
  void writeFirstRow(const String &line);
  void writeSecondRow(const String &line);
  void clear();
  void turnOff();
  void turnOn();

private:
  LiquidCrystal_I2C _lcd;
};

#endif
