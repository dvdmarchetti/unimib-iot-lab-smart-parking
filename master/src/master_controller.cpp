#include "../include/master_controller.h"

using namespace std::placeholders;

MasterController::MasterController() : _server(80),
  _ws("/ws"),
  _mqtt_reader(MqttWrapper("Reader", MQTT_BROKERIP, MQTT_CLIENTID_READER, MQTT_USERNAME, MQTT_PASSWORD)),
  _mqtt_writer(MqttWrapper("Writer", MQTT_BROKERIP, MQTT_CLIENTID_WRITER, MQTT_USERNAME, MQTT_PASSWORD))
{
  //
}

void MasterController::setup()
{
  Serial.println(F("[CONTROLLER] Setting up Wifi"));
  _wifi_manager.begin();
  _wifi_manager.ensure_wifi_is_connected();

  Serial.println(F("[CONTROLLER] Setting up WebSocket server"));
  _ws.onEvent(std::bind(&MasterController::onEvent, this, _1, _2, _3, _4, _5, _6));
  _server.addHandler(&_ws);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  _server.begin();

  Serial.print(F("[CONTROLLER] Setting up telegram bot"));
  _telegram_manager.setup()
    .onMessageReceived(this, &TelegramReceiver::onTelegramMessageReceived);

  Serial.println(F("[CONTROLLER] Setting up OpenWeather client"));
  _ow_client.setup(WEATHER_CITY, WEATHER_COUNTRY)
      .onMessageReceived(this, &OpenWeatherReceiver::onWeatherReceived);

  this->setup_mqtt();

  _last_push = millis();
}

void MasterController::setup_mqtt() {
  Serial.println(F("[CONTROLLER] Setting up MQTT"));

  _mqtt_reader.begin()
    .onMessageReceived(this, &MqttReceiver::onMessageReceived)
    .connect()
    .subscribe(MQTT_TOPIC_GLOBAL_CONFIG, 2);

  _mqtt_writer.begin().connect();
}

void MasterController::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    Serial.println("Client connected");
    _ws_clients_id.insert(client->id());

    client->text(buildJsonCarParkState());
    } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
    _ws_clients_id.erase(client->id());
  }
}

void MasterController::loop()
{
  // Check Wifi connection
  _wifi_manager.ensure_wifi_is_connected();

  // Handle WebSocket requests
  _ws.cleanupClients();

  // Mqtt publish/subscribe
  _mqtt_reader.reconnect().subscribeAll();
  _mqtt_writer.publishAll();

  // Mqtt listen for incoming messages && Keep-alive
  _mqtt_reader.reconnect().listen();
  _mqtt_writer.reconnect().listen();

  // Telegram bot handling
  _telegram_manager.listen();

  // OpenWeather weather pull
  _ow_client.pull();

  // Master loop (Influx push)
  this->masterLoop();
}

void MasterController::masterLoop() {
  /* Mean of light values but need to be done for each MAC address. */
  if (millis() - _last_push >= PUSH_LIGHT_VALUES_PERIOD) {
    std::map<String, std::vector<uint>>::iterator it = _light_values.begin();

    int sum;
    while (it != _light_values.end()) {
      sum = 0;
      String mac = it->first;
      std::vector<uint> values = it->second;

      for (auto v : values) {
        Serial.println(v);
        sum += v;
      }

      if (sum > 0) {
        float avg = sum / values.size();

        InfluxWrapper::getInstance().checkConnection();
        InfluxWrapper::getInstance().addTag("mac", mac);
        InfluxWrapper::getInstance().addTag("type", DEVICE_LIGHT_TYPE);
        InfluxWrapper::getInstance().writeToInflux("lightValue", avg);

        it->second.clear();
      }
      it++;
    }

    _last_push = millis();
  }
}

void MasterController::onMessageReceived(const String &topic, const String &payload)
{
  // this function handles a message from the MQTT broker
  Serial.println("Incoming MQTT message: " + topic + " - " + payload);

  // deserialize the JSON object
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (topic == MQTT_TOPIC_GLOBAL_CONFIG) {
    String mac_address = doc["mac"];
    String type = doc["type"];

    Serial.print("Received mac address: ");
    Serial.println(mac_address);
    Serial.print("Received device type: ");
    Serial.println(type);

    Serial.println("Retrieve configuration...");

    MySqlWrapper::getInstance().setAutoclose(false);

    StaticJsonDocument<512> config;
    this->getDeviceConfiguration(config, mac_address, type);

    if (config.containsKey("topicToPublish")) {
      Serial.print("Subscribe to:");
      Serial.println(config["topicToPublish"].as<String>());
      _mqtt_reader.enqueueSubscribeMessage(config["topicToPublish"].as<String>(), 0);
    }

    // Subscribe to last will topic
    _mqtt_reader.enqueueSubscribeMessage(MQTT_LAST_WILL_PREFIX + mac_address, 2);

    // Publish config file in configuration topic for this device
    String clientTopicForConfig = MQTT_TOPIC_DEVICE_CONFIG + mac_address;
    String buffer;
    serializeJson(config, buffer);
    Serial.println(clientTopicForConfig);
    _mqtt_writer.enqueuePublishMessage(clientTopicForConfig, buffer, false, 2);

    if (type == DEVICE_CAR_PARK_TYPE) {
      _car_park_status[mac_address] = "on";
      _car_park_busy[mac_address] = false;
    } else if (type == DEVICE_LIGHT_TYPE) {
      _light_status[mac_address] = 2;
    } else if (type == DEVICE_INTRUSION_TYPE) {
      _alarm_status[mac_address] = 0;
      _intrusion_detected[mac_address] = false;
    } else if (type == DEVICE_ROOF_TYPE) {
      _roof_status[mac_address] = 0;
    } else if (type == DEVICE_RFID_TYPE) {
      _rfid_status[mac_address] = 0;
    } else if (type == DEVICE_GATE_TYPE) {
      _gate_status[mac_address] = 0;
    }

    // Notify ws clients of device connection
    StaticJsonDocument<256> doc;
    doc["event"] = WS_DEVICE_CONNECTED;
    doc["mac"] = mac_address;
    doc["type"] = type;
    String payload;
    serializeJson(doc, payload);
    sendWsUpdate(payload);

    // Write device on db
    MySqlWrapper::getInstance().insertDevice(mac_address, type, 1);

    // Push device connected event
    char event[128];
    sprintf(event, DEVICE_CONNECTED_EVENT, mac_address.c_str(), type.c_str());
    MySqlWrapper::getInstance().setAutoclose(true);
    MySqlWrapper::getInstance().insertEvent(DEVICE_EVENT_CATEGORY, String(event), mac_address);
  } else if (topic.startsWith(String(MQTT_DEVICE_SUB_TOPIC_PREFIX).c_str())) {
    String mac_address = doc["mac"];
    String type = doc["type"];

    if (type == DEVICE_CAR_PARK_TYPE) {
      _car_park_busy[mac_address] = doc["busy"].as<boolean>();

      // Store on/off status.
      String status = doc["status"].as<String>();
      _car_park_status[mac_address] = status;

      // Notify ws clients of car park status changed
      StaticJsonDocument<256> doc;
      doc["event"] = WS_CARPARK_UPDATE;
      doc["mac"] = mac_address;
      doc["status"] = status == "on" ? 1 : 0;
      doc["busy"] = _car_park_busy[mac_address];
      String payload;
      serializeJson(doc, payload);
      sendWsUpdate(payload);

      InfluxWrapper::getInstance().checkConnection();
      InfluxWrapper::getInstance().addTag("mac", mac_address);
      InfluxWrapper::getInstance().addTag("type", type);
      InfluxWrapper::getInstance().writeToInflux("busy", doc["busy"].as<boolean>());
    } else if (type == DEVICE_LIGHT_TYPE) {
      int lightAmount = doc["lightAmount"].as<uint>();
      _light_values[mac_address].push_back(lightAmount);

      // Store light status.
      int status = doc["status"].as<String>() == "off" ? 0 : doc["status"].as<String>() == "on" ? 1 : 2;
      _light_status[mac_address] = status;
    } else if (type == DEVICE_INTRUSION_TYPE) {
      String status = doc["status"].as<String>();
      _alarm_status[mac_address] = status == "on" ? 1 : 0;
      _intrusion_detected[mac_address] = doc["intrusion"].as<uint>();

			String message = String(NOTIFICATION_INTRUSION_MESSAGE);
			_telegram_manager.sendNotification(_id_to_notify, message, "");

      // Notify ws clients of car park status changed
      StaticJsonDocument<256> doc;
      doc["event"] = WS_INTRUSION_UPDATE;
      doc["mac"] = mac_address;
      doc["status"] = _alarm_status[mac_address];
      doc["intrusion"] = _intrusion_detected[mac_address];
      String payload;
      serializeJson(doc, payload);
      sendWsUpdate(payload);
    } else if (type == DEVICE_RFID_TYPE) {
      StaticJsonDocument<512> config;
      this->getConfiguration(config, DEVICE_RFID_TYPE);
      String subscribed_topic = config["topicToSubscribe"];
      subscribed_topic.replace("<mac>", mac_address);

      String card = doc["card"];
      String action = doc["action"];

      StaticJsonDocument<256> payload;
      payload["card"] = card;

      bool is_master, is_authorized;
      MySqlWrapper::getInstance().setAutoclose(false);
      MySqlWrapper::getInstance().validateCard(card, is_master, is_authorized);
      payload["is_master"] = is_master;
      payload["authorized"] = is_authorized;

      if (action == "verify") {
        if (is_authorized) {
          StaticJsonDocument<512> config;
          this->getConfiguration(config, DEVICE_GATE_TYPE);

          auto it_gate = _gate_status.begin();

          while (it_gate != _gate_status.end()) {
            _gate_status[it_gate->first] = 1;
            String subscribed_topic = config["topicToSubscribe"];
            subscribed_topic.replace("<mac>", it_gate->first);

            StaticJsonDocument<256> doc_open_gate;
            doc_open_gate["command"] = 1;

            String p;
            serializeJson(doc_open_gate, p);

            _mqtt_writer.enqueuePublishMessage(subscribed_topic, p, false, 1);
            it_gate++;
          }

          // Notify ws clients of gate to be opened
          StaticJsonDocument<256> doc;
          doc["event"] = WS_GATE_UPDATE;
          doc["mac"] = mac_address;
          doc["status"] = 1;
        } else {
          _telegram_manager.sendNotification(_id_to_notify, NOTIFICATION_INVALID_CARD, "");
        }
      }

      if (action == "authorize") {
        if (! is_master) {
          if (is_authorized) {
            MySqlWrapper::getInstance().revokeCard(card);
            _telegram_manager.sendNotification(_id_to_notify, NOTIFICATION_CARD_REMOVED, "");

            // Notify ws clients of removed card
            StaticJsonDocument<256> doc;
            doc["event"] = WS_CARD_REMOVED;
            doc["mac"] = mac_address;
            doc["card"] = card;
            String payload;
            serializeJson(doc, payload);
            sendWsUpdate(payload);
          } else {
            MySqlWrapper::getInstance().authorizeCard(card);
            _telegram_manager.sendNotification(_id_to_notify, NOTIFICATION_CARD_REGISTERED, "");

            // Notify ws clients of registered card
            StaticJsonDocument<256> doc;
            doc["event"] = WS_CARD_REMOVED;
            doc["mac"] = mac_address;
            doc["card"] = card;
            String payload;
            serializeJson(doc, payload);
            sendWsUpdate(payload);
          }
          payload["authorized"] = !is_authorized;
        }
      }

      String response;
      serializeJson(payload, response);
      _mqtt_writer.connect().publish(subscribed_topic, response);

      MySqlWrapper::getInstance().setAutoclose(true);
    }

    else {
      Serial.println("Device type not recognized");
    }
  } else if (topic.startsWith(String(MQTT_LAST_WILL_PREFIX).c_str())) {
    String mac_address = doc["mac"];
    String type = doc["type"];

    if (type == DEVICE_CAR_PARK_TYPE) {
      _car_park_status.erase(mac_address);
      _car_park_busy.erase(mac_address);
    } else if (type == DEVICE_LIGHT_TYPE) {
      _light_status.erase(mac_address);
    } else if (type == DEVICE_INTRUSION_TYPE) {
      _alarm_status.erase(mac_address);
      _intrusion_detected.erase(mac_address);
    } else if (type == DEVICE_ROOF_TYPE) {
      _roof_status.erase(mac_address);
    } else if (type == DEVICE_RFID_TYPE) {
      _rfid_status.erase(mac_address);
    } else if (type == DEVICE_GATE_TYPE) {
      _gate_status.erase(mac_address);
    }

    // Notify web socket of device death
    StaticJsonDocument<256> doc;
    doc["event"] = WS_DEVICE_DEAD;
    doc["mac"] = mac_address;
    doc["type"] = type;
    String payload;
    serializeJson(doc, payload);
    sendWsUpdate(payload);

    // Update status for this specific device
    MySqlWrapper::getInstance().updateDevice(mac_address, 0);

    char event[128];
    sprintf(event, DEVICE_DISCONNECTED_EVENT, mac_address.c_str(), type.c_str());
    MySqlWrapper::getInstance().insertEvent(DEVICE_EVENT_CATEGORY, String(event), mac_address);
  }
}

void MasterController::onTelegramMessageReceived(const String &chat_id, const String &message, const String &from)
{
	String command_list = "";
	command_list += " > " + String(ALARM_ON_COMMAND) + ": " + String(ALARM_ON_COMMAND_DESCRPTION) + "\n";
	command_list += " > " + String(ALARM_OFF_COMMAND) + ": " + String(ALARM_OFF_COMMAND_DESCRPTION) + "\n";
	command_list += " > " + String(AVAILABILITY_COMMAND) + ": " + String(AVAILABILITY_COMMAND_DESCRPTION) + "\n";
	command_list += " > " + String(NOTIFICATIONS_ON_COMMAND) + ": " + String(NOTIFICATIONS_ON_COMMAND_DESCRPTION) + "\n";
	command_list += " > " + String(NOTIFICATIONS_OFF_COMMAND) + ": " + String(NOTIFICATIONS_OFF_COMMAND_DESCRPTION) +"\n";
	//command_list += " > " + String(REGISTER_CARD_COMMAND) + ": " + String(REGISTER_CARD_COMMAND_DESCRIPTION) + "\n";
	command_list += " > " + String(PARKING_INFO_COMMAND) + ": " + String(PARKING_INFO_COMMAND_DESCRPTION) + "\n";

	if (message.equals(ALARM_ON_COMMAND)) {
    StaticJsonDocument<512> config;
    this->getConfiguration(config, DEVICE_INTRUSION_TYPE);

    StaticJsonDocument<256> doc;
    doc["command"] = 1;

    String payload;
    serializeJson(doc, payload);

    for (auto device : _alarm_status) {
      _alarm_status[device.first] = 1;

      // Push command to each device through MQTT
      String deviceTopic = config["topicToSubscribe"];
      deviceTopic.replace("<mac>", device.first);
      _mqtt_writer.connect().publish(deviceTopic, payload, false, 1);
    }

		_telegram_manager.sendMessageWithReplyKeyboard(chat_id, "Alarm armed!", "");

    // Notify ws clients of alarm armed
    StaticJsonDocument<256> doc_alarm;
    doc_alarm["event"] = WS_ALARM_UPDATE;
    doc_alarm["status"] = 1;
    String payload_alarm;
    serializeJson(doc_alarm, payload_alarm);
    sendWsUpdate(payload_alarm);
  } else if (message.equals(ALARM_OFF_COMMAND)) {
    StaticJsonDocument<256> doc;
    StaticJsonDocument<512> config;
    this->getConfiguration(config, DEVICE_INTRUSION_TYPE);

    doc["command"] = 0;

    String payload;
    serializeJson(doc, payload);

    for (auto device : _alarm_status) {
      _alarm_status[device.first] = 1;

      // Push command to each device through MQTT
      String deviceTopic = config["topicToSubscribe"];
      deviceTopic.replace("<mac>", device.first);
      _mqtt_writer.connect().publish(deviceTopic, payload, false, 1);
    }

		_telegram_manager.sendMessageWithReplyKeyboard(chat_id, "Alarm off!", "");

    // Notify ws clients of alarm off
    StaticJsonDocument<256> doc_alarm;
    doc_alarm["event"] = WS_ALARM_UPDATE;
    doc_alarm["status"] = 0;
    String payload_alarm;
    serializeJson(doc_alarm, payload_alarm);
    sendWsUpdate(payload_alarm);
  } else if (message.equals(AVAILABILITY_COMMAND)) {
    uint available = 0, total = 0;
    auto it = _car_park_busy.begin();

    while (it != _car_park_busy.end()) {
      if (!it->second) {
        available++;
      }
      total++;
      it++;
    }

    String message = "Availability: " + String(available) + "/" + String(total);
    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, message, "");
  } else if (message.equals(REGISTER_CARD_COMMAND)) {
    // TODO
  } else if (message.equals(PARKING_INFO_COMMAND)) {
    String response = "Smart Parking status:\n";

		uint available = 0, total = 0;

    // 1. Check availability
    for (auto car_park : _car_park_busy) {
      if (!car_park.second) available++;
      total++;
    }
    response += " > Availability: " + String(available) + "/" + String(total) + "\n";

    // 2. Lights status
    for (auto light : _light_status) {
      String status = light.second == 1 ? "on" : "off";
      response += " > Light " + light.first + ": " + status + "\n";
    }

    // 3. Alarms status
    auto it_alarm = _alarm_status.begin();
    auto it_intrusion = _intrusion_detected.begin();
    while (it_alarm != _alarm_status.end() && it_intrusion != _intrusion_detected.end()) {
        String status = it_alarm->second == 1 ? "on" : "off";
        String intrusion_status = it_intrusion->second ? "true" : "false";
        response += " > Alarm " + it_alarm->first + ": " + status + "\n";
        response += "  > Intrusion: " + intrusion_status + "\n";
        it_alarm++;
        it_intrusion++;
      }

    // 4. Rooftop windows status
    for (auto roof : _roof_status) {
      String status = roof.second == 1 ? "open" : "close";
      response += " > Roof window " + roof.first + ": " + status + "\n";
    }

    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, response, "");
  } else if (message.equals(NOTIFICATIONS_ON_COMMAND)) {
    _id_to_notify.insert(chat_id);
    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, NOTIFICATION_ON_MESSAGE, "");
  } else if (message.equals(NOTIFICATIONS_OFF_COMMAND)) {
    _id_to_notify.erase(chat_id);
    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, NOTIFICATION_OFF_MESSAGE, "");
  } else if (message.equals(HELP_COMMAND)) {
    String response = "Welcome to Smart Parking Telegram Bot, " + from + ".\n";
    response += "Commands:\n\n";
    response += command_list;

    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, response, "");
  } else {
    String response = "Command not recognized! Possible commands:\n";
    response += command_list;

    _telegram_manager.sendMessageWithReplyKeyboard(chat_id, response, "");
  }
}

void MasterController::onWeatherReceived(StaticJsonDocument<1024> &doc){
  float temperature = doc["main"]["temp"].as<float>();
  float humidity = doc["main"]["humidity"].as<float>();
  String weather = doc["weather"][0]["main"].as<String>();

  String tmp[] = {"Rain", "Snow", "Thunderstorm", "Drizzle"};
  std::set<String> bad_conditions(tmp, tmp + sizeof(tmp) / sizeof(tmp[0]));

  const boolean is_in = bad_conditions.find(weather) != bad_conditions.end();

  if (is_in) this->sendCommandToRoof(0);
  else this->sendCommandToRoof(1);
}

boolean MasterController::getConfiguration(StaticJsonDocument<512> &config, String type)
{
  String strConfig = MySqlWrapper::getInstance().getDeviceConfiguration(type);

  // deserialize the JSON config
  DeserializationError error = deserializeJson(config, strConfig);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  return true;
}

boolean MasterController::getDeviceConfiguration(StaticJsonDocument<512> &config, String mac_address, String type)
{
  if (! this->getConfiguration(config, type)) {
    return false;
  }

  if (config.containsKey("topicToPublish")) {
    String topic = config["topicToPublish"].as<String>();
    topic.replace("<mac>", mac_address);
    config["topicToPublish"] = topic;
  }

  String topic2 = config["topicToSubscribe"].as<String>();
  topic2.replace("<mac>", mac_address);
  config["topicToSubscribe"] = topic2;

  return true;
}

void MasterController::sendCommandToRoof(int command)
{
	StaticJsonDocument<512> config;
	this->getConfiguration(config, DEVICE_ROOF_TYPE);

	StaticJsonDocument<256> doc;
	doc["command"] = command;

	String payload;
	serializeJson(doc, payload);

  bool should_send = false;
	for (auto device : _roof_status) {
    if (device.second != command) {
      should_send = true;

      // Push command to each device through MQTT
      String subscribe_topic = config["topicToSubscribe"];
      subscribe_topic.replace("<mac>", device.first);
      _mqtt_writer.connect().publish(subscribe_topic, payload, false, 1);

      _roof_status[device.first] = command;
    }
  }

  if (should_send) {
    // Push notification through telegram bot
    String msg = command == 0 ? NOTIFICATION_ROOF_CLOSED_MESSAGE : NOTIFICATION_ROOF_OPENED_MESSAGE;
    _telegram_manager.sendNotification(_id_to_notify, msg, "");

    // Notify ws clients of alarm off
    StaticJsonDocument<256> doc;
    doc["event"] = WS_ROOF_UPDATE;
    doc["status"] = command;
    String payload;
    serializeJson(doc, payload);
    sendWsUpdate(payload);
  }
}

String MasterController::buildJsonCarParkState()
{
  DynamicJsonDocument doc(1536);
  doc["event"] = WS_BOOT;

  // 1. Alarms status
  if (_alarm_status.size() > 0 && _intrusion_detected.size() > 0) {
    uint alarmArmed = _alarm_status.begin()->second;
    uint intrusion = _intrusion_detected.begin()->second;

    doc["alarm"] = alarmArmed;
    doc["intrusion"] = intrusion;
  }

  // 2. Gate status
  if (_gate_status.size() > 0) {
    uint gateOpen = _gate_status.begin()->second;

    doc["gate"] = gateOpen;
  }

  // 3. Gate status
  if (_roof_status.size() > 0) {
    uint roofOpen = _roof_status.begin()->second;

    doc["roof"] = roofOpen;
  }

  // 4. Cards
  int columns;
  std::vector<std::vector<String>> rows;
  MySqlWrapper::getInstance().getCards(columns, rows);

  JsonArray cards = doc.createNestedArray("cards");
  for (auto row : rows) {
    JsonObject card = cards.createNestedObject();
    card["card_id"] = row[0];
    card["is_master"] = (bool)row[1].toInt();
    card["registered_at"] = row[2];
  }

  // 5. Devices
  MySqlWrapper::getInstance().getDevices(columns, rows);

  JsonObject devices = doc.createNestedObject("devices");
  for (auto row : rows) {
    JsonObject device = devices.createNestedObject(row[0]);
    device["device_id"] = row[0];
    device["type"] = row[1];
    device["online"] = (bool)row[2].toInt();
    device["last_seen"] = row[3];
  }

  // 6. Car-park slots
  JsonArray parking_slots_availability = doc.createNestedArray("slots");

  auto busy_it = _car_park_busy.begin();
  auto status_it = _car_park_status.begin();
  while (busy_it != _car_park_busy.end() && status_it != _car_park_status.end()) {
    JsonObject slot = parking_slots_availability.createNestedObject();

    slot["mac_address"] = status_it->first;
    slot["busy"] = busy_it->second;
    slot["status"] = status_it->second;

    busy_it++;
    status_it++;
  }

  String payload;
  serializeJson(doc, payload);

  return payload;
}

void MasterController::sendWsUpdate(String payload)
{
  for (auto id : _ws_clients_id) {
    _ws.text((uint32_t)id, payload.c_str());
  }
}
