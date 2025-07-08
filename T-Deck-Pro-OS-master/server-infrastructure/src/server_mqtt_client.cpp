// server_mqtt_client.cpp
// T-Deck-Pro OS MQTT Client Implementation
#include "server_mqtt_client.h"

// Static instance pointers for callbacks
ServerMQTTClient* ServerMQTTClient::instance = nullptr;
TDeckProServerIntegration* TDeckProServerIntegration::instance = nullptr;

// ServerMQTTClient Implementation
ServerMQTTClient::ServerMQTTClient(const String& deviceId, const String& brokerHost, int brokerPort)
    : deviceId(deviceId), brokerHost(brokerHost), brokerPort(brokerPort), 
      connected(false), lastTelemetryTime(0), telemetryInterval(300000), // 5 minutes
      lastHeartbeatTime(0), heartbeatInterval(60000), // 1 minute
      configHandler(nullptr), otaHandler(nullptr), appHandler(nullptr) {
    
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(brokerHost.c_str(), brokerPort);
    mqttClient.setCallback(mqttCallback);
    instance = this;
    
    Logger::info("ServerMQTTClient", "Initialized for device: " + deviceId);
}

ServerMQTTClient::~ServerMQTTClient() {
    disconnect();
    instance = nullptr;
}

bool ServerMQTTClient::initialize() {
    if (!WiFi.isConnected()) {
        Logger::error("ServerMQTTClient", "WiFi not connected");
        return false;
    }
    
    Logger::info("ServerMQTTClient", "Connecting to MQTT broker: " + brokerHost + ":" + String(brokerPort));
    
    if (mqttClient.connect(deviceId.c_str())) {
        connected = true;
        onMqttConnect();
        Logger::info("ServerMQTTClient", "Connected to MQTT broker");
        return true;
    } else {
        Logger::error("ServerMQTTClient", "Failed to connect to MQTT broker, state: " + String(mqttClient.state()));
        return false;
    }
}

void ServerMQTTClient::update() {
    if (!mqttClient.connected()) {
        connected = false;
        if (WiFi.isConnected()) {
            reconnect();
        }
        return;
    }
    
    mqttClient.loop();
    
    unsigned long currentTime = millis();
    
    // Send heartbeat
    if (currentTime - lastHeartbeatTime > heartbeatInterval) {
        sendHeartbeat();
        lastHeartbeatTime = currentTime;
    }
}

void ServerMQTTClient::disconnect() {
    if (connected) {
        sendStatus("offline", JsonObject());
        mqttClient.disconnect();
        connected = false;
        Logger::info("ServerMQTTClient", "Disconnected from MQTT broker");
    }
}

bool ServerMQTTClient::reconnect() {
    if (mqttClient.connected()) {
        return true;
    }
    
    Logger::info("ServerMQTTClient", "Attempting to reconnect to MQTT broker");
    
    if (mqttClient.connect(deviceId.c_str())) {
        connected = true;
        onMqttConnect();
        Logger::info("ServerMQTTClient", "Reconnected to MQTT broker");
        return true;
    } else {
        Logger::error("ServerMQTTClient", "Reconnection failed, state: " + String(mqttClient.state()));
        return false;
    }
}

void ServerMQTTClient::onMqttConnect() {
    // Subscribe to device-specific topics
    String configTopic = "tdeckpro/" + deviceId + "/config";
    String otaTopic = "tdeckpro/" + deviceId + "/ota";
    String appTopic = "tdeckpro/" + deviceId + "/apps";
    
    mqttClient.subscribe(configTopic.c_str());
    mqttClient.subscribe(otaTopic.c_str());
    mqttClient.subscribe(appTopic.c_str());
    
    Logger::info("ServerMQTTClient", "Subscribed to device topics");
    
    // Send online status
    sendStatus("online", JsonObject());
}

void ServerMQTTClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->onMqttMessage(topic, payload, length);
    }
}

void ServerMQTTClient::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    // Parse payload as JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Logger::error("ServerMQTTClient", "Failed to parse MQTT message: " + String(error.c_str()));
        return;
    }
    
    String topicStr = String(topic);
    Logger::debug("ServerMQTTClient", "Received message on topic: " + topicStr);
    
    // Route message based on topic
    if (topicStr.endsWith("/config")) {
        handleConfigMessage(doc.as<JsonObject>());
    } else if (topicStr.endsWith("/ota")) {
        handleOtaMessage(doc.as<JsonObject>());
    } else if (topicStr.endsWith("/apps")) {
        handleAppMessage(doc.as<JsonObject>());
    }
}

void ServerMQTTClient::handleConfigMessage(const JsonObject& config) {
    Logger::info("ServerMQTTClient", "Received configuration update");
    
    // Update telemetry interval if specified
    if (config.containsKey("update_interval")) {
        telemetryInterval = config["update_interval"].as<unsigned long>() * 1000; // Convert to ms
    }
    
    // Call user handler if set
    if (configHandler) {
        configHandler(config);
    }
}

void ServerMQTTClient::handleOtaMessage(const JsonObject& ota) {
    Logger::info("ServerMQTTClient", "Received OTA update notification");
    
    if (otaHandler) {
        otaHandler(ota);
    }
}

void ServerMQTTClient::handleAppMessage(const JsonObject& app) {
    Logger::info("ServerMQTTClient", "Received app management message");
    
    if (appHandler) {
        appHandler(app);
    }
}

void ServerMQTTClient::sendHeartbeat() {
    DynamicJsonDocument doc(256);
    doc["status"] = "online";
    doc["timestamp"] = millis();
    doc["uptime"] = millis() / 1000;
    
    String topic = "tdeckpro/" + deviceId + "/heartbeat";
    publishMessage(topic, doc.as<JsonObject>());
}

bool ServerMQTTClient::registerDevice(const JsonObject& deviceInfo) {
    String topic = "tdeckpro/" + deviceId + "/register";
    return publishMessage(topic, deviceInfo, true); // Retain registration
}

bool ServerMQTTClient::sendTelemetryData(const JsonObject& telemetry) {
    String topic = "tdeckpro/" + deviceId + "/telemetry";
    return publishMessage(topic, telemetry);
}

bool ServerMQTTClient::sendStatus(const String& status, const JsonObject& additionalData) {
    DynamicJsonDocument doc(512);
    doc["status"] = status;
    doc["timestamp"] = millis();
    
    // Merge additional data
    for (JsonPair kv : additionalData) {
        doc[kv.key()] = kv.value();
    }
    
    String topic = "tdeckpro/" + deviceId + "/status";
    return publishMessage(topic, doc.as<JsonObject>(), true); // Retain status
}

bool ServerMQTTClient::sendMeshMessage(const String& fromNode, const String& toNode, const String& messageType, const JsonObject& payload) {
    DynamicJsonDocument doc(1024);
    doc["from_node"] = fromNode;
    doc["to_node"] = toNode;
    doc["message_type"] = messageType;
    doc["payload"] = payload;
    doc["timestamp"] = millis();
    
    String topic = "tdeckpro/mesh/" + messageType;
    return publishMessage(topic, doc.as<JsonObject>());
}

bool ServerMQTTClient::publishMessage(const String& topic, const JsonObject& payload, bool retain) {
    if (!connected) {
        return false;
    }
    
    String jsonString;
    serializeJson(payload, jsonString);
    
    return mqttClient.publish(topic.c_str(), jsonString.c_str(), retain);
}

bool ServerMQTTClient::publishMessage(const String& topic, const String& payload, bool retain) {
    if (!connected) {
        return false;
    }
    
    return mqttClient.publish(topic.c_str(), payload.c_str(), retain);
}

void ServerMQTTClient::setConfigHandler(void (*handler)(const JsonObject& config)) {
    configHandler = handler;
}

void ServerMQTTClient::setOtaHandler(void (*handler)(const JsonObject& ota)) {
    otaHandler = handler;
}

void ServerMQTTClient::setAppHandler(void (*handler)(const JsonObject& app)) {
    appHandler = handler;
}

void ServerMQTTClient::setTelemetryInterval(unsigned long interval) {
    telemetryInterval = interval * 1000; // Convert to ms
}

bool ServerMQTTClient::isConnected() const {
    return connected && mqttClient.connected();
}

String ServerMQTTClient::getDeviceId() const {
    return deviceId;
}

String ServerMQTTClient::getBrokerHost() const {
    return brokerHost;
}

// TDeckProServerIntegration Implementation
TDeckProServerIntegration::TDeckProServerIntegration(const String& deviceId, const String& brokerHost)
    : deviceId(deviceId), initialized(false) {
    
    mqttClient = new ServerMQTTClient(deviceId, brokerHost);
    instance = this;
    
    // Set up handlers
    mqttClient->setConfigHandler(configHandler);
    mqttClient->setOtaHandler(otaHandler);
    mqttClient->setAppHandler(appHandler);
}

TDeckProServerIntegration::~TDeckProServerIntegration() {
    shutdown();
    delete mqttClient;
    instance = nullptr;
}

bool TDeckProServerIntegration::initialize() {
    if (!mqttClient->initialize()) {
        return false;
    }
    
    // Register device with server
    if (!registerWithServer()) {
        Logger::error("ServerIntegration", "Failed to register with server");
        return false;
    }
    
    initialized = true;
    Logger::info("ServerIntegration", "Server integration initialized");
    return true;
}

void TDeckProServerIntegration::update() {
    if (!initialized) {
        return;
    }
    
    mqttClient->update();
    
    // Send periodic telemetry
    static unsigned long lastTelemetry = 0;
    if (millis() - lastTelemetry > 300000) { // 5 minutes
        sendCurrentTelemetry();
        lastTelemetry = millis();
    }
}

void TDeckProServerIntegration::shutdown() {
    if (initialized) {
        reportStatus("offline", "shutdown");
        mqttClient->disconnect();
        initialized = false;
    }
}

bool TDeckProServerIntegration::registerWithServer() {
    DynamicJsonDocument doc(1024);
    doc["device_type"] = "t-deck-pro";
    doc["firmware_version"] = "1.0.0"; // TODO: Get from system
    doc["hardware_version"] = "1.0";
    
    JsonObject capabilities = doc.createNestedObject("capabilities");
    capabilities["wifi"] = true;
    capabilities["lora"] = true;
    capabilities["cellular"] = true;
    capabilities["bluetooth"] = true;
    capabilities["gps"] = true;
    capabilities["eink_display"] = true;
    
    JsonObject config = doc.createNestedObject("config");
    config["timezone"] = "UTC";
    config["language"] = "en";
    
    return mqttClient->registerDevice(doc.as<JsonObject>());
}

bool TDeckProServerIntegration::sendCurrentTelemetry() {
    DynamicJsonDocument doc(1024);
    collectTelemetryData(doc.as<JsonObject>());
    return mqttClient->sendTelemetryData(doc.as<JsonObject>());
}

bool TDeckProServerIntegration::reportStatus(const String& status, const String& reason) {
    DynamicJsonDocument doc(256);
    if (!reason.isEmpty()) {
        doc["reason"] = reason;
    }
    
    return mqttClient->sendStatus(status, doc.as<JsonObject>());
}

void TDeckProServerIntegration::collectTelemetryData(JsonObject& telemetry) {
    // TODO: Integrate with actual system components
    telemetry["battery_percentage"] = 85;
    telemetry["temperature"] = 23.5;
    telemetry["cpu_usage"] = 45.2;
    telemetry["memory_usage"] = 67.8;
    telemetry["signal_strength"] = -65;
    telemetry["wifi_connected"] = WiFi.isConnected();
    telemetry["lora_active"] = true;
    telemetry["cellular_connected"] = false;
    
    JsonArray apps = telemetry.createNestedArray("running_apps");
    apps.add("meshtastic");
    apps.add("file_manager");
}

void TDeckProServerIntegration::applyConfiguration(const JsonObject& config) {
    Logger::info("ServerIntegration", "Applying configuration");
    
    // TODO: Integrate with system configuration
    if (config.containsKey("timezone")) {
        // Set system timezone
    }
    
    if (config.containsKey("language")) {
        // Set system language
    }
    
    if (config.containsKey("display_settings")) {
        // Apply display settings
    }
}

void TDeckProServerIntegration::handleOtaUpdate(const JsonObject& ota) {
    Logger::info("ServerIntegration", "Processing OTA update");
    
    if (ota["available"].as<bool>()) {
        // TODO: Integrate with OTA system
        String version = ota["version"];
        String downloadUrl = ota["download_url"];
        
        Logger::info("ServerIntegration", "OTA update available: " + version);
        // Start OTA download and installation
    }
}

void TDeckProServerIntegration::handleAppManagement(const JsonObject& app) {
    Logger::info("ServerIntegration", "Processing app management");
    
    String action = app["action"];
    String appId = app["app_id"];
    
    // TODO: Integrate with app manager
    if (action == "install") {
        // Install app
    } else if (action == "remove") {
        // Remove app
    } else if (action == "update") {
        // Update app
    }
}

bool TDeckProServerIntegration::forwardMeshMessage(const String& fromNode, const String& toNode, const String& messageType, const JsonObject& payload) {
    return mqttClient->sendMeshMessage(fromNode, toNode, messageType, payload);
}

bool TDeckProServerIntegration::isServerConnected() const {
    return mqttClient->isConnected();
}

String TDeckProServerIntegration::getServerStatus() const {
    if (mqttClient->isConnected()) {
        return "connected";
    } else if (WiFi.isConnected()) {
        return "connecting";
    } else {
        return "offline";
    }
}

// Static handlers
void TDeckProServerIntegration::configHandler(const JsonObject& config) {
    if (instance) {
        instance->applyConfiguration(config);
    }
}

void TDeckProServerIntegration::otaHandler(const JsonObject& ota) {
    if (instance) {
        instance->handleOtaUpdate(ota);
    }
}

void TDeckProServerIntegration::appHandler(const JsonObject& app) {
    if (instance) {
        instance->handleAppManagement(app);
    }
}