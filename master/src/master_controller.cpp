#include "../include/master_controller.h"

MasterController::MasterController() :  _server(80),
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

    Serial.println(F("[CONTROLLER] Setting up HTTP server"));
    this->register_routes();
    _server.begin();

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

void MasterController::loop()
{
    // Check Wifi connection
    _wifi_manager.ensure_wifi_is_connected();

    // Handle HTTP requests
    _server.handleClient();

    // Mqtt publish/subscribe
    _mqtt_reader.subscribeAll();
    _mqtt_writer.publishAll();

    // Mqtt listen for incoming messages
    _mqtt_reader.reconnect().listen();

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

void MasterController::register_routes()
{
    _server.on("/", std::bind(&MasterController::handle_root, this));
    _server.on("/api/status", HTTP_GET, std::bind(&MasterController::handle_status_get, this));
    _server.on("/api/display", HTTP_POST, std::bind(&MasterController::handle_display_post, this));
    _server.on("/api/lights", HTTP_POST, std::bind(&MasterController::handle_lights_post, this));
    _server.on("/api/parking-slots", HTTP_POST, std::bind(&MasterController::handle_parking_slot_post, this));

    _server.onNotFound(std::bind(&MasterController::handle_cors, this));
}

void MasterController::handle_cors()
{
    if (_server.method() == HTTP_OPTIONS) {
        _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
        _server.sendHeader("Access-Control-Max-Age", "10000");
        _server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        _server.sendHeader("Access-Control-Allow-Headers", "*");
        _server.send(204);
    } else {
        _server.send(404, "text/plain", "");
    }
}

void MasterController::handle_root()
{
    _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server.send(200, "text/html", PAGE_ROOT);
}

void MasterController::handle_status_get()
{
    DynamicJsonDocument doc(1536);

    JsonObject status = doc.createNestedObject("status");
    auto it = _display_status.begin();
    if (it != _display_status.end()) {
        status["display"] = it->second.equals("on");
    }

    auto it2 = _light_status.begin();
    if (it2 != _light_status.end()) {
        status["lights"] = it2->second;
    }

    int columns;
    std::vector<std::vector<String>> rows;
    MySqlWrapper::getInstance().getDevices(columns, rows);

    JsonArray devices = doc.createNestedArray("devices");
    for (auto row : rows) {
        JsonObject device = devices.createNestedObject();
        device["device_id"] = row[0];
        device["type"] = row[1];
        device["online"] = (bool) row[2].toInt();
        device["last_seen"] = row[3];
    }

    JsonArray parking_slots_availability = doc.createNestedArray("parking_slots_availability");

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

    String json;
    serializeJson(doc, json);
    _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server.send(200, "application/json", json);
}

void MasterController::handle_display_post()
{
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    if (error) {
        this->send_error_response(error.c_str());
        return;
    }
    if (! doc.containsKey("command") || (strcmp(doc["command"], "on") != 0 && strcmp(doc["command"], "off") != 0) && strcmp(doc["command"], "update") != 0) {
        this->send_error_response("Command parameter is missing or does not contain a correct value.");
        return;
    }

    // 1. Update cached status
    auto it = _display_status.begin();
    while (it != _display_status.end()) {
        Serial.print("Updating display: ");
        Serial.println(it->first);

        String mac = it->first;
        _display_status[mac] = doc["command"].as<String>();
        it++;
    }

    // 2. Replace template {{slots}} with number of parking
    if (doc.containsKey("textFirstRow") && doc.containsKey("textSecondRow")) {
        _display_first_row = doc["textFirstRow"].as<String>();
        _display_second_row = doc["textSecondRow"].as<String>();
    }

    String row1, row2;
    substituteDisplayLine(row1, _display_first_row);
    substituteDisplayLine(row2, _display_second_row);
    doc["textFirstRow"] = row1;
    doc["textSecondRow"] = row2;

    // 3. Get configuration for generic light device
    StaticJsonDocument<512> config;
    this->getConfiguration(config, "display");

    String payload;
    serializeJson(doc, payload);

    for (auto device : _display_status) {
        // 4. Push command to each device through MQTT
        char deviceTopic[128];
        sprintf(deviceTopic, config["topicToSubscribe"], device.first.c_str());
        _mqtt_writer.connect().publish(String(deviceTopic), payload, false, 1);

        // 5. Register event on MYSQL
        char event[128];
        sprintf(event, DASHBOARD_COMMAND_EVENT, "DISPLAY", doc["command"].as<String>().c_str());
        MySqlWrapper::getInstance().insertEvent(DASHBOARD_EVENT_CATEGORY, String(event), device.first);
    }

    _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server.send(204, "application/json");
}

void MasterController::handle_lights_post()
{
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    if (error) {
        this->send_error_response(error.c_str());
        return;
    }
    if (! doc.containsKey("command") || doc["command"] < 0 || doc["command"] > 2) {
        this->send_error_response("Command parameter is missing or does not contain a correct value.");
        return;
    }

    // 1. Update cached status
    auto it = _light_status.begin();
    while (it != _light_status.end()) {
        String mac = it->first;
        _light_status[mac] = doc["command"].as<uint>();
        it++;
    }

    // 2. Get configuration for generic light device
    StaticJsonDocument<512> config;
    this->getConfiguration(config, "light");

    for (auto device : _light_status) {
        // 3. Push command to each device through MQTT
        char deviceTopic[128];
        sprintf(deviceTopic, config["topicToSubscribe"], device.first.c_str());
        _mqtt_writer.connect().publish(String(deviceTopic), _server.arg("plain"), false, 1);

        // 4. Register event on MYSQL
        static String commands[] = {"OFF", "ON", "AUTO"};
        char event[128];
        sprintf(event, DASHBOARD_COMMAND_EVENT, "LIGHTS", commands[doc["command"].as<uint>()].c_str());
        MySqlWrapper::getInstance().insertEvent(DASHBOARD_EVENT_CATEGORY, String(event), device.first);
    }

    _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server.send(204, "application/json");
}

void MasterController::handle_parking_slot_post()
{
    if (strlen(_server.arg("device_id").c_str()) == 0) {
        _server.send(404, "application/json", "{\"message\":\"A non-null device_id is required.\"}");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    if (error) {
        this->send_error_response(error.c_str());
    }
    if (! doc.containsKey("command") || (strcmp(doc["command"], "on") != 0 && strcmp(doc["command"], "off") != 0)) {
        this->send_error_response("Command parameter is missing or does not contain a correct value.");
        return;
    }

    String mac_address = _server.arg("device_id");

    // 1. Update cached status
    auto it = _car_park_status.begin();
    while (it != _car_park_status.end()) {
        String mac = it->first;
        _car_park_status[mac] = doc["command"].as<String>();
        it++;
    }

    // 2. Get configuration for generic light device
    StaticJsonDocument<512> config;
    this->getDeviceConfiguration(config, mac_address, "car-park");

    // 3. Push command to each device through MQTT
    String payload = _server.arg("plain");

    _mqtt_writer.connect().publish(config["topicToSubscribe"].as<String>(), payload, false, 1);

    // 4. Register event on MYSQL
    char event[128];
    sprintf(event, DASHBOARD_COMMAND_EVENT, "CAR-PARK", doc["command"].as<String>().c_str());
    MySqlWrapper::getInstance().insertEvent(DASHBOARD_EVENT_CATEGORY, String(event), mac_address);

    _server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    _server.send(204, "application/json");
}

void MasterController::send_error_response(const char* error)
{
    char message[128];
    sprintf(message, "{\"message\":\"%s\"}", error);
    _server.send(400, "application/json", message);
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
        size_t n = serializeJson(config, buffer);
        Serial.println(clientTopicForConfig);
        _mqtt_writer.enqueuePublishMessage(clientTopicForConfig, buffer, false, 2);

        if (type == DEVICE_DISPLAY_TYPE) {
            _display_status[mac_address] = "on";
        } else if (type == DEVICE_CAR_PARK_TYPE) {
            _car_park_status[mac_address] = "on";
            _car_park_busy[mac_address] = false;
        } else if (type == DEVICE_LIGHT_TYPE) {
            _light_status[mac_address] = 2;
        }

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

            // Update display to reflect new value
            StaticJsonDocument<512> config;
            this->getConfiguration(config, "display");

            StaticJsonDocument<128> jsonPayload;
            String row1, row2;
            substituteDisplayLine(row1, _display_first_row);
            substituteDisplayLine(row2, _display_second_row);
            jsonPayload["command"] = "update";
            jsonPayload["textFirstRow"] = row1;
            jsonPayload["textSecondRow"] = row2;

            String payload;
            serializeJson(jsonPayload, payload);

            for (auto device : _display_status) {
                char deviceTopic[128];
                sprintf(deviceTopic, config["topicToSubscribe"], device.first.c_str());
                // mqttWrapper->connectToBroker(MQTT_TOPIC_GLOBAL_CONFIG);
                //mqttWrapper->publish(deviceTopic, payload);
                _mqtt_writer.enqueuePublishMessage(String(deviceTopic), payload);
            }

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
        } else {
            Serial.println("Device type not recognized");
        }
    } else if (topic.startsWith(String(MQTT_LAST_WILL_PREFIX).c_str())) {
        // Get URL from the end of MQTT path
        //String mac_address = topic.substring(topic.lastIndexOf("/") + 1);
        String mac_address = doc["mac"];
        String type = doc["type"];

        if (type == DEVICE_DISPLAY_TYPE) {
            _display_status.erase(mac_address);
        } else if (type == DEVICE_CAR_PARK_TYPE) {
            _car_park_status.erase(mac_address);
            _car_park_busy.erase(mac_address);
        } else if (type == DEVICE_LIGHT_TYPE) {
            _light_status.erase(mac_address);
        }

        // Update status for this specific device
        MySqlWrapper::getInstance().updateDevice(mac_address, 0);

        char event[128];
        sprintf(event, DEVICE_DISCONNECTED_EVENT, mac_address.c_str(), type.c_str());
        MySqlWrapper::getInstance().insertEvent(DEVICE_EVENT_CATEGORY, String(event), mac_address);
    }
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
        topic.replace("%s", mac_address);
        config["topicToPublish"] = topic;
    }

    String topic2 = config["topicToSubscribe"].as<String>();
    topic2.replace("%s", mac_address);
    config["topicToSubscribe"] = topic2;

    return true;
}

void MasterController::substituteDisplayLine(String &out, String in)
{
    if (in.indexOf("{{slots}}") == -1) {
        out = in;
        return;
    }

    uint available = 0;
    auto it = _car_park_busy.begin();

    while (it != _car_park_busy.end()) {
        if (! it->second) {
            available++;
        }
        it++;
    }

    out = in;
    out.replace("{{slots}}", String(available));
}