#ifndef __RFID_RECEIVER__
#define __RFID_RECEIVER__

#include <Arduino.h>
#include <functional>

class RfidReceiver
{
public:
  virtual ~RfidReceiver() {};
  virtual void onCardAvailable(const String &serial) = 0;
};

typedef void (RfidReceiver::*RfidCallback)(const String &serial);

#endif // __RFID_RECEIVER__
