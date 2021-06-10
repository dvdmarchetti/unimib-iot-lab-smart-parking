#pragma once

// Json device type recognized by master
#define DEVICE_LIGHT_TYPE "light"
#define DEVICE_CAR_PARK_TYPE "car-park"
#define DEVICE_DISPLAY_TYPE "display"
#define DEVICE_INTRUSION_TYPE "intrusion"
#define DEVICE_ROOF_TYPE "roof"
#define DEVICE_DISPLAY_TYPE "rfid"
#define DEVICE_DISPLAY_TYPE "gate"

// Mqtt
#define MQTT_BROKERIP "149.132.178.180"
#define DEVICE_MAC_ADDRESS "ee94450e4c6f4d16adb121f750df5fd4"

#define MQTT_CLIENTID_WRITER DEVICE_MAC_ADDRESS "_writer"
#define MQTT_CLIENTID_READER DEVICE_MAC_ADDRESS "_reader"

#define MQTT_TOPIC_GLOBAL_CONFIG "smpk/configurations"
#define MQTT_TOPIC_DEVICE_CONFIG MQTT_TOPIC_GLOBAL_CONFIG "/" // DEVICE_MAC_ADDRESS
#define MQTT_LAST_WILL_PREFIX "smpk/will/"
#define MQTT_DEVICE_SUB_TOPIC_PREFIX "smpk/devices/"

// MySQL
#define MYSQL_HOST {149,132,178,180}
#define MYSQL_DB "mvincenzi14"

// Influx
#define INFLUXDB_URL "http://149.132.178.180:8086"
#define INFLUXDB_ORG "labiot-org"
#define INFLUXDB_BUCKET "dmarchetti8-bucket"
#define INFLUX_DATA_POINT_LABEL "Smart Parking"

// Push values
#define PUSH_LIGHT_VALUES_PERIOD 300000 // 5 minutes.

// Telegram
#define BOT_MTBS 1000 // mean time between scan messages
#define ALARM_ON_COMMAND "/alarm\_on"
#define ALARM_OFF_COMMAND "/alarm\_off"
#define AVAILABILITY_COMMAND "/availability"
#define REGISTER_CARD_COMMAND "/register_card"
#define PARKING_INFO_COMMAND "/parking\_info"
#define NOTIFICATIONS_ON_COMMAND "/notifications\_on"
#define NOTIFICATIONS_OFF_COMMAND "/notifications\_off"
#define HELP_COMMAND "/help"

#define ALARM_ON_COMMAND_DESCRPTION "switch alarm ON"
#define ALARM_OFF_COMMAND_DESCRPTION "switch alarm OFF"
#define AVAILABILITY_COMMAND_DESCRPTION "car-park availability"
#define REGISTER_CARD_COMMAND_DESCRPTION "information about registration of new RFID cards"
#define PARKING_INFO_COMMAND_DESCRPTION "information parking status"
#define NOTIFICATIONS_ON_COMMAND_DESCRPTION "subscribe to channel's push notifications"
#define NOTIFICATIONS_OFF_COMMAND_DESCRPTION "unsubscribe to channel's push notifications"

#define NOTIFICATION_INTRUSION_MESSAGE "Intrusion detected --> Alarm on!"
#define ROOF_CLOSED_MESSAGE "Rooftop window has been closed!"