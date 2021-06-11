#include "../include/gate_controller.h"

#include <ArduinoJson.h>

GateController::GateController() : _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
                                  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
                                  _servo(ServoMotor(SERVO_PIN))
{
  //
}

void GateController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up entrance gate"));
  _servo.move(0); // Return servo to init position.

  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  this->setupMqtt();
}

void GateController::setupMqtt()
{
  Serial.println(F("[CONTROLLER] Setting up MQTT"));

  // Create last_will payload (same as config payload)
  String payload;
  this->device_payload(payload);

  _mqtt_writer.begin().connect();

  _mqtt_reader.begin()
      .setLastWill("smpk/last-will", payload)
      .onMessageReceived(this, &MqttReceiver::onMessageReceived)
      .connect()
      .subscribe(MQTT_TOPIC_DEVICE_CONFIG, 2);
}

void GateController::loop()
{
  _wifi_manager.ensure_wifi_is_connected();

  _mqtt_reader.reconnect().listen();

  this->fsm_loop();
  //this->send_mqtt_data();
}

void GateController::fsm_loop()
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

        Serial.print("[CONTROLLER] Requesting configuration: ");
        Serial.println(payload);

        _mqtt_writer.connect().publish(MQTT_TOPIC_GLOBAL_CONFIG, payload, false, 2);
        _has_requested_configuration = true;

        break;
      }

    case CLOSE:
      _opened = false;
      _servo.move(0);
      break;

    case OPEN:
      _servo.move(180);

      if (!_opened) {
        _opened = true;
        _start_open = millis();
      } else if (millis() - _start_open >= _open_time) {
        _current_state = CLOSE;
        _servo.move(0);
        _opened = false;
      }

      break;

    }

  last_execution = millis();
}

void GateController::onMessageReceived(const String &topic, const String &payload)
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
    _open_time = doc["openTime"].as<uint>();

    _mqtt_reader.subscribe(_topic_commands);

    // if (doc.containsKey("mqttPushCooldown")) {
    //     _mqtt_push_cooldown = doc["mqttPushCooldown"];
    // }

    Serial.println(F("[CONTROLLER] Received configuration:"));
    Serial.print(F(" > Commands topic: "));
    Serial.println(_topic_commands);

    _current_state = CLOSE;
  } else if (topic == _topic_commands) {
    uint status = doc["command"];

    if (status == 0) {
      _current_state = CLOSE;
    } else if (status == 1) {
      _current_state = OPEN;
    } else {
      Serial.println(F("[CONTROLLER] Payload not recognized. Message skipped."));
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
  }
}

void GateController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
