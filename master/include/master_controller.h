#ifndef MASTER_CONTROLLER_HPP_
#define MASTER_CONTROLLER_HPP_

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
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
#include "open_weather_client.h"
#include "open_weather_receiver.h"

#include "../dashboard.h"

class MasterController : public MqttReceiver, public TelegramReceiver, public OpenWeatherReceiver
{
public:
  MasterController();

  void setup();
  void loop();

private:
  // Wrappers
  AsyncWebServer _server;
  AsyncWebSocket _ws;
  MqttWrapper _mqtt_reader;
  MqttWrapper _mqtt_writer;
  WiFiManager _wifi_manager;
  TelegramManager _telegram_manager;
  OpenWeatherClient _ow_client;

  // Internal status
  std::map<String, std::vector<uint>> _light_values;
  std::map<String, uint> _light_status;

  std::map<String, bool> _car_park_busy;
  std::map<String, String> _car_park_status;

  std::map<String, String> _display_status;
  std::map<String, uint> _alarm_status;
  std::map<String, bool> _intrusion_detected;
  std::map<String, uint> _roof_status;

  std::map<String, uint> _rfid_status; // 0 -> OFF / 1 -> ON / 2 -> MANAGE
  std::map<String, uint> _gate_status; // 0 -> CLOSED / 1 -> OPEN

  // Notification chat_id set
  std::set<String> _id_to_notify;

  unsigned long _last_push;

  // Mqtt methods
  void setup_mqtt();
  void onMessageReceived(const String &topic, const String &payload);

  // WebSocket methods
  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

  // Web server methods
  // void register_routes();
  // void handle_cors();
  // void handle_root();
  // void handle_status_get();
  // void handle_lights_post();
  // void handle_parking_slot_post();

  void send_error_response(const char* error);

  // DB access methods
  boolean getConfiguration(StaticJsonDocument<512> &config, String type);
  boolean getDeviceConfiguration(StaticJsonDocument<512> &config, String mac_address, String type);
  void masterLoop();

  //Telegram
  void onTelegramMessageReceived(const String &chat_id, const String &message, const String &from);

  // OpenWeather
  void onWeatherReceived(StaticJsonDocument<1024> &doc);

  // Utils
  void sendCommandToRoof(int command);
};

#endif // MASTER_CONTROLLER_HPP_
