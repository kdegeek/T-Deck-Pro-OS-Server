/**
 * @file wifi_manager.h
 * @brief WiFi communication manager for T-Deck-Pro OS
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "core/utils/logger.h"

namespace TDeckOS {
namespace Communication {

/**
 * @brief WiFi operating modes
 */
enum class WiFiMode {
    OFF,
    STATION,
    ACCESS_POINT,
    STATION_AP
};

/**
 * @brief WiFi connection status
 */
enum class WiFiStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED,
    LOST_CONNECTION
};

/**
 * @brief WiFi security types
 */
enum class WiFiSecurity {
    OPEN,
    WEP,
    WPA_PSK,
    WPA2_PSK,
    WPA_WPA2_PSK,
    WPA2_ENTERPRISE,
    WPA3_PSK,
    WPA2_WPA3_PSK,
    WAPI_PSK,
    UNKNOWN
};

/**
 * @brief WiFi network information
 */
struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    WiFiSecurity security;
    bool isHidden;
};

/**
 * @brief WiFi configuration for station mode
 */
struct WiFiStationConfig {
    String ssid;
    String password;
    bool autoReconnect = true;
    uint32_t connectTimeoutMs = 10000;
    uint8_t maxRetries = 3;
    bool useDHCP = true;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
};

/**
 * @brief WiFi configuration for access point mode
 */
struct WiFiAPConfig {
    String ssid;
    String password;
    uint8_t channel = 1;
    bool hidden = false;
    uint8_t maxConnections = 4;
    IPAddress ip = IPAddress(192, 168, 4, 1);
    IPAddress gateway = IPAddress(192, 168, 4, 1);
    IPAddress subnet = IPAddress(255, 255, 255, 0);
};

/**
 * @brief WiFi statistics
 */
struct WiFiStats {
    uint32_t connectAttempts;
    uint32_t successfulConnections;
    uint32_t disconnections;
    uint32_t reconnections;
    uint32_t scanCount;
    uint32_t bytesTransmitted;
    uint32_t bytesReceived;
    uint32_t uptime;
    int32_t lastRssi;
    uint8_t lastChannel;
};

/**
 * @brief Callback function types
 */
typedef void (*WiFiEventCallback)(WiFiStatus status, const String& info);
typedef void (*WiFiScanCallback)(const std::vector<WiFiNetwork>& networks);

/**
 * @brief WiFi Manager class
 */
class WiFiManager {
public:
    /**
     * @brief Constructor
     */
    WiFiManager();

    /**
     * @brief Destructor
     */
    ~WiFiManager();

    /**
     * @brief Initialize WiFi manager
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Deinitialize WiFi manager
     */
    void deinitialize();

    /**
     * @brief Check if WiFi is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Set WiFi mode
     * @param mode WiFi mode
     * @return true if successful, false otherwise
     */
    bool setMode(WiFiMode mode);

    /**
     * @brief Get current WiFi mode
     * @return Current mode
     */
    WiFiMode getMode() const { return m_currentMode; }

    /**
     * @brief Connect to WiFi network (station mode)
     * @param config Station configuration
     * @param callback Optional callback for connection events
     * @return true if connection started, false otherwise
     */
    bool connect(const WiFiStationConfig& config, WiFiEventCallback callback = nullptr);

    /**
     * @brief Disconnect from WiFi network
     */
    void disconnect();

    /**
     * @brief Start access point
     * @param config AP configuration
     * @return true if successful, false otherwise
     */
    bool startAP(const WiFiAPConfig& config);

    /**
     * @brief Stop access point
     */
    void stopAP();

    /**
     * @brief Get connection status
     * @return Current status
     */
    WiFiStatus getStatus() const { return m_status; }

    /**
     * @brief Check if connected to WiFi
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return m_status == WiFiStatus::CONNECTED; }

    /**
     * @brief Scan for available networks
     * @param callback Callback for scan results
     * @param async Perform asynchronous scan
     * @return true if scan started, false otherwise
     */
    bool scanNetworks(WiFiScanCallback callback, bool async = true);

    /**
     * @brief Get current IP address
     * @return IP address
     */
    IPAddress getIPAddress() const;

    /**
     * @brief Get current MAC address
     * @return MAC address as string
     */
    String getMACAddress() const;

    /**
     * @brief Get current SSID
     * @return SSID string
     */
    String getSSID() const;

    /**
     * @brief Get current RSSI
     * @return RSSI in dBm
     */
    int32_t getRSSI() const;

    /**
     * @brief Get current channel
     * @return Channel number
     */
    uint8_t getChannel() const;

    /**
     * @brief Get number of connected clients (AP mode)
     * @return Number of connected clients
     */
    uint8_t getConnectedClients() const;

    /**
     * @brief Set power save mode
     * @param enable Enable/disable power save
     * @return true if successful, false otherwise
     */
    bool setPowerSave(bool enable);

    /**
     * @brief Set WiFi power
     * @param power Power level (0-20.5 dBm)
     * @return true if successful, false otherwise
     */
    bool setPower(float power);

    /**
     * @brief Get WiFi statistics
     * @return Current statistics
     */
    WiFiStats getStats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Process WiFi events (call from main loop)
     */
    void process();

    /**
     * @brief Set event callback
     * @param callback Event callback function
     */
    void setEventCallback(WiFiEventCallback callback) { m_eventCallback = callback; }

private:
    // Configuration
    bool m_initialized;
    WiFiMode m_currentMode;
    WiFiStatus m_status;
    WiFiStationConfig m_stationConfig;
    WiFiAPConfig m_apConfig;
    
    // Callbacks
    WiFiEventCallback m_eventCallback;
    WiFiScanCallback m_scanCallback;
    
    // Statistics
    mutable WiFiStats m_stats;
    uint32_t m_initTime;
    uint32_t m_lastConnectAttempt;
    uint8_t m_retryCount;
    
    // FreeRTOS
    TaskHandle_t m_taskHandle;
    QueueHandle_t m_eventQueue;
    SemaphoreHandle_t m_mutex;
    
    // Internal methods
    static void wifiTask(void* parameter);
    static void wifiEventHandler(WiFiEvent_t event);
    void handleWiFiEvent(WiFiEvent_t event);
    void updateStats();
    WiFiSecurity getSecurityType(wifi_auth_mode_t authMode);
    bool configureStation();
    bool configureAP();
    void checkConnection();
    
    // Static instance for event handler
    static WiFiManager* s_instance;
};

} // namespace Communication
} // namespace TDeckOS