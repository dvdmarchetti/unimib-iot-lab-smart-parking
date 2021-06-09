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

// Json device type recognized by master
#define DEVICE_LIGHT_TYPE "light"
#define DEVICE_CAR_PARK_TYPE "car-park"
#define DEVICE_DISPLAY_TYPE "display"

// Mqtt
#define DEVICE_MAC_ADDRESS "38:2B:78:03:88:79"
#define DEVICE_TYPE "car-park"

#define MQTT_CLIENTID_WRITER DEVICE_MAC_ADDRESS "_writer"
#define MQTT_CLIENTID_READER DEVICE_MAC_ADDRESS "_reader"

#define MQTT_TOPIC_GLOBAL_CONFIG "smpk/configurations"
#define MQTT_TOPIC_DEVICE_CONFIG MQTT_TOPIC_GLOBAL_CONFIG "/" // DEVICE_MAC_ADDRESS
#define MQTT_LAST_WILL_PREFIX "smpk/will/"
#define MQTT_TOPIC_DEVICE_LAST_WILL MQTT_LAST_WILL_PREFIX DEVICE_MAC_ADDRESS
#define MQTT_DEVICE_SUB_TOPIC_PREFIX "smpk/devices/"

// Influx
#define INFLUX_DATA_POINT_LABEL "Smart Parking"

// Push values
#define PUSH_LIGHT_VALUES_PERIOD 300000 // 5 minutes.

// Telegram
#define BOT_MTBS 1000 // mean time between scan messages