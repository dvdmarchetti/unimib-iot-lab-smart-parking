#ifndef __CARPARKCONTROLLER__
#define __CARPARKCONTROLLER__

#include "../config.h"
#include "../secrets.h"

#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "wifi_manager.h"
#include "proximity_sensor.h"
#include "led.h"

class CarParkController : public MqttReceiver
{
public:
    CarParkController(int period);

    void setup();
    void loop();

private:
    // Services
    WiFiManager _wifi_manager;
    MqttWrapper _mqtt_reader;
    MqttWrapper _mqtt_writer;

    // Devices
    ProximitySensor *_ps;
    Led *_busy;
    Led *_free;

    // State
    enum State { WAIT_CONFIGURATION, OFF, FREE, BUSY };
    State _current_state = WAIT_CONFIGURATION;

    // Dynamic Configuration
    bool _has_requested_configuration = false;
    String _topic_commands;
    String _topic_values;
    float _parking_distance = 0.1; // 10 cm.
    long _free_time = 15000; // 15 sec.

    // Sensor readings
    unsigned long _free_for;
    bool _is_away = false;
    long _time;
    float _distance = 100;
    int _period;

    void setup_mqtt();
    void onMessageReceived(const String &topic, const String &payload);

    bool is_enabled();
    void read_sensors();
    void fsm_loop();

    void send_mqtt_data();

    void device_payload(String &destination);
};

#endif
