#include "../include/entrance_controller.h"

#include <ArduinoJson.h>

EntranceController::EntranceController() :
  _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
  _reader(RfidReader(SS_PIN, RST_PIN))
{
  //
}

void EntranceController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up Display"));
  _display.setup();
  _display.writeFirstRow("Smart Parking:");
  _display.writeSecondRow("Booting...");
  _display.turnOn();

  Serial.println(F("[CONTROLLER] Setting up RFID reader"));
  _reader.setup();
  _reader.onCardAvailable(this, &RfidReceiver::onCardAvailable);

  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  this->setup_mqtt();
}

void EntranceController::setup_mqtt()
{
  Serial.println(F("[CONTROLLER] Setting up MQTT"));

  // Create last_will payload (same as config payload)
  String payload;
  this->device_payload(payload);

  _mqtt_writer.begin().connect();

  _mqtt_reader.begin()
    .setLastWill(MQTT_TOPIC_DEVICE_LAST_WILL, payload)
    .onMessageReceived(this, &MqttReceiver::onMessageReceived)
    .connect().subscribe(MQTT_TOPIC_DEVICE_CONFIG, 2);

  _last_action = millis();
}

void EntranceController::loop()
{
  if (_current_state == ON) {
    if (millis() - _last_action > 10000) {
      Serial.println(F("[CONTROLLER] Activating Deep Sleep"));
      _display.turnOff();
      ESP.deepSleep(ESP.deepSleepMax());
    }
  }

  _wifi_manager.ensure_wifi_is_connected();

  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();

  this->read_sensors();
  this->fsm_loop();
  this->update_display();
}

void EntranceController::read_sensors()
{
  static ulong last_read = 0;

  if (millis() - last_read < 15
   || (_current_state == OFF || _current_state == WAIT_CONFIGURATION)
  ) {
    return;
  }

  _reader.loop();
}

void EntranceController::fsm_loop()
{
  static ulong last_execution = 0;

  switch (_current_state) {
    case WAIT_CONFIGURATION:
      if (_has_requested_configuration) {
        return;
      }

      {
        String payload;
        this->device_payload(payload);

        _display.writeFirstRow("Smart Parking:");
        _display.writeSecondRow("Waiting config");
        Serial.print(F("[CONTROLLER] Requesting configuration: "));
        Serial.println(payload);

        _mqtt_writer.connect().publish(MQTT_TOPIC_GLOBAL_CONFIG, payload, false, 2);
        _has_requested_configuration = true;
      }
      break;
  }

  last_execution = millis();
}

void EntranceController::update_display()
{
  if (! (_repaint || (_is_holding && millis() - _hold_start > 3000))) {
    return;
  }
  _repaint = false;
  _is_holding = false;

  if (_display.isOff()) {
    _display.turnOn();
  }

  switch (_current_state) {
    case WAIT_CONFIGURATION:
      _display.writeFirstRow("SMART PARKING");
      _display.writeSecondRow("> Waiting config");
      break;
    case OFF:
      _display.turnOff();
      break;
    case ON:
      _display.writeFirstRow("SMART PARKING");
      _display.writeSecondRow("> Waiting card");
      break;
    case MANAGE:
      switch (_manage_state) {
        case WAIT_MASTER:
          _display.writeFirstRow("MANAGE CARDS");
          _display.writeSecondRow("> Waiting master");
          break;
        case ACTIVE:
          _display.writeFirstRow("MANAGE CARDS");
          _display.writeSecondRow("> Waiting card");
          break;
        case AUTHORIZE:
          _display.writeFirstRow("MANAGE CARDS");
          _display.writeSecondRow("> Authorizing...");
          break;
      }
  }
}

void EntranceController::manage_fsm_loop(StaticJsonDocument<256> &doc)
{
  String card = doc["card"];
  bool is_master = doc["is_master"];
  bool authorized = doc["authorized"];

  switch (_manage_state) {
    case WAIT_MASTER:
      if (is_master) {
        _manage_state = ACTIVE;
        _repaint = true;

        Serial.println(F("[CONTROLLER] State: MANAGE (ACTIVE)"));
        Serial.println(F("[CONTROLLER] Master correct"));
      } else {
        _display.writeSecondRow("> Wrong master  ");
        this->hold_display();

        Serial.println(F("[CONTROLLER] Wrong card!"));
      }
      break;

    case AUTHORIZE:
      if (is_master) {
        _manage_state = WAIT_MASTER;
        _current_state = ON;

        _repaint = true;
        this->reset_deep_sleep_timer();

        Serial.println(F("[CONTROLLER] State: ON"));
        return;
      }

      String line = F("CARD: ");
      line.concat(card);
      _display.writeFirstRow(line);
      if (authorized) {
        Serial.println(F("[CONTROLLER] Card authorized"));

        _display.writeSecondRow("> Authorized    ");
        this->hold_display();
      } else {
        Serial.println(F("[CONTROLLER] Card removed"));

        _display.writeSecondRow("> Removed       ");
        this->hold_display();
      }
      _manage_state = ACTIVE;

      break;
  }
}

void EntranceController::onMessageReceived(const String &topic, const String &payload)
{
  Serial.print(F("[CONTROLLER] "));
  Serial.print(topic);
  Serial.print(F(": "));
  Serial.println(payload);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("[CONTROLLER] Invalid payload: "));
    Serial.println(error.f_str());
    return;
  }

  if (topic == MQTT_TOPIC_DEVICE_CONFIG) {
    _topic_commands = doc["topicToSubscribe"].as<String>();
    _topic_check = doc["topicToCheck"].as<String>();
    _topic_authorize = doc["topicToAuthorize"].as<String>();

    _mqtt_reader.reconnect().subscribe(_topic_commands);

    Serial.println(F("[CONTROLLER] Received configuration:"));
    Serial.print(F(" > Commands topic: "));
    Serial.println(_topic_commands);
    Serial.print(F(" > Check topic: "));
    Serial.println(_topic_check);
    Serial.print(F(" > Authorize topic: "));
    Serial.println(_topic_authorize);

    _current_state = ON;
    _repaint = true;
    this->reset_deep_sleep_timer();
  } else if (topic == _topic_commands) {
    String card = doc["card"];
    bool is_master = doc["is_master"];
    bool authorized = doc["authorized"];

    if (_current_state == ON) {
      if (is_master) {
        _current_state = MANAGE;
        _manage_state = ACTIVE;
        _repaint = true;

        Serial.println(F("[CONTROLLER] State: MANAGE (ACTIVE)"));
      } else if (authorized) {
        Serial.println(F("[CONTROLLER] Access granted"));

        _display.writeSecondRow("> Access granted");
        this->hold_display();

        this->reset_deep_sleep_timer();
      } else if (! authorized) {
        Serial.println(F("[CONTROLLER] Invalid card"));

        _display.writeSecondRow("> Invalid card  ");
        this->hold_display();

        this->reset_deep_sleep_timer();
      }
    } else if (_current_state == MANAGE) {
      this->manage_fsm_loop(doc);
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
  }
}

void EntranceController::onCardAvailable(const String &serial)
{
  static ulong last_execution = 0;
  if (millis() - last_execution < 1000) {
    return;
  }

  last_execution = millis();
  Serial.print(F("[CONTROLLER] Reading card: "));
  Serial.println(serial);

  StaticJsonDocument<JSON_OBJECT_SIZE(4)> doc;
  doc["card"] = serial;

  String payload;
  serializeJson(doc, payload);

  if (_current_state == ON) {
    _mqtt_writer.reconnect().publish(_topic_check, payload);
  } else if (_current_state == MANAGE) {
    _mqtt_writer.reconnect().publish(_topic_authorize, payload);
    _manage_state = AUTHORIZE;
  }

  _reader.halt();
}

void EntranceController::hold_display()
{
  _is_holding = true;
  _hold_start = millis();
}

void EntranceController::reset_deep_sleep_timer()
{
  _last_action = millis();
}

void EntranceController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
