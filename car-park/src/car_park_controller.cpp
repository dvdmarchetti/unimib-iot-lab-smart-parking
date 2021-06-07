#include "../include/car_park_controller.h"

#include <ArduinoJson.h>

CarParkController::CarParkController(int period) :
  _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
  _free(new Led(PIN_LED_FREE)),
  _busy(new Led(PIN_LED_BUSY)),
  _ps(new ProximitySensor(PIN_ECHO, PIN_TRIG)),
  _period(period)
{
  Serial.println(F("[CONTROLLER] Setting up Leds"));
  _busy->switchOff();
  _free->switchOff();
}

void CarParkController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  this->setup_mqtt();
}

void CarParkController::setup_mqtt()
{
  Serial.println(F("[CONTROLLER] Setting up MQTT"));

  // Create last_will payload (same as config payload)
  String payload;
  this->device_payload(payload);

  _mqtt_writer.begin().connect();

  _mqtt_reader.begin()
    .setLastWill("smpk/last-will", payload)
    .onMessageReceived(this, &MqttReceiver::onMessageReceived)
    .connect().subscribe(MQTT_TOPIC_DEVICE_CONFIG, 2);
}

void CarParkController::loop()
{
  _wifi_manager.ensure_wifi_is_connected();

  if (this->is_enabled()) {
    this->read_sensors();
    this->fsm_loop();
  }

  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();
}

bool CarParkController::is_enabled()
{
  static ulong last_execution = millis();

  if (millis() - last_execution < _period) {
    return false;
  }

  last_execution = millis();
  return true;
}

void CarParkController::read_sensors() {
  if (_current_state == OFF || _current_state == WAIT_CONFIGURATION) {
    return;
  }

  _distance = _ps->getDistance();
  Serial.print(F("[CONTROLLER] Distance: "));
  Serial.println(_distance);
}

void CarParkController::fsm_loop()
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

        _free->switchOn();
        _busy->switchOn();
      }
      break;

    case OFF:
      Serial.println(F("[CONTROLLER] State: OFF"));
      _free->switchOff();
      _busy->switchOff();
      break;

    case FREE:
      Serial.println(F("[CONTROLLER] State: FREE"));
      _free->switchOn();

      if (_distance <= _parking_distance && _distance != 0) {
        Serial.println(F("[CONTROLLER] State: FREE --> BUSY"));

        _free->switchOff();
        _current_state = BUSY;
        _busy->switchOn();

        this->send_mqtt_data();
      }
      break;

    case BUSY:
      Serial.println(F("[CONTROLLER] State: BUSY"));
      _busy->switchOn();

      if (_distance > _parking_distance && !_is_away) {
        Serial.println(F(" > Vehicle started moving away"));

        _free_for = millis();
        _is_away = true;
      } else if (_distance <= _parking_distance && _is_away) {
        Serial.println(F(" > Vehicle returned inside car park"));

        _free_for = millis();
        _is_away = false;
      }

      if (_is_away && (millis() - _free_for) >= _free_time) {
        Serial.println(F("[CONTROLLER] State: BUSY --> FREE"));

        _busy->switchOff();
        _current_state = FREE;
        _free->switchOn();

        this->send_mqtt_data();
      }
      break;
  }
}

void CarParkController::send_mqtt_data()
{
  bool state = _current_state == BUSY ? true : false;
  String status = _current_state == OFF ? "off" : "on";

  Serial.print(F("[CONTROLLER] Sending data: "));

  StaticJsonDocument<JSON_OBJECT_SIZE(4)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  doc[JSON_KEY_CAR_PARK_BUSY] = state;
  doc[JSON_KEY_CAR_PARK_ON_OFF] = status.c_str();

  String payload;
  serializeJson(doc, payload);
  Serial.println(payload);

  _mqtt_writer.connect().publish(_topic_values, payload);
}

void CarParkController::onMessageReceived(const String &topic, const String &payload)
{
  Serial.print(F("[CONTROLLER] "));
  Serial.print(topic);
  Serial.print(F(": "));
  Serial.println(payload);

  // deserialize the JSON object
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

    // Read thresholds
    float parking_distance = doc["distParking"].as<float>();
    long free_time = doc["freeTime"].as<long>();

    // Use config values if greather than 0, otherwise use defaults
    if (parking_distance > 0) _parking_distance = parking_distance;
    if (free_time > 0) _free_time = free_time;

    Serial.println(F("[CONTROLLER] Received configuration:"));
    Serial.print(F(" > Commands topic: "));
    Serial.println(_topic_commands);
    Serial.print(F(" > Values topic: "));
    Serial.println(_topic_values);
    Serial.print(F(" > Parking distance: "));
    Serial.println(_parking_distance);
    Serial.print(F(" > Free time: "));
    Serial.println(_free_time);

    _current_state = FREE;
    _busy->switchOff();
    _free->switchOff();
  } else if (topic == _topic_commands) {
    const String status = doc["command"].as<String>();

    if (status == "on") {
      _current_state = FREE;
    } else if (status == "off") {
      _current_state = OFF;
    }  else {
      Serial.println(F("[CONTROLLER] Payload not recognized. Message skipped."));
    }
  } else {
    Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
  }
}

void CarParkController::device_payload(String &destination)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
  doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
  doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
  serializeJson(doc, destination);
}
