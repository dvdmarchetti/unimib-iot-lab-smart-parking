#ifndef __LIGHT_ARRAY__
#define __LIGHT_ARRAY__

#include <Arduino.h>
#include <set>

class LightArray
{
public:
  LightArray& add(uint8_t pin);
  LightArray& on();
  LightArray& on(uint8_t pin);
  LightArray& off();
  LightArray& off(uint8_t pin);

private:
  std::set<uint8_t> _lights;
};

#endif // __LIGHT_ARRAY__
