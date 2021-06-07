#include "../include/light_array.h"

LightArray& LightArray::add(uint8_t pin)
{
  pinMode(pin, OUTPUT);
  this->off(pin);

  _lights.insert(pin);
  return *this;
}

LightArray& LightArray::on()
{
  for (auto light : _lights) {
    this->on(light);
  }
}

LightArray& LightArray::on(uint8_t pin)
{
  digitalWrite(pin, LOW);
}

LightArray& LightArray::off()
{
  for (auto light : _lights) {
    this->off(light);
  }
}

LightArray& LightArray::off(uint8_t pin)
{
  digitalWrite(pin, HIGH);
}
