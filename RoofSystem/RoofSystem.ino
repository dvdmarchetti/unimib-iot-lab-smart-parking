#include "include/roof_controller.h"

RoofController controller;

void setup()
{
  Serial.begin(115200);

  controller.setup();

  Serial.println("[CONTROLLER] End of setup.");
}

void loop()
{
  controller.loop();
}
