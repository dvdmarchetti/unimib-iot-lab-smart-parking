#include "include/master_controller.h"

MasterController controller;

void setup() {
    Serial.begin(115200);
    delay(200);

    controller.setup();
}

void loop() {
    controller.loop();
}
