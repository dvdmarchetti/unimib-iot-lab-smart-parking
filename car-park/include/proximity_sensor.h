#ifndef __PROXIMITYSENSOR__
#define __PROXIMITYSENSOR__

#include <Arduino.h>

class ProximitySensor
{
public:
    ProximitySensor(int pinInput, int pinOutput);
    float getDistance();

private:
    const float vs = 331.5 + 0.6 * 20; /* Suppose 20° temperature */
    int pinEcho;
    int pinTrig;
};

#endif
