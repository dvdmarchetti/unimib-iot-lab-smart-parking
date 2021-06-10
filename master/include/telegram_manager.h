#ifndef __TELEGRAM_MANAGER__
#define __TELEGRAM_MANAGER__

#include <WiFi.h>
#include <functional>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include <set>

#include "../config.h"
#include "wifi_manager.h"
#include "telegram_receiver.h"

class TelegramManager {

public:
  TelegramManager();

  TelegramManager& setup();
  TelegramManager& onMessageReceived(TelegramReceiver *object, TelegramMsgCallback callback);
  TelegramManager& listen();
  TelegramManager& sendNotification(std::set<String> ids, const String &message, const String &parseMode);
  TelegramManager& sendMessage(const String &chat_id, const String &message, const String &parseMode);
  TelegramManager& sendMessageWithReplyKeyboard(const String &chat_id, const String &message, const String &parseMode, bool resize = false, bool oneTime = false, bool selective = false);

private:
  WiFiManager _wifi_manager;
  WiFiClientSecure _secured_client;
  UniversalTelegramBot _bot;

  unsigned long _bot_lasttime;
  std::function<void(const String &, const String &, const String &)> _callback;

  void handle_messages(int newMessages);
  String build_reply_keyboard();
};

#endif // __TELEGRAM_MANAGER__
