#include "../include/photoresistor.h"

Photoresistor::Photoresistor(
    uint8_t input_pin,
    uint cooldown
) : _input_pin(input_pin), _cooldown(cooldown) {
    //
}

uint Photoresistor::read() {
    static ulong last_print = 0;
    static uint last_value = 0;
    static ulong last_read = 0;

    if (millis() - last_read < _cooldown) {
        return last_value;
    }

    last_read = millis();
    last_value = analogRead(_input_pin);

    if (millis() - last_print > 3000) {
        last_print = millis();
        Serial.print(F("[LIGHT] Light value: "));
        Serial.print(last_value);
        Serial.println(F("/1024"));
    }

    return last_value;
}
