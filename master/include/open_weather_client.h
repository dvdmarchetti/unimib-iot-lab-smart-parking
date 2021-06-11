#ifndef __OPENWEATHER_CLIENT__
#define __OPENWEATHER_CLIENT__

#include <WiFi.h>
#include <ArduinoJson.h>

#include "../config.h"
#include "../secrets.h"
#include "open_weather_receiver.h"

class OpenWeatherClient {

public:
    OpenWeatherClient();

    OpenWeatherClient& setup(String city, String country);
    OpenWeatherClient& onMessageReceived(OpenWeatherReceiver *object, OpenWeatherCallback callback);
    OpenWeatherClient& pull();

private:
    WiFiClient _wifi_client;

    String _weather_server = "api.openweathermap.org";
    String _weather_query = "GET /data/2.5/weather?q=%s,%s&units=metric&APPID=%s";

    String _city;
    String _country;

    unsigned long _last_pull;
    std::function<void(StaticJsonDocument<1024> &doc)> _callback;

    void printWeatherInformation(StaticJsonDocument<1024> &doc);
};

#endif //__OPENWEATHER_CLIENT__
