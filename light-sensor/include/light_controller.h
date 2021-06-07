#ifndef __LIGHT_CONTROLLER__
#define __LIGHT_CONTROLLER__

#include "../config.h"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "wifi_manager.h"
#include "photoresistor.h"
#include "light_array.h"

class LightController : public MqttReceiver
{
public:
  LightController();

  void setup();
  void loop();

private:
  // Services
  WiFiManager _wifi_manager;
  MqttWrapper _mqtt_reader;
  MqttWrapper _mqtt_writer;
  Photoresistor _photoresistor;
  LightArray _light_array;

  // State
  enum State { WAIT_CONFIGURATION, OFF, ON, AUTO };
  State _current_state = WAIT_CONFIGURATION;

  // Dynamic configuration
  bool _has_requested_configuration = false;
  String _topic_commands;
  String _topic_values;
  uint _mqtt_push_cooldown = 3000;
  uint _light_amount_threshold = 300;

  // Sensor readings
  uint16_t _light_amount = 0;

  void setupMqtt();
  void onMessageReceived(const String &topic, const String &payload);

  void read_sensors();
  void fsm_loop();

  void send_mqtt_data();

  void device_payload(String &destination);
};

#endif // __LIGHT_CONTROLLER__
