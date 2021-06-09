#include "../include/mqtt_wrapper.h"

MqttWrapper::MqttWrapper(
  const String &tag,
  const String &mqttBrokerIp,
  const String &mqttClientId,
  const String &mqttUsername,
  const String &mqttPassword
) : _tag(tag),
  _mqttClient(MQTT_MESSAGE_BUFFER_SIZE),
  _mqttBrokerIp(mqttBrokerIp),
  _mqttClientId(mqttClientId),
  _mqttUsername(mqttUsername),
  _mqttPassword(mqttPassword)
{
  //
}

MqttWrapper& MqttWrapper::begin()
{
  _mqttClient.begin(_mqttBrokerIp.c_str(), _port, _client);

  return *this;
}

MqttWrapper& MqttWrapper::connect(bool resubscribe)
{
  if (! _mqttClient.connected()) {
    Serial.print(F("[MQTT] "));
    Serial.print(_tag);
    Serial.print(F(": Connecting to broker"));
    while (!_mqttClient.connect(_mqttClientId.c_str(), _mqttUsername.c_str(), _mqttPassword.c_str())) {
      Serial.print(F("."));
      delay(1000);
    }
    Serial.println(F(" (OK)"));

    if (resubscribe) {
      for (auto subscription : _subscribed_topics) {
        this->subscribe(subscription.topic, subscription.qos);
      }
    }
  }

  return *this;
}

MqttWrapper& MqttWrapper::reconnect()
{
  this->connect(true);

  return *this;
}

MqttWrapper& MqttWrapper::enqueueSubscribeMessage(const String topic, int qos)
{
  _subscribe_queue.push(subscribe_packet{topic, qos});
  return *this;
}

MqttWrapper& MqttWrapper::subscribe(const String topic, int qos)
{
  _subscribed_topics.insert(subscribe_packet{topic, qos});
  auto result = _mqttClient.subscribe(topic, qos);

  Serial.print(F("[MQTT] Topic subscription: "));
  Serial.print(topic);
  Serial.print(F(" ("));
  Serial.print(result == 1 ? F("OK") : F("FAIL"));
  Serial.println(F(")"));

  return *this;
}

MqttWrapper& MqttWrapper::subscribeAll()
{
  this->connect();
  while (!_subscribe_queue.empty())
  {
    subscribe_packet packet = _subscribe_queue.front();
    _subscribe_queue.pop();

    Serial.println("Processing publish item:");
    Serial.print(packet.topic);
    Serial.print(" - ");
    Serial.println(packet.qos);

    this->subscribe(packet.topic, packet.qos);
  }
}

MqttWrapper& MqttWrapper::enqueuePublishMessage(const String topic, const String message, bool retained, int qos) 
{
  _publish_queue.push(publish_packet{topic, message, retained, qos});
  return *this;
}

MqttWrapper& MqttWrapper::publish(const String topic, const String msg, bool retained, int qos) 
{
  _mqttClient.publish(topic, msg, retained, qos);
}

MqttWrapper& MqttWrapper::publishAll() 
{
  this->connect();
  while (!_publish_queue.empty())
  {
    publish_packet packet = _publish_queue.front();
    _publish_queue.pop();

    Serial.println("Processing publish item:");
    Serial.print(packet.topic);
    Serial.print(" - ");
    Serial.println(packet.msg);

    this->publish(packet.topic, packet.msg, packet.retained, packet.qos);
  }
}

MqttWrapper& MqttWrapper::onMessageReceived(MqttReceiver *object, MqttCallback callback)
{
  _mqttClient.onMessage(std::bind(callback, object, std::placeholders::_1, std::placeholders::_2));

  return *this;
}

MqttWrapper& MqttWrapper::setLastWill(const String topic, const String payload)
{
  _mqttClient.setWill(topic.c_str(), payload.c_str());

  return *this;
}

MqttWrapper& MqttWrapper::listen()
{
  _mqttClient.loop();

  return *this;
}
