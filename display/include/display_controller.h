#ifndef __CARPARKMONITOR__
#define __CARPARKMONITOR__

#include "../config.h"
#include "../secrets.h"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "wifi_manager.h"
#include "display.h"

class DisplayController : public MqttReceiver
{
public:
  DisplayController(int period);
  void setup();
  void loop();

private:
  // Services
  WiFiManager _wifi_manager;
  MqttWrapper _mqtt_reader;
  MqttWrapper _mqtt_writer;
  Display _display;

  // State
  enum State { WAIT_CONFIGURATION, OFF, ON };
  State _current_state = WAIT_CONFIGURATION;

  // Dynamic Configuration
  bool _has_requested_configuration = false;
  String _topic_commands;
  String _first_row;
  String _second_row;
  int _period;

  void setup_mqtt();
  void onMessageReceived(const String &topic, const String &payload);

  bool is_enabled();
  void fsm_loop();

  void device_payload(String &destination);
};

#endif
