#include "../include/rfid_reader.h"

RfidReader::RfidReader(const uint8_t &ss_pin, const uint8_t &rst_pin) :
  _reader(ss_pin, rst_pin)
{
  SPI.begin();
  _reader.PCD_Init();
}

void RfidReader::setup()
{
  //
}

void RfidReader::loop()
{
  if (! _reader.PICC_IsNewCardPresent() || ! _reader.PICC_ReadCardSerial()) {
    return;
  }

  String serial;
  dump_byte_array(_reader.uid.uidByte, _reader.uid.size, serial);
  Serial.print("[RFID] CardID=");
  Serial.println(serial);

  _callback(serial);
}

void RfidReader::halt()
{
  _reader.PICC_HaltA();
}

void RfidReader::onCardAvailable(RfidReceiver *object, RfidCallback callback)
{
  _callback = std::bind(callback, object, std::placeholders::_1);
}

void RfidReader::dump_byte_array(byte *buffer, byte buffer_size, String &serial)
{
  for (byte i = 0; i < buffer_size; i++) {
    serial.concat(buffer[i] < 0x10 ? "0" : "");
    serial.concat(String(buffer[i], HEX));
  }
}
