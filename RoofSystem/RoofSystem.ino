#include "include/servo_motor.h"
#define PIN D4
ServoMotor *servoMotor;

void setup() {
  Serial.begin(115200);

  servoMotor = new ServoMotor(PIN);

  Serial.println(F("\n\nSetup completed.\n\n"));
}

void loop() {
  // put your main code here, to run repeatedly:
  servoMotor->move(0);
  delay(1000);
  servoMotor->move(180);
  delay(1000);
}
