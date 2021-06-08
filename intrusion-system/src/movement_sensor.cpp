#include "../include/movement_sensor.h"
#include "Arduino.h"

MovementSensor::MovementSensor(int pin)
{
  this -> pin = pin; 
  pinMode(pin, INPUT);
}

bool MovementSensor::isMovementDetected()
{
  return digitalRead(pin) == HIGH;
}
