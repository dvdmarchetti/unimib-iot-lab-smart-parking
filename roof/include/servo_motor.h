#ifndef __SERVOMOTOR__
#define __SERVOMOTOR__

#include <Servo.h>

class ServoMotor
{
public:
    ServoMotor(int pin);
    void move(int degree);

private:
    Servo servo;
};

#endif
