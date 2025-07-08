/**
 * @file wifi_manager.cpp
 * @brief WiFi communication manager implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "wifi_manager.h"
#include <Arduino.h>

namespace TDeckOS {
namespace Communication {

// Static instance for event handler
WiFiManager* WiFiManager::s_instance = nullptr;

WiFiManager::WiFiManager()
    : m_initialized(false)
    , m_currentMode(WiFiMode::OFF)
    , m_status(WiFiStatus::DISCONNECTED)
    , m_eventCallback(nullptr)
    , m_scanCallback(nullptr)
    , m_stats{}
    , m_initTime(0)
    , m_lastConnectAttempt(0)
    , m_retryCount(0)
    , m_taskHandle(nullptr)
    , m_eventQueue(nullptr)
    , m_mutex(nullptr)
{
    s_instance = this;
}

WiFiManager::~WiFiManager() {
    deinitialize();
    s_instance = nullptr;
}

bool WiFiManager::initialize() {
    if (m_initialized) {
        LOG_WARN("WiFi", "Already initialized");
        return true;
    }

    LOG_INFO("WiFi", "Initializing WiFi manager...");
    
    m_initTime = millis();
    
    // Create mutex
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        LOG_ERROR("WiFi", "Failed to create mutex");
        return false;
    }
    
    // Create event queue
    m_eventQueue = xQueueCreate(20, sizeof(WiFiEvent_t));
    if (!m_eventQueue) {
        LOG_ERROR("WiFi", "Failed to create event queue");
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    // Set WiFi event handler
    WiFi.onEvent(wifiEventHandler);
    
    // Create WiFi task
    BaseType_t result = xTaskCreate(
        wifiTask,
        "WiFiTask",
        4096,
        this,
        WIFI_TASK_PRIORITY,
        &m_taskHandle
    );
    
    if (result != pdPASS) {
        LOG_ERROR("WiFi", "Failed to create WiFi task");
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_mutex);
        return false;
    }
    
    m_initialized = true;
    m_currentMode = WiFiMode::OFF;
    m_status = WiFiStatus::DISCONNECTED;
    
    // Reset statistics
    resetStats();
    
    LOG_INFO("WiFi", "WiFi manager initialized successfully");
    return true;
}

void WiFiManager::deinitialize() {
    if (!m_initialized) {
        return;
    }
    
    LOG_INFO("WiFi", "Deinitializing WiFi manager...");
    
    // Stop task
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    
    // Disconnect and turn off WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
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
    m_currentMode = WiFiMode::OFF;
    m_status = WiFiStatus::DISCONNECTED;
    
    LOG_INFO("WiFi", "WiFi manager deinitialized");
}

bool WiFiManager::setMode(WiFiMode mode) {
    if (!m_initialized) {
        LOG_ERROR("WiFi", "Not initialized");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("WiFi", "Failed to acquire mutex");
        return false;
    }
    
    bool success = true;
    
    switch (mode) {
        case WiFiMode::OFF:
            WiFi.mode(WIFI_OFF);
            m_status = WiFiStatus::DISCONNECTED;
            break;
            
        case WiFiMode::STATION:
            WiFi.mode(WIFI_STA);
            break;
            
        case WiFiMode::ACCESS_POINT:
            WiFi.mode(WIFI_AP);
            break;
            
        case WiFiMode::STATION_AP:
            WiFi.mode(WIFI_AP_STA);
            break;
            
        default:
            success = false;
            break;
    }
    
    if (success) {
        m_currentMode = mode;
        LOG_DEBUG("WiFi", "Mode changed to %d", static_cast<int>(mode));
    }
    
    xSemaphoreGive(m_mutex);
    return success;
}

bool WiFiManager::connect(const WiFiStationConfig& config, WiFiEventCallback callback) {
    if (!m_initialized) {
        LOG_ERROR("WiFi", "Not initialized");
        return false;
    }
    
    if (config.ssid.isEmpty()) {
        LOG_ERROR("WiFi", "SSID cannot be empty");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("WiFi", "Failed to acquire mutex");
        return false;
    }
    
    m_stationConfig = config;
    m_eventCallback = callback;
    m_retryCount = 0;
    
    // Set station mode if not already set
    if (m_currentMode != WiFiMode::STATION && m_currentMode != WiFiMode::STATION_AP) {
        setMode(WiFiMode::STATION);
    }
    
    // Configure station
    if (!configureStation()) {
        xSemaphoreGive(m_mutex);
        return false;
    }
    
    // Start connection
    m_status = WiFiStatus::CONNECTING;
    m_lastConnectAttempt = millis();
    m_stats.connectAttempts++;
    
    LOG_INFO("WiFi", "Connecting to '%s'...", config.ssid.c_str());
    
    if (config.password.isEmpty()) {
        WiFi.begin(config.ssid.c_str());
    } else {
        WiFi.begin(config.ssid.c_str(), config.password.c_str());
    }
    
    xSemaphoreGive(m_mutex);
    return true;
}

void WiFiManager::disconnect() {
    if (m_initialized) {
        LOG_INFO("WiFi", "Disconnecting from WiFi...");
        WiFi.disconnect();
        m_status = WiFiStatus::DISCONNECTED;
    }
}

bool WiFiManager::startAP(const WiFiAPConfig& config) {
    if (!m_initialized) {
        LOG_ERROR("WiFi", "Not initialized");
        return false;
    }
    
    if (config.ssid.isEmpty()) {
        LOG_ERROR("WiFi", "AP SSID cannot be empty");
        return false;
    }
    
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        LOG_ERROR("WiFi", "Failed to acquire mutex");
        return false;
    }
    
    m_apConfig = config;
    
    // Set AP mode if not already set
    if (m_currentMode != WiFiMode::ACCESS_POINT && m_currentMode != WiFiMode::STATION_AP) {
        setMode(WiFiMode::ACCESS_POINT);
    }
    
    // Configure AP
    bool success = configureAP();
    
    if (success) {
        LOG_INFO("WiFi", "Access Point '%s' started", config.ssid.c_str());
        LOG_INFO("WiFi", "IP address: %s", config.ip.toString().c_str());
    }
    
    xSemaphoreGive(m_mutex);
    return success;
}

void WiFiManager::stopAP() {
    if (m_initialized) {
        LOG_INFO("WiFi", "Stopping Access Point...");
        WiFi.softAPdisconnect(true);
    }
}

bool WiFiManager::scanNetworks(WiFiScanCallback callback, bool async) {
    if (!m_initialized) {
        LOG_ERROR("WiFi", "Not initialized");
        return false;
    }
    
    m_scanCallback = callback;
    m_stats.scanCount++;
    
    LOG_INFO("WiFi", "Starting WiFi scan...");
    
    if (async) {
        return WiFi.scanNetworks(true) != WIFI_SCAN_FAILED;
    } else {
        int n = WiFi.scanNetworks();
        if (n >= 0) {
            std::vector<WiFiNetwork> networks;
            for (int i = 0; i < n; i++) {
                WiFiNetwork network;
                network.ssid = WiFi.SSID(i);
                network.rssi = WiFi.RSSI(i);
                network.channel = WiFi.channel(i);
                network.security = getSecurityType(WiFi.encryptionType(i));
                network.isHidden = network.ssid.isEmpty();
                networks.push_back(network);
            }
            
            if (callback) {
                callback(networks);
            }
            
            WiFi.scanDelete();
            return true;
        }
        return false;
    }
}

IPAddress WiFiManager::getIPAddress() const {
    if (m_currentMode == WiFiMode::STATION || m_currentMode == WiFiMode::STATION_AP) {
        return WiFi.localIP();
    } else if (m_currentMode == WiFiMode::ACCESS_POINT) {
        return WiFi.softAPIP();
    }
    return IPAddress(0, 0, 0, 0);
}

String WiFiManager::getMACAddress() const {
    if (m_currentMode == WiFiMode::STATION || m_currentMode == WiFiMode::STATION_AP) {
        return WiFi.macAddress();
    } else if (m_currentMode == WiFiMode::ACCESS_POINT) {
        return WiFi.softAPmacAddress();
    }
    return String();
}

String WiFiManager::getSSID() const {
    if (m_status == WiFiStatus::CONNECTED) {
        return WiFi.SSID();
    }
    return String();
}

int32_t WiFiManager::getRSSI() const {
    if (m_status == WiFiStatus::CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

uint8_t WiFiManager::getChannel() const {
    if (m_status == WiFiStatus::CONNECTED) {
        return WiFi.channel();
    }
    return 0;
}

uint8_t WiFiManager::getConnectedClients() const {
    if (m_currentMode == WiFiMode::ACCESS_POINT || m_currentMode == WiFiMode::STATION_AP) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

bool WiFiManager::setPowerSave(bool enable) {
    if (!m_initialized) {
        return false;
    }
    
    return WiFi.setSleep(enable);
}

bool WiFiManager::setPower(float power) {
    if (!m_initialized) {
        return false;
    }
    
    return WiFi.setTxPower(static_cast<wifi_power_t>(power * 4)) == ESP_OK;
}

WiFiStats WiFiManager::getStats() const {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        m_stats.lastRssi = getRSSI();
        m_stats.lastChannel = getChannel();
        WiFiStats stats = m_stats;
        xSemaphoreGive(m_mutex);
        return stats;
    }
    return WiFiStats{};
}

void WiFiManager::resetStats() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_stats = WiFiStats{};
        m_initTime = millis();
        xSemaphoreGive(m_mutex);
        LOG_INFO("WiFi", "Statistics reset");
    }
}

void WiFiManager::process() {
    // Most work is done in the FreeRTOS task
    // This method can be called from main loop for additional processing
}

void WiFiManager::wifiTask(void* parameter) {
    WiFiManager* manager = static_cast<WiFiManager*>(parameter);
    WiFiEvent_t event;
    
    LOG_INFO("WiFi", "WiFi task started");
    
    while (true) {
        if (xQueueReceive(manager->m_eventQueue, &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
            manager->handleWiFiEvent(event);
        }
        
        // Check connection status and handle reconnection
        manager->checkConnection();
        
        // Update statistics periodically
        manager->updateStats();
    }
}

void WiFiManager::wifiEventHandler(WiFiEvent_t event) {
    if (s_instance && s_instance->m_eventQueue) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(s_instance->m_eventQueue, &event, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void WiFiManager::handleWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            LOG_DEBUG("WiFi", "Station started");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            LOG_INFO("WiFi", "Connected to WiFi");
            m_status = WiFiStatus::CONNECTED;
            m_stats.successfulConnections++;
            m_retryCount = 0;
            if (m_eventCallback) {
                m_eventCallback(m_status, "Connected");
            }
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            LOG_INFO("WiFi", "Got IP address: %s", WiFi.localIP().toString().c_str());
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            LOG_WARN("WiFi", "Disconnected from WiFi");
            if (m_status == WiFiStatus::CONNECTED) {
                m_stats.disconnections++;
                m_status = WiFiStatus::LOST_CONNECTION;
            } else {
                m_status = WiFiStatus::FAILED;
            }
            if (m_eventCallback) {
                m_eventCallback(m_status, "Disconnected");
            }
            break;
            
        case ARDUINO_EVENT_WIFI_AP_START:
            LOG_INFO("WiFi", "Access Point started");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            LOG_INFO("WiFi", "Client connected to AP");
            break;
            
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            LOG_INFO("WiFi", "Client disconnected from AP");
            break;
            
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            LOG_DEBUG("WiFi", "Scan completed");
            if (m_scanCallback) {
                int n = WiFi.scanComplete();
                if (n >= 0) {
                    std::vector<WiFiNetwork> networks;
                    for (int i = 0; i < n; i++) {
                        WiFiNetwork network;
                        network.ssid = WiFi.SSID(i);
                        network.rssi = WiFi.RSSI(i);
                        network.channel = WiFi.channel(i);
                        network.security = getSecurityType(WiFi.encryptionType(i));
                        network.isHidden = network.ssid.isEmpty();
                        networks.push_back(network);
                    }
                    m_scanCallback(networks);
                    WiFi.scanDelete();
                }
                m_scanCallback = nullptr;
            }
            break;
            
        default:
            break;
    }
}

void WiFiManager::updateStats() {
    // Update uptime and other periodic statistics
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        m_stats.uptime = millis() - m_initTime;
        xSemaphoreGive(m_mutex);
    }
}

WiFiSecurity WiFiManager::getSecurityType(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return WiFiSecurity::OPEN;
        case WIFI_AUTH_WEP: return WiFiSecurity::WEP;
        case WIFI_AUTH_WPA_PSK: return WiFiSecurity::WPA_PSK;
        case WIFI_AUTH_WPA2_PSK: return WiFiSecurity::WPA2_PSK;
        case WIFI_AUTH_WPA_WPA2_PSK: return WiFiSecurity::WPA_WPA2_PSK;
        case WIFI_AUTH_WPA2_ENTERPRISE: return WiFiSecurity::WPA2_ENTERPRISE;
        case WIFI_AUTH_WPA3_PSK: return WiFiSecurity::WPA3_PSK;
        case WIFI_AUTH_WPA2_WPA3_PSK: return WiFiSecurity::WPA2_WPA3_PSK;
        case WIFI_AUTH_WAPI_PSK: return WiFiSecurity::WAPI_PSK;
        default: return WiFiSecurity::UNKNOWN;
    }
}

bool WiFiManager::configureStation() {
    if (!m_stationConfig.useDHCP) {
        if (!WiFi.config(m_stationConfig.staticIP, m_stationConfig.gateway, 
                        m_stationConfig.subnet, m_stationConfig.dns1, m_stationConfig.dns2)) {
            LOG_ERROR("WiFi", "Failed to configure static IP");
            return false;
        }
    }
    
    WiFi.setAutoReconnect(m_stationConfig.autoReconnect);
    return true;
}

bool WiFiManager::configureAP() {
    WiFi.softAPConfig(m_apConfig.ip, m_apConfig.gateway, m_apConfig.subnet);
    
    bool success;
    if (m_apConfig.password.isEmpty()) {
        success = WiFi.softAP(m_apConfig.ssid.c_str(), nullptr, m_apConfig.channel, 
                             m_apConfig.hidden, m_apConfig.maxConnections);
    } else {
        success = WiFi.softAP(m_apConfig.ssid.c_str(), m_apConfig.password.c_str(), 
                             m_apConfig.channel, m_apConfig.hidden, m_apConfig.maxConnections);
    }
    
    return success;
}

void WiFiManager::checkConnection() {
    if (m_status == WiFiStatus::LOST_CONNECTION && m_stationConfig.autoReconnect) {
        uint32_t now = millis();
        if (now - m_lastConnectAttempt > 5000 && m_retryCount < m_stationConfig.maxRetries) {
            LOG_INFO("WiFi", "Attempting to reconnect... (attempt %d/%d)", 
                    m_retryCount + 1, m_stationConfig.maxRetries);
            
            m_retryCount++;
            m_lastConnectAttempt = now;
            m_stats.reconnections++;
            m_status = WiFiStatus::CONNECTING;
            
            WiFi.reconnect();
        }
    }
}

} // namespace Communication
} // namespace TDeckOS