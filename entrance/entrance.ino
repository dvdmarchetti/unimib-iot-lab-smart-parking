#include "include/entrance_controller.h"

EntranceController controller;
void setup() {
  Serial.begin(115200);

  controller.setup();
}

void loop() {
  controller.loop();
}
