#include "../include/servo_motor.h"

ServoMotor::ServoMotor(int pin) {
    servo.attach(pin);
}

void ServoMotor::move(int degree) {
    servo.write(degree);
}