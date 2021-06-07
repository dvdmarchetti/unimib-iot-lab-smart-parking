#include "../include/proximity_sensor.h"

ProximitySensor::ProximitySensor(int pinInput, int pinOutput)
{
    this->pinEcho = pinInput;
    this->pinTrig = pinOutput;
    pinMode(this->pinEcho, INPUT);
    pinMode(this->pinTrig, OUTPUT);
}

float ProximitySensor::getDistance()
{
    /* send pulse */
    digitalWrite(pinTrig, LOW);
    delayMicroseconds(3);
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(5);
    digitalWrite(pinEcho, LOW);

    /* receive pulse */
    float tUS = pulseIn(pinEcho, HIGH);
    float t = tUS / 1000.0 / 1000.0 / 2;
    float d = t * vs;
    return d;
}
