#ifndef __ENTRANCE_CONTROLLER__
#define __ENTRANCE_CONTROLLER__

#include <ArduinoJson.h>
#include <ESPNtpClient.h>
#include "../config.h"
#include "wifi_manager.h"
#include "display.h"
#include "eeprom_manager.hpp"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "rfid_receiver.h"
#include "rfid_reader.h"

#define ENTRANCE_JSON_BUFFER_SIZE 256

typedef struct {
  time_t last_update;
  char value[396];
} time_config;

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
  EEPROMManager<time_config> _eeprom_manager;

  // State
  enum State { WAIT_CONFIGURATION, OFF, ON, VERIFY, MANAGE };
  State _current_state = WAIT_CONFIGURATION;
  enum ManageState { WAIT_MASTER, ACTIVE, AUTHORIZE };
  ManageState _manage_state = WAIT_MASTER;

  // Display handling
  bool _repaint = false;
  bool _is_holding = false;
  ulong _hold_start = 0;
  ulong _last_action = 0;

  // Dynamic configuration
  time_config _config;
  bool _has_requested_configuration = false;
  String _topic_commands;
  String _topic_values;

  void setup_mqtt();
  void onMessageReceived(const String &topic, const String &payload);
  void onCardAvailable(const String &serial);

  void read_sensors();
  void fsm_loop();
  void read_config_from_eeprom();
  void update_display();
  void manage_fsm_loop(StaticJsonDocument<ENTRANCE_JSON_BUFFER_SIZE> &doc);

  void hold_display();
  void reset_deep_sleep_timer();
  void device_payload(String &destination);
};

#endif // __ENTRANCE_CONTROLLER__
