/**
 * @file lora_manager.h
 * @brief LoRa communication manager for T-Deck-Pro OS
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#pragma once

#include <RadioLib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "core/hal/board_config.h"
#include "core/utils/logger.h"

namespace TDeckOS {
namespace Communication {

/**
 * @brief LoRa operating modes
 */
enum class LoRaMode {
    IDLE,
    TRANSMIT,
    RECEIVE,
    SLEEP
};

/**
 * @brief LoRa configuration parameters
 */
struct LoRaConfig {
    float frequency = 850.0;        // MHz
    float bandwidth = 125.0;        // kHz
    uint8_t spreadingFactor = 10;   // SF10
    uint8_t codingRate = 6;         // CR 4/6
    int8_t outputPower = 22;        // dBm
    uint16_t preambleLength = 15;   // symbols
    uint8_t syncWord = 0xAB;        // LoRa sync word
    float tcxoVoltage = 2.4;        // V
    uint8_t currentLimit = 140;     // mA
    bool crcEnabled = false;
};

/**
 * @brief LoRa packet structure
 */
struct LoRaPacket {
    uint8_t* data;
    size_t length;
    int16_t rssi;
    float snr;
    float frequencyError;
    uint32_t timestamp;
    bool isValid;
};

/**
 * @brief LoRa statistics
 */
struct LoRaStats {
    uint32_t packetsTransmitted;
    uint32_t packetsReceived;
    uint32_t transmissionErrors;
    uint32_t receptionErrors;
    uint32_t crcErrors;
    int16_t lastRssi;
    float lastSnr;
    uint32_t uptime;
};

/**
 * @brief Callback function types
 */
typedef void (*LoRaTransmitCallback)(bool success, int errorCode);
typedef void (*LoRaReceiveCallback)(const LoRaPacket& packet);

/**
 * @brief LoRa Manager class for SX1262 radio
 */
class LoRaManager {
public:
    /**
     * @brief Constructor
     */
    LoRaManager();

    /**
     * @brief Destructor
     */
    ~LoRaManager();

    /**
     * @brief Initialize LoRa radio
     * @param config LoRa configuration parameters
     * @return true if successful, false otherwise
     */
    bool initialize(const LoRaConfig& config = LoRaConfig{});

    /**
     * @brief Deinitialize LoRa radio
     */
    void deinitialize();

    /**
     * @brief Check if LoRa is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Set LoRa operating mode
     * @param mode Operating mode
     * @return true if successful, false otherwise
     */
    bool setMode(LoRaMode mode);

    /**
     * @brief Get current operating mode
     * @return Current mode
     */
    LoRaMode getMode() const { return m_currentMode; }

    /**
     * @brief Transmit data
     * @param data Data to transmit
     * @param length Data length
     * @param callback Optional callback for transmission result
     * @return true if transmission started, false otherwise
     */
    bool transmit(const uint8_t* data, size_t length, LoRaTransmitCallback callback = nullptr);

    /**
     * @brief Transmit string
     * @param message String to transmit
     * @param callback Optional callback for transmission result
     * @return true if transmission started, false otherwise
     */
    bool transmit(const String& message, LoRaTransmitCallback callback = nullptr);

    /**
     * @brief Start continuous receive mode
     * @param callback Callback for received packets
     * @return true if successful, false otherwise
     */
    bool startReceive(LoRaReceiveCallback callback);

    /**
     * @brief Stop receive mode
     */
    void stopReceive();

    /**
     * @brief Check if currently receiving
     * @return true if in receive mode, false otherwise
     */
    bool isReceiving() const { return m_currentMode == LoRaMode::RECEIVE; }

    /**
     * @brief Set sleep mode
     * @return true if successful, false otherwise
     */
    bool sleep();

    /**
     * @brief Wake up from sleep
     * @return true if successful, false otherwise
     */
    bool wakeup();

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    bool updateConfig(const LoRaConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const LoRaConfig& getConfig() const { return m_config; }

    /**
     * @brief Get statistics
     * @return Current statistics
     */
    LoRaStats getStats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Get last RSSI
     * @return RSSI in dBm
     */
    int16_t getLastRssi() const;

    /**
     * @brief Get last SNR
     * @return SNR in dB
     */
    float getLastSnr() const;

    /**
     * @brief Get frequency error
     * @return Frequency error in Hz
     */
    float getFrequencyError() const;

    /**
     * @brief Check if radio is busy
     * @return true if busy, false otherwise
     */
    bool isBusy() const;

    /**
     * @brief Process LoRa events (call from main loop)
     */
    void process();

private:
    // Hardware
    SX1262* m_radio;
    SPIClass* m_spi;
    
    // Configuration
    LoRaConfig m_config;
    bool m_initialized;
    LoRaMode m_currentMode;
    
    // Callbacks
    LoRaTransmitCallback m_transmitCallback;
    LoRaReceiveCallback m_receiveCallback;
    
    // Statistics
    mutable LoRaStats m_stats;
    uint32_t m_initTime;
    
    // FreeRTOS
    TaskHandle_t m_taskHandle;
    QueueHandle_t m_eventQueue;
    SemaphoreHandle_t m_mutex;
    
    // Flags
    volatile bool m_transmittedFlag;
    volatile bool m_receivedFlag;
    
    // Internal methods
    bool configureRadio();
    void enableInterrupts();
    void disableInterrupts();
    static void transmitISR();
    static void receiveISR();
    static void loraTask(void* parameter);
    void handleTransmitComplete();
    void handleReceiveComplete();
    void updateStats();
    
    // Static instance for ISR
    static LoRaManager* s_instance;
};

} // namespace Communication
} // namespace TDeckOS