#ifndef __TELEGRAM_MANAGER__
#define __TELEGRAM_MANAGER__

#include <WiFi.h>
#include <functional>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "wifi_manager.h"
#include "telegram_receiver.h"

class TelegramManager {

public:
    TelegramManager();

    void setup();
    void onMessageReceived(TelegramReceiver *object, TelegramMsgCallback callback);
    void listen();

private:
    WiFiManager _wifi_manager;
    WiFiClientSecure _secured_client;
    UniversalTelegramBot _bot;

    unsigned long _bot_lasttime;
    std::function<void(const String &)> _callback;
};

#endif // __TELEGRAM_MANAGER__
