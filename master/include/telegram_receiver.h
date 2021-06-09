#ifndef __TELEGRAM_RECEIVER__
#define __TELEGRAM_RECEIVER__

#include <Arduino.h>
#include <functional>

class TelegramReceiver
{
public:
    virtual ~TelegramReceiver(){};
    virtual void onTelegramMessageReceived(const String &message) = 0;
};

// typedef std::function<void(const String&)> RfidCallback;

typedef void (TelegramReceiver::*TelegramMsgCallback)(const String &message);

#endif // __TELEGRAM_RECEIVER__