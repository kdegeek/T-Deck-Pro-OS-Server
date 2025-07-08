/**
 * @file lora_manager.cpp
 * @brief LoRa communication manager implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "lora_manager.h"
#include <Arduino.h>
#include <SPI.h>

namespace TDeckOS {
namespace Communication {

// Static instance for ISR
LoRaManager* LoRaManager::s_instance = nullptr;

LoRaManager::LoRaManager()
    : m_radio(nullptr)
    , m_spi(nullptr)
    , m_initialized(false)
    , m_currentMode(LoRaMode::IDLE)
    , m_transmitCallback(nullptr)
    , m_receiveCallback(nullptr)
    , m_stats{}
    , m_initTime(0)
    , m_taskHandle(nullptr)
    , m_eventQueue(nullptr)
    , m_mutex(nullptr)
    , m_transmittedFlag(false)
    , m_receivedFlag(false)
{
    s_instance = this;
}

LoRaManager::~LoRaManager() {
    deinitialize();
    s_instance = nullptr;
}

bool LoRaManager::initialize(const LoRaConfig& config) {
    if (m_initialized) {
        LOG_WARN("LoRa", "Already initialized");
        return true;
    }

    LOG_INFO("LoRa", "Initializing LoRa manager...");
    
    // Store configuration
    m_config = config;
    m_initTime = millis();
    
    // Create mutex
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        LOG_ERROR("LoRa", "Failed to create mutex");
        return false;
    }
    
    // Create event queue
    m_eventQueue = xQueueCreate(10, sizeof(uint32_t));
    if (!m_eventQueue) {
        LOG_ERROR("LoRa", "Failed to create event queue");
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    // Enable LoRa module
    pinMode(BOARD_LORA_EN, OUTPUT);
    digitalWrite(BOARD_LORA_EN, HIGH);
    delay(100);
    
    // Initialize SPI
    m_spi = &SPI;
    m_spi->begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, BOARD_LORA_CS);
    
    // Create radio instance
    m_radio = new SX1262(new Module(BOARD_LORA_CS, BOARD_LORA_INT, BOARD_LORA_RST, BOARD_LORA_BUSY));
    
    // Initialize radio
    LOG_INFO("LoRa", "Initializing SX1262 radio...");
    int state = m_radio->begin(m_config.frequency);
    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("LoRa", "Failed to initialize radio, code: %d", state);
        delete m_radio;
        m_radio = nullptr;
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    // Configure radio parameters
    if (!configureRadio()) {
        LOG_ERROR("LoRa", "Failed to configure radio");
        delete m_radio;
        m_radio = nullptr;
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    // Create LoRa task
    BaseType_t result = xTaskCreate(
        loraTask,
        "LoRaTask",
        4096,
        this,
        LORA_TASK_PRIORITY,
        &m_taskHandle
    );
    
    if (result != pdPASS) {
        LOG_ERROR("LoRa", "Failed to create LoRa task");
        delete m_radio;
        m_radio = nullptr;
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    m_initialized = true;
    m_currentMode = LoRaMode::IDLE;
    
    // Reset statistics
    resetStats();
    
    LOG_INFO("LoRa", "LoRa manager initialized successfully");
    return true;
}

void LoRaManager::deinitialize() {
    if (!m_initialized) {
        return;
    }
    
    LOG_INFO("LoRa", "Deinitializing LoRa manager...");
    
    // Stop task
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    
    // Disable interrupts
    disableInterrupts();
    
    // Put radio to sleep
    if (m_radio) {
        m_radio->sleep();
        delete m_radio;
        m_radio = nullptr;
    }
    
    // Disable LoRa module
    digitalWrite(BOARD_LORA_EN, LOW);
    
    // Clean up FreeRTOS objects
    if (m_eventQueue) {
        vQueueDelete(m_eventQueue);
        m_eventQueue = nullptr;
    }
    
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
    
    m_initialized = false;
    m_currentMode = LoRaMode::IDLE;
    
    LOG_INFO("LoRa", "LoRa manager deinitialized");
}

bool LoRaManager::setMode(LoRaMode mode) {
    if (!m_initialized) {
        LOG_ERROR("LoRa", "Not initialized");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("LoRa", "Failed to acquire mutex");
        return false;
    }
    
    bool success = true;
    
    switch (mode) {
        case LoRaMode::IDLE:
            disableInterrupts();
            m_radio->standby();
            break;
            
        case LoRaMode::TRANSMIT:
            // Mode will be set when transmit is called
            break;
            
        case LoRaMode::RECEIVE:
            enableInterrupts();
            m_radio->setPacketReceivedAction(receiveISR);
            if (m_radio->startReceive() != RADIOLIB_ERR_NONE) {
                success = false;
            }
            break;
            
        case LoRaMode::SLEEP:
            disableInterrupts();
            m_radio->sleep();
            break;
            
        default:
            success = false;
            break;
    }
    
    if (success) {
        m_currentMode = mode;
        LOG_DEBUG("LoRa", "Mode changed to %d", static_cast<int>(mode));
    }
    
    xSemaphoreGive(m_mutex);
    return success;
}

bool LoRaManager::transmit(const uint8_t* data, size_t length, LoRaTransmitCallback callback) {
    if (!m_initialized) {
        LOG_ERROR("LoRa", "Not initialized");
        return false;
    }
    
    if (!data || length == 0 || length > 255) {
        LOG_ERROR("LoRa", "Invalid data or length");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("LoRa", "Failed to acquire mutex for transmission");
        return false;
    }
    
    m_transmitCallback = callback;
    m_transmittedFlag = false;
    
    // Set transmit interrupt
    m_radio->setPacketSentAction(transmitISR);
    
    // Start transmission
    int state = m_radio->startTransmit(const_cast<uint8_t*>(data), length);
    
    if (state == RADIOLIB_ERR_NONE) {
        m_currentMode = LoRaMode::TRANSMIT;
        LOG_DEBUG("LoRa", "Started transmission of %d bytes", length);
        xSemaphoreGive(m_mutex);
        return true;
    } else {
        LOG_ERROR("LoRa", "Failed to start transmission, code: %d", state);
        m_transmitCallback = nullptr;
        xSemaphoreGive(m_mutex);
        return false;
    }
}

bool LoRaManager::transmit(const String& message, LoRaTransmitCallback callback) {
    return transmit(reinterpret_cast<const uint8_t*>(message.c_str()), message.length(), callback);
}

bool LoRaManager::startReceive(LoRaReceiveCallback callback) {
    if (!m_initialized) {
        LOG_ERROR("LoRa", "Not initialized");
        return false;
    }
    
    m_receiveCallback = callback;
    return setMode(LoRaMode::RECEIVE);
}

void LoRaManager::stopReceive() {
    if (m_currentMode == LoRaMode::RECEIVE) {
        setMode(LoRaMode::IDLE);
        m_receiveCallback = nullptr;
    }
}

bool LoRaManager::sleep() {
    return setMode(LoRaMode::SLEEP);
}

bool LoRaManager::wakeup() {
    return setMode(LoRaMode::IDLE);
}

bool LoRaManager::updateConfig(const LoRaConfig& config) {
    if (!m_initialized) {
        LOG_ERROR("LoRa", "Not initialized");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("LoRa", "Failed to acquire mutex");
        return false;
    }
    
    LoRaMode oldMode = m_currentMode;
    setMode(LoRaMode::IDLE);
    
    m_config = config;
    bool success = configureRadio();
    
    if (success) {
        LOG_INFO("LoRa", "Configuration updated successfully");
        setMode(oldMode);
    } else {
        LOG_ERROR("LoRa", "Failed to update configuration");
    }
    
    xSemaphoreGive(m_mutex);
    return success;
}

LoRaStats LoRaManager::getStats() const {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        LoRaStats stats = m_stats;
        xSemaphoreGive(m_mutex);
        return stats;
    }
    return LoRaStats{};
}

void LoRaManager::resetStats() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats = LoRaStats{};
        m_initTime = millis();
        xSemaphoreGive(m_mutex);
        LOG_INFO("LoRa", "Statistics reset");
    }
}

int16_t LoRaManager::getLastRssi() const {
    if (m_initialized && m_radio) {
        return m_radio->getRSSI();
    }
    return 0;
}

float LoRaManager::getLastSnr() const {
    if (m_initialized && m_radio) {
        return m_radio->getSNR();
    }
    return 0.0f;
}

float LoRaManager::getFrequencyError() const {
    if (m_initialized && m_radio) {
        return m_radio->getFrequencyError();
    }
    return 0.0f;
}

bool LoRaManager::isBusy() const {
    if (m_initialized) {
        return digitalRead(BOARD_LORA_BUSY) == HIGH;
    }
    return false;
}

void LoRaManager::process() {
    // This method can be called from main loop for additional processing
    // Most work is done in the FreeRTOS task
}

bool LoRaManager::configureRadio() {
    if (!m_radio) {
        return false;
    }
    
    LOG_INFO("LoRa", "Configuring radio parameters...");
    
    // Set frequency
    if (m_radio->setFrequency(m_config.frequency) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        LOG_ERROR("LoRa", "Invalid frequency: %.1f MHz", m_config.frequency);
        return false;
    }
    
    // Set bandwidth
    if (m_radio->setBandwidth(m_config.bandwidth) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        LOG_ERROR("LoRa", "Invalid bandwidth: %.1f kHz", m_config.bandwidth);
        return false;
    }
    
    // Set spreading factor
    if (m_radio->setSpreadingFactor(m_config.spreadingFactor) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        LOG_ERROR("LoRa", "Invalid spreading factor: %d", m_config.spreadingFactor);
        return false;
    }
    
    // Set coding rate
    if (m_radio->setCodingRate(m_config.codingRate) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        LOG_ERROR("LoRa", "Invalid coding rate: %d", m_config.codingRate);
        return false;
    }
    
    // Set sync word
    if (m_radio->setSyncWord(m_config.syncWord) != RADIOLIB_ERR_NONE) {
        LOG_ERROR("LoRa", "Failed to set sync word: 0x%02X", m_config.syncWord);
        return false;
    }
    
    // Set output power
    if (m_radio->setOutputPower(m_config.outputPower) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        LOG_ERROR("LoRa", "Invalid output power: %d dBm", m_config.outputPower);
        return false;
    }
    
    // Set current limit
    if (m_radio->setCurrentLimit(m_config.currentLimit) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        LOG_ERROR("LoRa", "Invalid current limit: %d mA", m_config.currentLimit);
        return false;
    }
    
    // Set preamble length
    if (m_radio->setPreambleLength(m_config.preambleLength) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        LOG_ERROR("LoRa", "Invalid preamble length: %d", m_config.preambleLength);
        return false;
    }
    
    // Set CRC
    if (m_radio->setCRC(m_config.crcEnabled) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        LOG_ERROR("LoRa", "Invalid CRC configuration");
        return false;
    }
    
    // Set TCXO voltage
    if (m_radio->setTCXO(m_config.tcxoVoltage) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
        LOG_ERROR("LoRa", "Invalid TCXO voltage: %.1f V", m_config.tcxoVoltage);
        return false;
    }
    
    // Set DIO2 as RF switch
    if (m_radio->setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
        LOG_ERROR("LoRa", "Failed to set DIO2 as RF switch");
        return false;
    }
    
    LOG_INFO("LoRa", "Radio configured successfully");
    LOG_INFO("LoRa", "  Frequency: %.1f MHz", m_config.frequency);
    LOG_INFO("LoRa", "  Bandwidth: %.1f kHz", m_config.bandwidth);
    LOG_INFO("LoRa", "  SF: %d, CR: %d", m_config.spreadingFactor, m_config.codingRate);
    LOG_INFO("LoRa", "  Power: %d dBm", m_config.outputPower);
    
    return true;
}

void LoRaManager::enableInterrupts() {
    // Interrupts are handled by RadioLib
}

void LoRaManager::disableInterrupts() {
    if (m_radio) {
        m_radio->clearPacketSentAction();
        m_radio->clearPacketReceivedAction();
    }
}

void IRAM_ATTR LoRaManager::transmitISR() {
    if (s_instance) {
        s_instance->m_transmittedFlag = true;
        uint32_t event = 1; // Transmit event
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(s_instance->m_eventQueue, &event, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void IRAM_ATTR LoRaManager::receiveISR() {
    if (s_instance) {
        s_instance->m_receivedFlag = true;
        uint32_t event = 2; // Receive event
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(s_instance->m_eventQueue, &event, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void LoRaManager::loraTask(void* parameter) {
    LoRaManager* manager = static_cast<LoRaManager*>(parameter);
    uint32_t event;
    
    LOG_INFO("LoRa", "LoRa task started");
    
    while (true) {
        if (xQueueReceive(manager->m_eventQueue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
            switch (event) {
                case 1: // Transmit complete
                    manager->handleTransmitComplete();
                    break;
                case 2: // Receive complete
                    manager->handleReceiveComplete();
                    break;
                default:
                    break;
            }
        }
        
        // Update statistics periodically
        manager->updateStats();
    }
}

void LoRaManager::handleTransmitComplete() {
    if (!m_transmittedFlag) {
        return;
    }
    
    m_transmittedFlag = false;
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Finish transmission
        int state = m_radio->finishTransmit();
        bool success = (state == RADIOLIB_ERR_NONE);
        
        if (success) {
            m_stats.packetsTransmitted++;
            LOG_DEBUG("LoRa", "Transmission completed successfully");
        } else {
            m_stats.transmissionErrors++;
            LOG_ERROR("LoRa", "Transmission failed, code: %d", state);
        }
        
        // Call callback if set
        if (m_transmitCallback) {
            m_transmitCallback(success, state);
            m_transmitCallback = nullptr;
        }
        
        // Return to idle mode
        m_currentMode = LoRaMode::IDLE;
        
        xSemaphoreGive(m_mutex);
    }
}

void LoRaManager::handleReceiveComplete() {
    if (!m_receivedFlag) {
        return;
    }
    
    m_receivedFlag = false;
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Read received data
        String receivedData;
        int state = m_radio->readData(receivedData);
        
        if (state == RADIOLIB_ERR_NONE) {
            // Create packet structure
            LoRaPacket packet;
            packet.data = reinterpret_cast<uint8_t*>(const_cast<char*>(receivedData.c_str()));
            packet.length = receivedData.length();
            packet.rssi = m_radio->getRSSI();
            packet.snr = m_radio->getSNR();
            packet.frequencyError = m_radio->getFrequencyError();
            packet.timestamp = millis();
            packet.isValid = true;
            
            m_stats.packetsReceived++;
            m_stats.lastRssi = packet.rssi;
            m_stats.lastSnr = packet.snr;
            
            LOG_DEBUG("LoRa", "Received packet: %d bytes, RSSI: %d dBm, SNR: %.1f dB", 
                     packet.length, packet.rssi, packet.snr);
            
            // Call callback if set
            if (m_receiveCallback) {
                m_receiveCallback(packet);
            }
            
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            m_stats.crcErrors++;
            LOG_WARN("LoRa", "CRC error in received packet");
        } else {
            m_stats.receptionErrors++;
            LOG_ERROR("LoRa", "Reception failed, code: %d", state);
        }
        
        // Continue receiving if still in receive mode
        if (m_currentMode == LoRaMode::RECEIVE) {
            m_radio->startReceive();
        }
        
        xSemaphoreGive(m_mutex);
    }
}

void LoRaManager::updateStats() {
    // Update uptime and other periodic statistics
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        xSemaphoreGive(m_mutex);
    }
}

} // namespace Communication
} // namespace TDeckOS