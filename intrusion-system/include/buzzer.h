#ifndef __BUZZER__
#define __BUZZER__

#include "Arduino.h"

class Buzzer
{
public:
    Buzzer(int pin);
    void soundOn();
    void soundOff();

private:
    int pin;
};

#endif
