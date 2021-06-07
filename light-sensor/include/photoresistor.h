#ifndef __PHOTORESISTOR__
#define __PHOTORESISTOR__

#include <Arduino.h>

#define PHOTORESISTOR_READ_COOLDOWN 10

class Photoresistor
{
public:
  Photoresistor(uint8_t input_pin, uint cooldown = PHOTORESISTOR_READ_COOLDOWN);
  uint read();

private:
  const uint _cooldown;
  const uint8_t _input_pin;
};

#endif // __PHOTORESISTOR__
