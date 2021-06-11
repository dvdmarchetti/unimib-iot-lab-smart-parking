#ifndef __OPENWEATHER_RECEIVER__
#define __OPENWEATHER_RECEIVER__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

class OpenWeatherReceiver
{
public:
    virtual ~OpenWeatherReceiver(){};
    virtual void onWeatherReceived(StaticJsonDocument<1024> &doc) = 0;
};

typedef void (OpenWeatherReceiver::*OpenWeatherCallback)(StaticJsonDocument<1024> &doc);

#endif // __OPENWEATHER_RECEIVER__
