#include "include/intrusion_controller.h"

IntrusionController controller(INTRUSION_SYSTEM_PERIOD);

void setup()
{
  Serial.begin(115200);

  controller.setup();

  //delay(PIR_SETUP_PERIOD);
}

void loop()
{
  controller.loop();
}
