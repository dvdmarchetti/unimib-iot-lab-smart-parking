#include "include/car_park_controller.h"

CarParkController controller(CAR_PARK_PERIOD);
void setup() {
  Serial.begin(115200);

  controller.setup();
}

void loop() {
  controller.loop();
}
