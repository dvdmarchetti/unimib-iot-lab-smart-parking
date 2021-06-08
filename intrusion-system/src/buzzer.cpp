#include "../include/buzzer.h"

Buzzer::Buzzer(int pin)
{
    this->pin = pin;
    pinMode(pin, OUTPUT);
}

// Like led the buzzer is turned on by writing low level voltage.
void Buzzer::soundOn()
{
    digitalWrite(pin, LOW);
}

void Buzzer::soundOff()
{
    digitalWrite(pin, HIGH);
}
