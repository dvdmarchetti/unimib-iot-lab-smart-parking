#include "../include/wifi_manager.h"

WiFiManager::WiFiManager()
{
  //
}

void WiFiManager::begin()
{
  WiFi.mode(WIFI_STA);
}

void WiFiManager::print_wifi_status()
{
  Serial.println(F("\n=== [WiFi Manager] WiFi connection status ==="));

  // SSID
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // signal strength
  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(WiFi.RSSI());
  Serial.println(F(" dBm"));

  // current IP
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  // subnet mask
  Serial.print(F("Subnet mask: "));
  Serial.println(WiFi.subnetMask());

  // gateway
  Serial.print(F("Gateway IP: "));
  Serial.println(WiFi.gatewayIP());

  // DNS
  Serial.print(F("DNS IP: "));
  Serial.println(WiFi.dnsIP());

  Serial.println(F("==============================\n"));
}

void WiFiManager::ensure_wifi_is_connected()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("[WiFi] Connecting to SSID: "));
    Serial.print(_ssid);
    Serial.print(F(" "));

    WiFi.begin(_ssid.c_str(), _pass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(150);
    }
    Serial.print(F("\n[WiFi] Connected! Device address: "));
    Serial.println(WiFi.localIP());
  }
}
