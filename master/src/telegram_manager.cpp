#include "../include/telegram_manager.h"

TelegramManager::TelegramManager() : _bot(BOT_TOKEN, _secured_client)
{
    //
}

void TelegramManager::setup() {
    _secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    configTime(0, 0, "pool.ntp.org");                     // get UTC time via NTP
    time_t now = time(nullptr);

    while (now < 24 * 3600) {
        Serial.print(".");
        delay(100);
        now = time(nullptr);
    }
    Serial.print("[TELEGRAM MANAGER] ");
    Serial.println(now);
}

void TelegramManager::onMessageReceived(TelegramReceiver *object, TelegramMsgCallback callback) {
    _callback = std::bind(callback, object, std::placeholders::_1);
}

void TelegramManager::listen() [
    // Listen, handle messages and call callaback.
    String message;
    _callback(message);
]