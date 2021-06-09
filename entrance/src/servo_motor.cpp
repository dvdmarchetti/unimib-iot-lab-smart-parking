#include "../include/servo_motor.h"

ServoMotor::ServoMotor(uint8_t pin)
{
    _servo.attach(pin);
}

void ServoMotor::move(uint8_t degree)
{
    _servo.write(degree);
}
