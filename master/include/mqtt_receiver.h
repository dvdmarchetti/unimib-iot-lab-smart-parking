#ifndef __RECEIVER__
#define __RECEIVER__

#include <Arduino.h>

class MqttReceiver
{
public:
  virtual ~MqttReceiver() {};
  virtual void onMessageReceived(const String &topic, const String &payload) = 0;
};

typedef void (MqttReceiver::*MqttCallback)(const String &topic, const String &payload);

#endif
