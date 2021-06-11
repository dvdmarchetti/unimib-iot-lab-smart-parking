#include "../include/open_weather_client.h"

OpenWeatherClient::OpenWeatherClient() 
{
    //
}

OpenWeatherClient& OpenWeatherClient::setup(String city, String country)
{
    _city = city;
    _country = country;

    _last_pull = millis();

    return *this;
}

OpenWeatherClient& OpenWeatherClient::onMessageReceived(OpenWeatherReceiver *object, OpenWeatherCallback callback)
{
    _callback = std::bind(callback, object, std::placeholders::_1);
    return *this;
}

OpenWeatherClient& OpenWeatherClient::pull()
{
    if (millis() - _last_pull < OPEN_WEATHER_PULL_PERIOD)
        return *this;

    if (_wifi_client.connect(_weather_server.c_str(), 80)) {
        char request[100];
        sprintf(request, _weather_query.c_str(), _city, _country, WEATHER_API_KEY);

        _wifi_client.println(request);
        _wifi_client.println(F("Host: api.openweathermap.org"));
        _wifi_client.println(F("User-Agent: ArduinoWiFi/1.1"));
        _wifi_client.println(F("Connection: close"));
        _wifi_client.println();
    } else {
        Serial.println(F("[OPENWEATHER CLIENT] Connection to api.openweathermap.org failed!\n"));
    }

    // Waiting for API response
    while (_wifi_client.connected() && !_wifi_client.available()) delay(1);

    String result;
    while (_wifi_client.connected() || _wifi_client.available()) {
        char c = _wifi_client.read();
        result = result + c;
    }

    _wifi_client.stop();

    char jsonArray[result.length() + 1];
    result.toCharArray(jsonArray, sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';

    StaticJsonDocument<1024> jsonResponse;
    DeserializationError error = deserializeJson(jsonResponse, jsonArray);

    if (error) {
        Serial.print(F("[OPENWEATHER CLIENT] deserializeJson() failed: "));
        Serial.println(error.c_str());
        return *this;
    }

    printWeatherInformation(jsonResponse);

    _callback(jsonResponse);

    _last_pull = millis();
    return *this;
}

void OpenWeatherClient::printWeatherInformation(StaticJsonDocument<1024> doc)
{
    Serial.println(F("=== [OPENWEATHER CLIENT] Current weather ==="));

    Serial.print(F("Location: "));
    Serial.println(doc["name"].as<String>());
    Serial.print(F("Country: "));
    Serial.println(doc["sys"]["country"].as<String>());
    Serial.print(F("Temperature (°C): "));
    Serial.println((float)doc["main"]["temp"]);
    Serial.print(F("Humidity (%): "));
    Serial.println((float)doc["main"]["humidity"]);
    Serial.print(F("Weather: "));
    Serial.println(doc["weather"][0]["main"].as<String>());
    Serial.print(F("Weather description: "));
    Serial.println(doc["weather"][0]["description"].as<String>());
    Serial.print(F("Pressure (hPa): "));
    Serial.println((float)doc["main"]["pressure"]);
    Serial.print(F("Sunrise (UNIX timestamp): "));
    Serial.println((float)doc["sys"]["sunrise"]);
    Serial.print(F("Sunset (UNIX timestamp): "));
    Serial.println((float)doc["sys"]["sunset"]);
    Serial.print(F("Temperature min. (°C): "));
    Serial.println((float)doc["main"]["temp_min"]);
    Serial.print(F("Temperature max. (°C): "));
    Serial.println((float)doc["main"]["temp_max"]);
    Serial.print(F("Wind speed (m/s): "));
    Serial.println((float)doc["wind"]["speed"]);
    Serial.print(F("Wind angle: "));
    Serial.println((float)doc["visibility"]);
    Serial.print(F("Visibility (m): "));
    Serial.println((float)doc["wind"]["deg"]);

    Serial.println(F("==============================\n"));
}