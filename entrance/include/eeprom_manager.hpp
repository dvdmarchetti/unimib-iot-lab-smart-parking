#ifndef __EEPROM_MANAGER__
#define __EEPROM_MANAGER__

#include <EEPROM.h>

template<typename T>
class EEPROMManager
{
public:
  void begin(uint capacity) {
    _capacity = capacity;
    EEPROM.begin(capacity);
  }

  void read(uint addr, T &payload) {
    EEPROM.get(addr, payload);
  }

  void write(uint addr, T payload) {
    Serial.print(F("[EEPROM] Writing EEPROM"));
    EEPROM.put(addr, payload);
    EEPROM.commit();
  }

  void reset() {
    for (int i = 0; i < _capacity; i++) {
      EEPROM.write(i, 0);
    }

    EEPROM.commit();
    delay(500);
  }

private:
  uint _capacity;
};

#endif // __EEPROM_MANAGER__
