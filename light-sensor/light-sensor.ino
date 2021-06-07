#include "include/light_controller.h"

WiFiClient _wifi_clnt;
LightController controller;
void setup() {
  Serial.begin(115200);

  controller.setup();
}

void loop() {
  controller.loop();
}
