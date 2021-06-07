#ifndef __ROOFCONTROLLER__
#define __ROOFCONTROLLER__

#include "servo_motor.h"
#include "mqtt_wrapper.h"
#include "wifi_manager.h"
#include "mqtt_receiver.h"
#include "../config.h"

class RoofController : public MqttReceiver
{
public:
    RoofController();

    void setup();
    void loop();

private:
    // Services
    WiFiManager _wifi_manager;
    MqttWrapper _mqtt_reader;
    MqttWrapper _mqtt_writer;
    ServoMotor _servo;

    // State
    enum State
    { WAIT_CONFIGURATION, OPEN, CLOSE };
    State _current_state = WAIT_CONFIGURATION;

    // Dynamic configuration
    bool _has_requested_configuration = false;
    String _topic_commands;

    void setupMqtt();
    void onMessageReceived(const String &topic, const String &payload);

    void fsm_loop();

    void send_mqtt_data();

    void device_payload(String &destination);
};

#endif // __MQTT_WRAPPER__
