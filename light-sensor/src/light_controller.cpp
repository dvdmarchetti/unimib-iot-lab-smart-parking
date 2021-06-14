#include "../include/light_controller.h"

#include <ArduinoJson.h>

LightController::LightController() :
  _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
  _photoresistor(Photoresistor(PHOTORESISTOR_PIN))
{
  //
}

void LightController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up Leds"));
  _light_array
    .add(LED_PIN_1)
    .add(LED_PIN_2)
    .add(LED_PIN_3)
    .add(LED_PIN_4)
    .add(LED_PIN_5)
    .add(LED_PIN_6)
    .off();

  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  this->setup_mqtt();
}

void LightController::setup_mqtt()
{
  Serial.println(F("[CONTROLLER] Setting up MQTT"));

  // Create last_will payload (same as config payload)
  String payload;
  this->device_payload(payload);

  _mqtt_writer.begin()
    .setLastWill(MQTT_TOPIC_DEVICE_LAST_WILL, payload)
    .connect();

  _mqtt_reader.begin()
    .onMessageReceived(this, &MqttReceiver::onMessageReceived)
    .connect().subscribe(MQTT_TOPIC_DEVICE_CONFIG, 2);
}

void LightController::loop()
{
  _wifi_manager.ensure_wifi_is_connected();

  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();

  this->read_sensors();
  this->fsm_loop();
  this->send_mqtt_data();
}

void LightController::read_sensors()
{
  static ulong last_read = 0;

  if (millis() - last_read < 15
   || (_current_state == OFF || _current_state == WAIT_CONFIGURATION)
  ) {
    return;
  }

  last_read = millis();
  _light_amount = _photoresistor.read();
}

void LightController::fsm_loop()
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

        _light_array.on(LED_PIN_1);
        _light_array.on(LED_PIN_3);
        _light_array.on(LED_PIN_5);
      }
      break;

    case OFF:
      _light_array.off();
      break;

    case ON:
      _light_array.on();
      break;

    case AUTO:
      if (millis() - last_execution < 30) {
        return;
      }

      if (_light_amount <= _light_amount_threshold) {
        _light_array.on();
      } else {
        _light_array.off();
      }
      break;
  }

  last_execution = millis();
}

void LightController::send_mqtt_data()
{
  static ulong last_execution = millis();

  if (_current_state == WAIT_CONFIGURATION
   || millis() - last_execution < _mqtt_push_cooldown) {
    return;
  }

  last_execution = millis();
  Serial.print(F("[CONTROLLER] Sending data: "));

  StaticJsonDocument<JSON_OBJECT_SIZE(5)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  doc["status"] = (_current_state == ON) ? "on" : (_current_state == OFF) ? "off" : "auto";
  doc["lightAmount"] = _light_amount;
  if (_current_state == AUTO) {
    doc["lightOn"] = _light_amount < _light_amount_threshold;
  }

  String payload;
  serializeJson(doc, payload);
  Serial.println(payload);

  _mqtt_writer.connect().publish(_topic_values, payload);
}

void LightController::onMessageReceived(const String &topic, const String &payload)
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
    _topic_values = doc["topicToPublish"].as<String>();

    _mqtt_reader.reconnect().subscribe(_topic_commands);

    if (doc.containsKey("mqttPushCooldown")) {
      _mqtt_push_cooldown = doc["mqttPushCooldown"];
    }

    if (doc.containsKey("lightAmountThreshold")) {
      _light_amount_threshold = doc["lightAmountThreshold"];
    }

    Serial.println(F("[CONTROLLER] Received configuration:"));
    Serial.print(F(" > Commands topic: "));
    Serial.println(_topic_commands);
    Serial.print(F(" > Values topic: "));
    Serial.println(_topic_values);
    Serial.print(F(" > MQTT Push Cooldown: "));
    Serial.println(_mqtt_push_cooldown);
    Serial.print(F(" > Light Amount Off Threshold: "));
    Serial.println(_light_amount_threshold);

    _current_state = AUTO;
  } else if (topic == _topic_commands) {
    uint status = doc["command"];

    if (status == 0) {
      _current_state = OFF;
    } else if (status == 1) {
      _current_state = ON;
    } else if (status == 2) {
      _current_state = AUTO;
    } else {
      Serial.println(F("[CONTROLLER] Payload not recognized. Message skipped."));
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
  }
}

void LightController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
