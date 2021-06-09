#include "../include/telegram_manager.h"

TelegramManager::TelegramManager() : _bot(BOT_TOKEN, _secured_client)
{
    //
}

TelegramManager& TelegramManager::setup()
{
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

    return *this;
}

TelegramManager& TelegramManager::onMessageReceived(TelegramReceiver *object, TelegramMsgCallback callback)
{
    _callback = std::bind(callback, object, std::placeholders::_1, std::placeholders::_2);
    return *this;
}

TelegramManager& TelegramManager::listen()
{
    if (millis() - _bot_lasttime < BOT_MTBS)
        return *this;

    int numNewMessages = _bot.getUpdates(_bot.last_message_received + 1);

    while (numNewMessages) {
        handle_message(numNewMessages);
        numNewMessages = _bot.getUpdates(_bot.last_message_received + 1);
    }

    _bot_lasttime = millis();
    return *this;
}

TelegramManager& TelegramManager::sendMessage(const String &chat_id, const String &message)
{
    bool isMessageSent = _bot.sendMessage(chat_id, message, "");

    if (isMessageSent) Serial.println("[TELEGRAM MANAGER] Message successfully sent");
    else Serial.println("[TELEGRAM MANAGER] Error dutring message sending");

    return *this;
}

void TelegramManager::handle_messages(int numMessages) 
{
    for (int i = 0; i < numNewMessages; i++) {
        _callback(_bot.messages[i].chat_id, _bot.messages[i].text);
    }
}