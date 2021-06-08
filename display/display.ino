#include "include/display_controller.h"

#define CAR_PARK_MONITOR_PERIOD 5000

DisplayController controller(CAR_PARK_MONITOR_PERIOD);
void setup() {
  Serial.begin(115200);

  Serial.println("Setup");
  controller.setup();
}

void loop() {
  controller.loop();
}
