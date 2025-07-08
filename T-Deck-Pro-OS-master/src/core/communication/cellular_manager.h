/**
 * @file cellular_manager.h
 * @brief Cellular communication manager for T-Deck-Pro OS (A7682E)
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#pragma once

#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "core/hal/board_config.h"
#include "core/utils/logger.h"

namespace TDeckOS {
namespace Communication {

/**
 * @brief Cellular network types
 */
enum class CellularNetworkType {
    UNKNOWN,
    GSM,
    GPRS,
    EDGE,
    UMTS,
    HSDPA,
    HSUPA,
    HSPA,
    LTE,
    LTE_CAT_M1,
    LTE_NB_IOT
};

/**
 * @brief Cellular connection status
 */
enum class CellularStatus {
    OFF,
    INITIALIZING,
    SEARCHING,
    REGISTERED,
    CONNECTED,
    DISCONNECTED,
    ERROR
};

/**
 * @brief SIM card status
 */
enum class SIMStatus {
    UNKNOWN,
    READY,
    PIN_REQUIRED,
    PUK_REQUIRED,
    NOT_INSERTED,
    ERROR
};

/**
 * @brief Network registration status
 */
enum class NetworkRegistration {
    NOT_REGISTERED,
    REGISTERED_HOME,
    SEARCHING,
    REGISTRATION_DENIED,
    REGISTERED_ROAMING,
    UNKNOWN
};

/**
 * @brief Cellular network information
 */
struct CellularNetworkInfo {
    String operatorName;
    String mcc; // Mobile Country Code
    String mnc; // Mobile Network Code
    int16_t rssi;
    uint8_t signalQuality;
    CellularNetworkType networkType;
    NetworkRegistration registration;
};

/**
 * @brief APN configuration
 */
struct APNConfig {
    String apn;
    String username;
    String password;
    String authType = "PAP"; // PAP, CHAP, or NONE
};

/**
 * @brief Cellular configuration
 */
struct CellularConfig {
    String pin;
    APNConfig apnConfig;
    uint32_t baudRate = 115200;
    uint32_t initTimeoutMs = 30000;
    uint32_t connectTimeoutMs = 60000;
    bool autoConnect = true;
    uint8_t maxRetries = 3;
};

/**
 * @brief SMS message
 */
struct SMSMessage {
    String sender;
    String message;
    String timestamp;
    bool isRead;
    uint16_t index;
};

/**
 * @brief Cellular statistics
 */
struct CellularStats {
    uint32_t connectAttempts;
    uint32_t successfulConnections;
    uint32_t disconnections;
    uint32_t dataBytesSent;
    uint32_t dataBytesReceived;
    uint32_t smsMessagesSent;
    uint32_t smsMessagesReceived;
    uint32_t uptime;
    int16_t lastRssi;
    uint8_t lastSignalQuality;
};

/**
 * @brief Callback function types
 */
typedef void (*CellularEventCallback)(CellularStatus status, const String& info);
typedef void (*SMSCallback)(const SMSMessage& message);
typedef void (*CallCallback)(const String& number, bool incoming);

/**
 * @brief Cellular Manager class for A7682E modem
 */
class CellularManager {
public:
    /**
     * @brief Constructor
     */
    CellularManager();

    /**
     * @brief Destructor
     */
    ~CellularManager();

    /**
     * @brief Initialize cellular manager
     * @param config Cellular configuration
     * @return true if successful, false otherwise
     */
    bool initialize(const CellularConfig& config = CellularConfig{});

    /**
     * @brief Deinitialize cellular manager
     */
    void deinitialize();

    /**
     * @brief Check if cellular is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Power on the modem
     * @return true if successful, false otherwise
     */
    bool powerOn();

    /**
     * @brief Power off the modem
     * @return true if successful, false otherwise
     */
    bool powerOff();

    /**
     * @brief Check if modem is powered on
     * @return true if powered on, false otherwise
     */
    bool isPoweredOn() const { return m_poweredOn; }

    /**
     * @brief Connect to cellular network
     * @param callback Optional callback for connection events
     * @return true if connection started, false otherwise
     */
    bool connect(CellularEventCallback callback = nullptr);

    /**
     * @brief Disconnect from cellular network
     */
    void disconnect();

    /**
     * @brief Get connection status
     * @return Current status
     */
    CellularStatus getStatus() const { return m_status; }

    /**
     * @brief Check if connected to network
     * @return true if connected, false otherwise
     */
    bool isConnected() const { return m_status == CellularStatus::CONNECTED; }

    /**
     * @brief Get SIM card status
     * @return SIM status
     */
    SIMStatus getSIMStatus();

    /**
     * @brief Get network information
     * @return Network information
     */
    CellularNetworkInfo getNetworkInfo();

    /**
     * @brief Get signal strength (RSSI)
     * @return RSSI in dBm
     */
    int16_t getSignalStrength();

    /**
     * @brief Get signal quality (0-31)
     * @return Signal quality
     */
    uint8_t getSignalQuality();

    /**
     * @brief Send SMS message
     * @param number Phone number
     * @param message Message text
     * @return true if successful, false otherwise
     */
    bool sendSMS(const String& number, const String& message);

    /**
     * @brief Read SMS messages
     * @param unreadOnly Read only unread messages
     * @return Vector of SMS messages
     */
    std::vector<SMSMessage> readSMS(bool unreadOnly = true);

    /**
     * @brief Delete SMS message
     * @param index Message index
     * @return true if successful, false otherwise
     */
    bool deleteSMS(uint16_t index);

    /**
     * @brief Set SMS callback
     * @param callback SMS callback function
     */
    void setSMSCallback(SMSCallback callback) { m_smsCallback = callback; }

    /**
     * @brief Make voice call
     * @param number Phone number to call
     * @return true if successful, false otherwise
     */
    bool makeCall(const String& number);

    /**
     * @brief Answer incoming call
     * @return true if successful, false otherwise
     */
    bool answerCall();

    /**
     * @brief Hang up call
     * @return true if successful, false otherwise
     */
    bool hangupCall();

    /**
     * @brief Set call callback
     * @param callback Call callback function
     */
    void setCallCallback(CallCallback callback) { m_callCallback = callback; }

    /**
     * @brief Send AT command
     * @param command AT command
     * @param response Response buffer
     * @param timeoutMs Timeout in milliseconds
     * @return true if successful, false otherwise
     */
    bool sendATCommand(const String& command, String& response, uint32_t timeoutMs = 1000);

    /**
     * @brief Get modem information
     * @return Modem info string
     */
    String getModemInfo();

    /**
     * @brief Get IMEI
     * @return IMEI string
     */
    String getIMEI();

    /**
     * @brief Get ICCID (SIM card ID)
     * @return ICCID string
     */
    String getICCID();

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    bool updateConfig(const CellularConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const CellularConfig& getConfig() const { return m_config; }

    /**
     * @brief Get statistics
     * @return Current statistics
     */
    CellularStats getStats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Process cellular events (call from main loop)
     */
    void process();

    /**
     * @brief Set event callback
     * @param callback Event callback function
     */
    void setEventCallback(CellularEventCallback callback) { m_eventCallback = callback; }

private:
    // Hardware
    HardwareSerial* m_serial;
    
    // Configuration
    CellularConfig m_config;
    bool m_initialized;
    bool m_poweredOn;
    CellularStatus m_status;
    
    // Callbacks
    CellularEventCallback m_eventCallback;
    SMSCallback m_smsCallback;
    CallCallback m_callCallback;
    
    // Statistics
    mutable CellularStats m_stats;
    uint32_t m_initTime;
    uint32_t m_lastConnectAttempt;
    uint8_t m_retryCount;
    
    // FreeRTOS
    TaskHandle_t m_taskHandle;
    QueueHandle_t m_commandQueue;
    SemaphoreHandle_t m_mutex;
    
    // Internal state
    String m_responseBuffer;
    uint32_t m_lastActivity;
    
    // Internal methods
    static void cellularTask(void* parameter);
    bool initializeModem();
    bool checkModemResponse();
    bool waitForResponse(const String& expected, uint32_t timeoutMs);
    bool setupPDP();
    void handleIncomingData();
    void parseNetworkRegistration(const String& response);
    void parseSignalQuality(const String& response);
    void parseSMSNotification(const String& response);
    void parseCallNotification(const String& response);
    void updateStats();
    CellularNetworkType parseNetworkType(const String& response);
    void powerCycle();
    bool checkSIMCard();
    void processATResponse(const String& response);
};

} // namespace Communication
} // namespace TDeckOS