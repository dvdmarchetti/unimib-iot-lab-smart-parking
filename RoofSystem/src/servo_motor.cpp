#include "include/roof_controller.h"

WiFiClient _wifi_clnt;
RoofController controller;

void setup()
{
    Serial.begin(115200);
    controller.setup();
}

void loop()
{
    controller.loop();
}
