#ifndef MASTER_CONTROLLER_HPP_
#define MASTER_CONTROLLER_HPP_

#include <ArduinoJson.h>
#include <WebServer.h>
#include <vector>
#include <map>

#include "events.h"
#include "influx_wrapper.h"
#include "wifi_manager.h"
#include "mysql_wrapper.h"
#include "mqtt_receiver.h"
#include "mqtt_wrapper.h"
#include "telegram_manager.h"
#include "telegram_receiver.h"

#include "../dashboard.h"

class MasterController : public MqttReceiver, public TelegramReceiver
{
public:
    MasterController();

    void setup();
    void loop();

private:

    // Wrappers
    WebServer _server;
    MqttWrapper _mqtt_reader;
    MqttWrapper _mqtt_writer;
    WiFiManager _wifi_manager;
    TelegramManager _telegram_manager;

    // Internal status
    std::map<String, std::vector<uint>> _light_values;
    std::map<String, uint> _light_status;
    std::map<String, bool> _car_park_busy;
    std::map<String, String> _car_park_status;
    std::map<String, String> _display_status;
    std::map<String, uint> _alarm_status;
    std::map<String, bool> _intrusion_detected;
    std::map<String, uint> _roof_status;

    unsigned long _last_push;
    String _display_first_row = "~Smart Parking~", _display_second_row = "Available: {{slots}}";

    // Mqtt methods
    void setup_mqtt();
    void onMessageReceived(const String &topic, const String &payload);

    // Web server methods
    void register_routes();
    void handle_cors();
    void handle_root();
    void handle_status_get();
    void handle_display_post();
    void handle_lights_post();
    void handle_parking_slot_post();

    void send_error_response(const char* error);

    // DB access methods
    boolean getConfiguration(StaticJsonDocument<512> &config, String type);
    boolean getDeviceConfiguration(StaticJsonDocument<512> &config, String mac_address, String type);
    void masterLoop();

    // Utils for display rows
    void substituteDisplayLine(String &out, String in);

    //Telegram
    void onTelegramMessageReceived(const String &chat_id, const String &message, const String &from);
};

#endif // MASTER_CONTROLLER_HPP_
