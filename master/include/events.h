#ifndef __EVENTS__
#define __EVENTS__

#define DASHBOARD_EVENT_CATEGORY "DASHBOARD"
#define DEVICE_EVENT_CATEGORY "DEVICE"
#define TELEGRAM_EVENT_CATEGORY "TELEGRAM"

#define DEVICE_CONNECTED_EVENT "Device with mac %s of type %s connected"
#define DEVICE_DISCONNECTED_EVENT "Device with mac %s of type %s disconnected"
#define DEVICE_INTRUSION_DETECTED "Intrusion has been detected"
#define DEVICE_RFID_ACCESS_DENIED "Access denied to card %s"

#define DASHBOARD_COMMAND_EVENT "Command %s=%s sent from dashboard"

#define TELEGRAM_COMMAND_EVENT "Command %s=%s sent from telegram"

#endif
