#include "../include/display_controller.h"

#include <ArduinoJson.h>

DisplayController::DisplayController(int period) :
  _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
  _period(period)
{
  //
}

void DisplayController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up display"));
  _display.setup();

  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  this->setup_mqtt();
}

void DisplayController::setup_mqtt()
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

void DisplayController::loop()
{
  _wifi_manager.ensure_wifi_is_connected();

  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();

  if (this->is_enabled()) {
    this->fsm_loop();
  }
}

bool DisplayController::is_enabled()
{
  static ulong last_execution = 0;

  if (millis() - last_execution < _period) {
    return false;
  }

  last_execution = millis();
  return true;
}

void DisplayController::fsm_loop()
{
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

        _display.writeFirstRow("     Waiting");
        _display.writeSecondRow("  configuration");
      }
      break;

    case OFF:
      Serial.println(F("[CONTROLLER] State: OFF"));
      break;

    case ON:
      Serial.println(F("[CONTROLLER] State: ON"));
      break;
  }
}

void DisplayController::onMessageReceived(const String &topic, const String &payload)
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

  _first_row = String(doc["textFirstRow"].as<String>());
  _second_row = String(doc["textSecondRow"].as<String>());

  if (topic == MQTT_TOPIC_DEVICE_CONFIG) {
    _topic_commands = doc["topicToSubscribe"].as<String>();

    _mqtt_reader.reconnect().subscribe(_topic_commands);

    Serial.println(F("[CONTROLLER] Received configuration:"));
    Serial.print(F(" > Commands topic: "));
    Serial.println(_topic_commands);
    Serial.print(F(" > First row: "));
    Serial.println(_first_row);
    Serial.print(F(" > Second row: "));
    Serial.println(_second_row);

    _current_state = ON;
    _display.writeFirstRow(_first_row);
    _display.writeSecondRow(_second_row);
  } else if (topic == _topic_commands) {
    const String command = doc["command"].as<String>();

    if (command == "on") {
      _current_state = ON;
      _display.turnOn();
      _display.writeFirstRow(_first_row);
      _display.writeSecondRow(_second_row);
    } else if (command == "off") {
      _current_state = OFF;
      _display.turnOff();
    } else if (command == "update") {
      _display.writeFirstRow(_first_row);
      _display.writeSecondRow(_second_row);
    } else {
      Serial.println(F("[CONTROLLER] Payload not recognized. Message skipped."));
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
  }
}

void DisplayController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
