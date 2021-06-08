#ifndef __MQTT_WRAPPER__
#define __MQTT_WRAPPER__

#include <MQTT.h>
#include <ESP8266WiFi.h>
#include <set>

#include "mqtt_receiver.h"

#define MQTT_MESSAGE_BUFFER_SIZE 512

struct subscribe_packet {
  const String topic;
  const int qos;
};

struct subscribe_packet_comparer {
  bool operator()(const subscribe_packet& s, const subscribe_packet& q) {
    return s.topic.compareTo(q.topic) || s.qos < q.qos;
  }
};

class MqttWrapper
{
public:
  MqttWrapper(const String& _tag, const String &mqttBrokerIp, const String &mqttClientId, const String &mqttUsername, const String &mqttPassword);

  MqttWrapper& begin();
  MqttWrapper& connect(bool resubscribe = false);
  MqttWrapper& reconnect();
  MqttWrapper& subscribe(const String topic, int qos = 0);
  MqttWrapper& publish(const String topic, const String message, bool retained = false, int qos = 0);
  MqttWrapper& setLastWill(const String topic, const String payload);
  MqttWrapper& onMessageReceived(MqttReceiver *object, MqttCallback callback);
  MqttWrapper& listen();

private:
  WiFiClient _client;
  MQTTClient _mqttClient;
  const String _tag;
  const String _mqttBrokerIp;
  const String _mqttClientId;
  const String _mqttUsername;
  const String _mqttPassword;
  const uint _port = 1883;

  std::set<subscribe_packet, subscribe_packet_comparer> _subscribed_topics;
};

#endif // __MQTT_WRAPPER__
