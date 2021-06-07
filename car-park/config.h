#pragma once

#define PIN_TRIG D1
#define PIN_ECHO D2

#define PIN_LED_FREE D5
#define PIN_LED_BUSY D6

#define CAR_PARK_PERIOD 1000

// Json payloads
#define JSON_KEY_DEVICE_MAC "mac"
#define JSON_KEY_DEVICE_TYPE "type"

#define JSON_KEY_CAR_PARK_BUSY "busy"
#define JSON_KEY_CAR_PARK_ON_OFF "status"

// Mqtt
#define DEVICE_MAC_ADDRESS "38:2B:78:03:88:79"
#define DEVICE_TYPE "car-park"

#define MQTT_CLIENTID_WRITER DEVICE_MAC_ADDRESS "_writer"
#define MQTT_CLIENTID_READER DEVICE_MAC_ADDRESS "_reader"

#define MQTT_TOPIC_GLOBAL_CONFIG "smpk/configurations"
#define MQTT_TOPIC_DEVICE_CONFIG MQTT_TOPIC_GLOBAL_CONFIG "/" DEVICE_MAC_ADDRESS
#define MQTT_TOPIC_DEVICE_LAST_WILL "smpk/will/" DEVICE_MAC_ADDRESS
