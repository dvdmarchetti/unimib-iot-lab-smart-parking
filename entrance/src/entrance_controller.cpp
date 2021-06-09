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
}

void EntranceController::loop()
{
  _wifi_manager.ensure_wifi_is_connected();

  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();

  this->read_sensors();
  this->fsm_loop();
  // this->send_mqtt_data();
}

void EntranceController::read_sensors()
{
  static ulong last_read = 0;

  if (millis() - last_read < 15
  //  || (_current_state == OFF || _current_state == WAIT_CONFIGURATION)
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

        Serial.print(F("[CONTROLLER] Requesting configuration: "));
        Serial.println(payload);

        _mqtt_writer.connect().publish(MQTT_TOPIC_GLOBAL_CONFIG, payload, false, 2);
        _has_requested_configuration = true;
      }
      break;

    case OFF:
      // Do nothing
      break;

    case ON | MANAGE:
      if (_manage_state != CHECKING) {
        _reader.loop();
      }
      break;
  }

  last_execution = millis();
}

void EntranceController::send_mqtt_data()
{
  // static ulong last_execution = millis();

  // if (_current_state == WAIT_CONFIGURATION
  //  || millis() - last_execution < _mqtt_push_cooldown) {
  //   return;
  // }

  // last_execution = millis();
  // Serial.print(F("[CONTROLLER] Sending data: "));

  // StaticJsonDocument<JSON_OBJECT_SIZE(5)> doc;
  // doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  // doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  // doc["status"] = (_current_state == ON) ? "on" : (_current_state == OFF) ? "off" : "auto";
  // doc["lightAmount"] = _light_amount;
  // // if (_current_state == AUTO) {
  // //   doc["lightOn"] = _light_amount < _light_amount_threshold;
  // // }

  // String payload;
  // serializeJson(doc, payload);
  // Serial.println(payload);

  // _mqtt_writer.connect().publish(_topic_values, payload);
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

    _current_state = MANAGE;
  } else if (topic == _topic_commands) {
    String card = doc["card"];
    bool is_master = doc["is_master"];
    bool authorized = doc["authorized"];

    if (_current_state == ON) {
      if (is_master) {
        _current_state = MANAGE;
        _manage_state = ACTIVE;
        Serial.println(F("[CONTROLLER] State: MANAGE (ACTIVE)"));
      } else if (authorized) {
        Serial.println(F("[CONTROLLER] Access granted"));
      }
    } else if (_current_state == MANAGE) {
      this->manage_fsm_loop(doc);
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
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
        Serial.println(F("[CONTROLLER] State: MANAGE (ACTIVE)"));
        Serial.println(F("[CONTROLLER] Master correct"));
      } else {
        Serial.println(F("[CONTROLLER] Wrong card!"));
      }
      break;

    case CHECKING:
      if (is_master) {
        _manage_state = WAIT_MASTER;
        _current_state = ON;
        Serial.println(F("[CONTROLLER] State: ON"));
        return;
      }

      if (authorized) {
        Serial.println(F("[CONTROLLER] Card authorized"));
      } else {
        Serial.println(F("[CONTROLLER] Card removed"));
      }
      _manage_state = ACTIVE;
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
  if (_current_state == ON) {
    doc["action"] = "check";
  } else {
    doc["action"] = "authorize";
  }

  String payload;
  serializeJson(doc, payload);

  if (_current_state == ON) {
    _mqtt_writer.reconnect().publish(_topic_check, payload);
  } else if (_current_state == MANAGE) {
    _mqtt_writer.reconnect().publish(_topic_authorize, payload);
    _manage_state = CHECKING;
  }
}

void EntranceController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
