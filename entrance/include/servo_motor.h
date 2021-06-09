#ifndef __SERVO_MOTOR__
#define __SERVO_MOTOR__

#include <Servo.h>

class ServoMotor
{
public:
  ServoMotor(uint8_t pin);

  void move(uint8_t degree);

private:
  Servo _servo;
};

#endif // __SERVO_MOTOR__
