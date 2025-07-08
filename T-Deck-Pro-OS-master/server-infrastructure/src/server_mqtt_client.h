// server_mqtt_client.h
// T-Deck-Pro OS MQTT Client for Server Communication
#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "core/utils/logger.h"

class ServerMQTTClient {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    String deviceId;
    String brokerHost;
    int brokerPort;
    bool connected;
    
    unsigned long lastTelemetryTime;
    unsigned long telemetryInterval;
    unsigned long lastHeartbeatTime;
    unsigned long heartbeatInterval;
    
    // Message handlers
    void (*configHandler)(const JsonObject& config);
    void (*otaHandler)(const JsonObject& ota);
    void (*appHandler)(const JsonObject& app);
    
    // Internal methods
    void onMqttConnect();
    void onMqttMessage(char* topic, byte* payload, unsigned int length);
    void sendHeartbeat();
    void handleConfigMessage(const JsonObject& config);
    void handleOtaMessage(const JsonObject& ota);
    void handleAppMessage(const JsonObject& app);
    
    // Static callback wrapper
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static ServerMQTTClient* instance;
    
public:
    ServerMQTTClient(const String& deviceId, const String& brokerHost = "localhost", int brokerPort = 1883);
    ~ServerMQTTClient();
    
    // Connection management
    bool initialize();
    void update();
    void disconnect();
    bool reconnect();
    
    // Device communication
    bool registerDevice(const JsonObject& deviceInfo);
    bool sendTelemetryData(const JsonObject& telemetry);
    bool sendStatus(const String& status, const JsonObject& additionalData = JsonObject());
    bool sendMeshMessage(const String& fromNode, const String& toNode, const String& messageType, const JsonObject& payload);
    
    // Configuration
    void setConfigHandler(void (*handler)(const JsonObject& config));
    void setOtaHandler(void (*handler)(const JsonObject& ota));
    void setAppHandler(void (*handler)(const JsonObject& app));
    void setTelemetryInterval(unsigned long interval);
    
    // Status
    bool isConnected() const;
    String getDeviceId() const;
    String getBrokerHost() const;
    
    // Utility methods
    bool publishMessage(const String& topic, const JsonObject& payload, bool retain = false);
    bool publishMessage(const String& topic, const String& payload, bool retain = false);
};

// Integration class for easy T-Deck-Pro OS integration
class TDeckProServerIntegration {
private:
    ServerMQTTClient* mqttClient;
    String deviceId;
    bool initialized;
    
    // System integration
    void collectTelemetryData(JsonObject& telemetry);
    void applyConfiguration(const JsonObject& config);
    void handleOtaUpdate(const JsonObject& ota);
    void handleAppManagement(const JsonObject& app);
    
    // Static handlers for MQTT client
    static void configHandler(const JsonObject& config);
    static void otaHandler(const JsonObject& ota);
    static void appHandler(const JsonObject& app);
    static TDeckProServerIntegration* instance;
    
public:
    TDeckProServerIntegration(const String& deviceId, const String& brokerHost = "localhost");
    ~TDeckProServerIntegration();
    
    // Lifecycle
    bool initialize();
    void update();
    void shutdown();
    
    // Device management
    bool registerWithServer();
    bool sendCurrentTelemetry();
    bool reportStatus(const String& status, const String& reason = "");
    
    // Mesh integration
    bool forwardMeshMessage(const String& fromNode, const String& toNode, const String& messageType, const JsonObject& payload);
    
    // Status
    bool isServerConnected() const;
    String getServerStatus() const;
};