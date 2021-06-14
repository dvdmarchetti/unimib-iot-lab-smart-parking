#include "../include/intrusion_controller.h"

#include <ArduinoJson.h>

IntrusionController::IntrusionController(int period) : _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
                                             _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD)),
                                             _ms(MovementSensor(PIN_PIR)),
                                             _buzzer(Buzzer(PIN_BUZZER)),
                                             _led_config(Led(LED_ESP8266)),
                                             _led_info(Led(PIN_INFO_LED)),
                                             _period(period)

{
    Serial.println(F("[CONTROLLER] Setting up Buzzer"));
    _buzzer.soundOff();
    _led_info.switchOff();
}

void IntrusionController::setup()
{
    Serial.println(F("[CONTROLLER] Setting up Wifi"));
    _wifi_manager.begin();
    _wifi_manager.ensure_wifi_is_connected();

    this->setup_mqtt();
}

void IntrusionController::setup_mqtt()
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
        .connect()
        .subscribe(MQTT_TOPIC_DEVICE_CONFIG, 2);
}

void IntrusionController::loop()
{
    _wifi_manager.ensure_wifi_is_connected();

    if (this->is_enabled()) {
        this->read_sensors();
        this->fsm_loop();
    }

    _mqtt_reader.reconnect().listen();
    _mqtt_writer.reconnect().listen();
}

bool IntrusionController::is_enabled()
{
    static ulong last_execution = millis();

    if (millis() - last_execution < _period) {
        return false;
    }

    last_execution = millis();
    return true;
}

void IntrusionController::read_sensors()
{
    if (_current_state == OFF || _current_state == WAIT_CONFIGURATION || _current_state == ARMING) {
        return;
    }

    _presence = _ms.isMovementDetected();
    Serial.print(F("[CONTROLLER] Movement: "));
    Serial.println(_presence);
}

void IntrusionController::fsm_loop()
{
    static ulong last_blink = millis();
    static bool info_led_on = false;

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

            _led_config.switchOn();
        }
        break;

    case OFF:
        Serial.println(F("[CONTROLLER] State: OFF"));
        _buzzer.soundOff();
        _led_info.switchOff();
        break;

    case ARMING:
        Serial.println(F("[CONTROLLER] State: ARMING"));
        if (millis() - last_blink > BLINK_INTERVAL) {
            if (info_led_on) _led_info.switchOff();
            else  _led_info.switchOn();
            info_led_on = !info_led_on;
            last_blink = millis();
        }

        if (_arming && millis() - _arming_time >= PIR_SETUP_PERIOD / 2) {
            _arming = false;
            _current_state = ARMED;
            Serial.println(F("[CONTROLLER] Alarm armed..."));
        }
        break;

    case ARMED:
        Serial.println(F("[CONTROLLER] State: ARMED"));
        _buzzer.soundOff();
        _led_info.switchOn();
        info_led_on = true;
        if (_presence) {
            _current_state = INTRUSION;
            Serial.println("[CONTROLLER] State: ARMED --> INTRUSION");
            _buzzer.soundOn();

            this->send_mqtt_data();
        }
        break;

    case INTRUSION:
        Serial.println(F("[CONTROLLER] State: INTRUSION"));
        _buzzer.soundOn();

        if (millis() - last_blink > BLINK_INTERVAL) {
            if (info_led_on) _led_info.switchOff();
            else _led_info.switchOn();

            info_led_on = !info_led_on;
            last_blink = millis();
        }

        break;
    }
}

void IntrusionController::send_mqtt_data()
{
    if (_current_state == WAIT_CONFIGURATION) {
        return;
    }

    Serial.print(F("[CONTROLLER] Sending data: "));

    StaticJsonDocument<JSON_OBJECT_SIZE(5)> doc;
    doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
    doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
    doc["status"] = (_current_state == OFF) ? "off" : "on";
    doc["intrusion"] = _presence;

    String payload;
    serializeJson(doc, payload);
    Serial.println(payload);

    _mqtt_writer.connect().publish(_topic_values, payload);
}

void IntrusionController::onMessageReceived(const String &topic, const String &payload)
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

        Serial.println(F("[CONTROLLER] Received configuration:"));
        Serial.print(F(" > Commands topic: "));
        Serial.println(_topic_commands);
        Serial.print(F(" > Values topic: "));
        Serial.println(_topic_values);

        _current_state = OFF;

        _led_config.switchOff();
    } else if (topic == _topic_commands) {
        uint status = doc["command"];

        if (status == 0) {
            _current_state = OFF;
        } else if (status == 1) {
            Serial.println(F("[CONTROLLER] Alarm arming..."));
            //delay(PIR_SETUP_PERIOD / 2);
            _current_state = ARMING;
            _arming = true;
            _arming_time = millis();
        } else {
            Serial.println(F("[CONTROLLER] Payload not recognized. Message skipped."));
        }
    }
    else
    {
        Serial.println(F("[CONTROLLER] MQTT Topic not recognized. Message skipped."));
    }
}

void IntrusionController::device_payload(String &destination)
{
    StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
    doc[JSON_KEY_DEVICE_MAC] = DEVICE_MAC_ADDRESS;
    doc[JSON_KEY_DEVICE_TYPE] = DEVICE_TYPE;
    serializeJson(doc, destination);
}
