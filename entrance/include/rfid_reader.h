#ifndef __RFID_READER__
#define __RFID_READER__

#include <MFRC522.h>
#include <functional>
#include "rfid_receiver.h"

class RfidReader
{
public:
  RfidReader(const uint8_t &ss, const uint8_t &rst);

  void setup();
  void loop();
  void halt();
  void onCardAvailable(RfidReceiver *object, RfidCallback callback);

private:
  MFRC522 _reader;
  MFRC522::MIFARE_Key _key;
  std::function<void(const String&)> _callback;

  void dump_byte_array(byte *buffer, byte buffer_size, String &serial);
};

#endif // __RFID_READER__
