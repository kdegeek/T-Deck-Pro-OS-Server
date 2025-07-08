/**
 * @file communication_manager.h
 * @brief Main communication manager coordinating all communication modules
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#pragma once

#include "lora_manager.h"
#include "wifi_manager.h"
#include "cellular_manager.h"
#include "core/utils/logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace TDeckOS {
namespace Communication {

/**
 * @brief Communication interface types
 */
enum class CommInterface {
    LORA,
    WIFI,
    CELLULAR,
    BLUETOOTH
};

/**
 * @brief Communication status
 */
struct CommStatus {
    bool loraAvailable;
    bool wifiAvailable;
    bool cellularAvailable;
    bool bluetoothAvailable;
    LoRaMode loraMode;
    WiFiStatus wifiStatus;
    CellularStatus cellularStatus;
    String activeInterfaces;
};

/**
 * @brief Network configuration
 */
struct NetworkConfig {
    // LoRa configuration
    LoRaConfig loraConfig;
    bool enableLoRa = true;
    
    // WiFi configuration
    WiFiStationConfig wifiStationConfig;
    WiFiAPConfig wifiAPConfig;
    bool enableWiFi = true;
    bool enableWiFiAP = false;
    
    // Cellular configuration
    CellularConfig cellularConfig;
    bool enableCellular = true;
    
    // Priority settings
    CommInterface primaryInterface = CommInterface::WIFI;
    CommInterface secondaryInterface = CommInterface::CELLULAR;
    CommInterface meshInterface = CommInterface::LORA;
};

/**
 * @brief Communication statistics
 */
struct CommStats {
    LoRaStats loraStats;
    WiFiStats wifiStats;
    CellularStats cellularStats;
    uint32_t totalBytesTransmitted;
    uint32_t totalBytesReceived;
    uint32_t interfaceSwitches;
    uint32_t uptime;
};

/**
 * @brief Communication event callback
 */
typedef void (*CommEventCallback)(CommInterface interface, const String& event, const String& data);

/**
 * @brief Main Communication Manager class
 */
class CommunicationManager {
public:
    /**
     * @brief Constructor
     */
    CommunicationManager();

    /**
     * @brief Destructor
     */
    ~CommunicationManager();

    /**
     * @brief Initialize communication manager
     * @param config Network configuration
     * @return true if successful, false otherwise
     */
    bool initialize(const NetworkConfig& config = NetworkConfig{});

    /**
     * @brief Deinitialize communication manager
     */
    void deinitialize();

    /**
     * @brief Check if communication manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get LoRa manager
     * @return Pointer to LoRa manager
     */
    LoRaManager* getLoRaManager() { return &m_loraManager; }

    /**
     * @brief Get WiFi manager
     * @return Pointer to WiFi manager
     */
    WiFiManager* getWiFiManager() { return &m_wifiManager; }

    /**
     * @brief Get cellular manager
     * @return Pointer to cellular manager
     */
    CellularManager* getCellularManager() { return &m_cellularManager; }

    /**
     * @brief Start all enabled interfaces
     * @return true if successful, false otherwise
     */
    bool startAllInterfaces();

    /**
     * @brief Stop all interfaces
     */
    void stopAllInterfaces();

    /**
     * @brief Get communication status
     * @return Current status
     */
    CommStatus getStatus() const;

    /**
     * @brief Check if any interface is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Get best available interface for internet connectivity
     * @return Best interface or LORA if none available
     */
    CommInterface getBestInterface() const;

    /**
     * @brief Send data via best available interface
     * @param data Data to send
     * @param length Data length
     * @param interface Preferred interface (optional)
     * @return true if successful, false otherwise
     */
    bool sendData(const uint8_t* data, size_t length, CommInterface interface = CommInterface::WIFI);

    /**
     * @brief Send string via best available interface
     * @param message Message to send
     * @param interface Preferred interface (optional)
     * @return true if successful, false otherwise
     */
    bool sendMessage(const String& message, CommInterface interface = CommInterface::WIFI);

    /**
     * @brief Broadcast message via LoRa (for mesh networking)
     * @param message Message to broadcast
     * @return true if successful, false otherwise
     */
    bool broadcastMesh(const String& message);

    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @return true if connection started, false otherwise
     */
    bool connectWiFi(const String& ssid, const String& password);

    /**
     * @brief Start WiFi access point
     * @param ssid AP SSID
     * @param password AP password
     * @return true if successful, false otherwise
     */
    bool startWiFiAP(const String& ssid, const String& password);

    /**
     * @brief Connect to cellular network
     * @param apn APN name
     * @param username APN username (optional)
     * @param password APN password (optional)
     * @return true if connection started, false otherwise
     */
    bool connectCellular(const String& apn, const String& username = "", const String& password = "");

    /**
     * @brief Scan for WiFi networks
     * @param callback Callback for scan results
     * @return true if scan started, false otherwise
     */
    bool scanWiFi(WiFiScanCallback callback);

    /**
     * @brief Send SMS via cellular
     * @param number Phone number
     * @param message SMS message
     * @return true if successful, false otherwise
     */
    bool sendSMS(const String& number, const String& message);

    /**
     * @brief Set LoRa mode
     * @param mode LoRa mode
     * @return true if successful, false otherwise
     */
    bool setLoRaMode(LoRaMode mode);

    /**
     * @brief Start LoRa receive mode
     * @param callback Callback for received packets
     * @return true if successful, false otherwise
     */
    bool startLoRaReceive(LoRaReceiveCallback callback);

    /**
     * @brief Update network configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    bool updateConfig(const NetworkConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const NetworkConfig& getConfig() const { return m_config; }

    /**
     * @brief Get communication statistics
     * @return Current statistics
     */
    CommStats getStats() const;

    /**
     * @brief Reset all statistics
     */
    void resetStats();

    /**
     * @brief Set event callback
     * @param callback Event callback function
     */
    void setEventCallback(CommEventCallback callback) { m_eventCallback = callback; }

    /**
     * @brief Process communication events (call from main loop)
     */
    void process();

    /**
     * @brief Enable/disable interface
     * @param interface Interface to control
     * @param enable Enable or disable
     * @return true if successful, false otherwise
     */
    bool enableInterface(CommInterface interface, bool enable);

    /**
     * @brief Check if interface is enabled
     * @param interface Interface to check
     * @return true if enabled, false otherwise
     */
    bool isInterfaceEnabled(CommInterface interface) const;

    /**
     * @brief Get signal strength for interface
     * @param interface Interface to check
     * @return Signal strength (RSSI in dBm)
     */
    int16_t getSignalStrength(CommInterface interface) const;

private:
    // Communication managers
    LoRaManager m_loraManager;
    WiFiManager m_wifiManager;
    CellularManager m_cellularManager;
    
    // Configuration
    NetworkConfig m_config;
    bool m_initialized;
    
    // Statistics
    mutable CommStats m_stats;
    uint32_t m_initTime;
    uint32_t m_lastInterfaceSwitch;
    
    // Callbacks
    CommEventCallback m_eventCallback;
    
    // FreeRTOS
    SemaphoreHandle_t m_mutex;
    
    // Internal methods
    void handleLoRaEvent(bool success, int errorCode);
    void handleWiFiEvent(WiFiStatus status, const String& info);
    void handleCellularEvent(CellularStatus status, const String& info);
    void updateStats();
    bool initializeInterface(CommInterface interface);
    void deinitializeInterface(CommInterface interface);
    CommInterface selectBestInterface() const;
};

} // namespace Communication
} // namespace TDeckOS