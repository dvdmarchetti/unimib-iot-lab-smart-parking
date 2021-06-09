#include "../include/led.h"

Led::Led(int pin) : _pin(pin)
{
    pinMode(_pin, OUTPUT);
}

void Led::switchOn()
{
    digitalWrite(_pin, LOW);
}

void Led::switchOff()
{
    digitalWrite(_pin, HIGH);
}
