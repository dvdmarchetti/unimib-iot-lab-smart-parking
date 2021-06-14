#ifndef __INTRUSION_CONTROLLER__
#define __INTRUSION_CONTROLLER__

#include "../config.h"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "wifi_manager.h"
#include "movement_sensor.h"
#include "buzzer.h"
#include "led.h"

class IntrusionController : public MqttReceiver
{
public:
    IntrusionController(int period);

    void setup();
    void loop();

private:
    // Services
    WiFiManager _wifi_manager;
    MqttWrapper _mqtt_reader;
    MqttWrapper _mqtt_writer;
    Buzzer _buzzer;
    MovementSensor _ms;
    Led _led_config;
    Led _led_info;

    // State
    enum State { WAIT_CONFIGURATION, INTRUSION, ARMING, ARMED, OFF };
    State _current_state = WAIT_CONFIGURATION;
    bool _arming = false;
    unsigned long _arming_time;

    // Dynamic configuration
    bool _has_requested_configuration = false;
    String _topic_commands;
    String _topic_values;

    // Sensor readings
    bool _presence = false;
    int _period;

    void setup_mqtt();
    void onMessageReceived(const String &topic, const String &payload);

    void read_sensors();
    void fsm_loop();
    bool is_enabled();

    void send_mqtt_data();

    void device_payload(String &destination);
};

#endif // __LIGHT_CONTROLLER__
