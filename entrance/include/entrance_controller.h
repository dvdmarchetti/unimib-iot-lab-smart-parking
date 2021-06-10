#ifndef __ENTRANCE_CONTROLLER__
#define __ENTRANCE_CONTROLLER__

#include <ArduinoJson.h>
#include "../config.h"
#include "wifi_manager.h"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "rfid_receiver.h"
#include "rfid_reader.h"
#include "display.h"
// #include "servo_motor.h"

#define ENTRANCE_JSON_BUFFER_SIZE 256

class EntranceController : public MqttReceiver, public RfidReceiver
{
public:
  EntranceController();

  void setup();
  void loop();

private:
  // Services
  WiFiManager _wifi_manager;
  MqttWrapper _mqtt_reader;
  MqttWrapper _mqtt_writer;
  RfidReader _reader;
  Display _display;
  // Servo _gate;

  // State
  enum State { WAIT_CONFIGURATION, OFF, ON, MANAGE };
  State _current_state = WAIT_CONFIGURATION;
  enum ManageState { WAIT_MASTER, ACTIVE, CHECKING };
  ManageState _manage_state = WAIT_MASTER;

  // Display handling
  bool _repaint = false;
  ulong _hold_display = 0;

  // Dynamic configuration
  bool _has_requested_configuration = false;
  String _topic_commands;
  String _topic_check;
  String _topic_authorize;

  void setup_mqtt();
  void onMessageReceived(const String &topic, const String &payload);
  void onCardAvailable(const String &serial);

  void read_sensors();
  void fsm_loop();
  void update_display();
  void manage_fsm_loop(StaticJsonDocument<ENTRANCE_JSON_BUFFER_SIZE> &doc);

  void device_payload(String &destination);
};

#endif // __ENTRANCE_CONTROLLER__
