#ifndef WIFI_MANAGER_HPP_
#define WIFI_MANAGER_HPP_

#include <WiFi.h>
#include "../secrets.h"

class WiFiManager
{
public:
  WiFiManager();

  void begin();
  void ensure_wifi_is_connected();
  void print_wifi_status();

private:
  const String _ssid = SECRET_SSID;
  const String _pass = SECRET_PASS;
};

#endif // WIFI_HPP_
