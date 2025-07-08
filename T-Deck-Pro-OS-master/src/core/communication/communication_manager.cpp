/**
 * @file communication_manager.cpp
 * @brief High-level communication manager implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "communication_manager.h"
#include "core/utils/logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

static const char* TAG = "CommMgr";

// Static instance
CommunicationManager* CommunicationManager::instance = nullptr;

CommunicationManager::CommunicationManager() :
    initialized(false),
    activeInterface(COMM_INTERFACE_NONE),
    preferredInterface(COMM_INTERFACE_WIFI),
    autoFailover(true),
    taskHandle(nullptr),
    mutex(nullptr),
    loraManager(nullptr),
    wifiManager(nullptr),
    cellularManager(nullptr)
{
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    
    // Create mutex for thread safety
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
    }
}

CommunicationManager::~CommunicationManager() {
    deinitialize();
    
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

CommunicationManager* CommunicationManager::getInstance() {
    if (!instance) {
        instance = new CommunicationManager();
    }
    return instance;
}

bool CommunicationManager::initialize() {
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing communication manager");
    
    // Initialize individual managers
    loraManager = LoRaManager::getInstance();
    wifiManager = WiFiManager::getInstance();
    cellularManager = CellularManager::getInstance();
    
    if (!loraManager || !wifiManager || !cellularManager) {
        ESP_LOGE(TAG, "Failed to get manager instances");
        return false;
    }
    
    // Initialize all communication interfaces
    bool loraOk = loraManager->initialize();
    bool wifiOk = wifiManager->initialize();
    bool cellularOk = cellularManager->initialize();
    
    if (!loraOk) {
        ESP_LOGW(TAG, "LoRa initialization failed");
    }
    if (!wifiOk) {
        ESP_LOGW(TAG, "WiFi initialization failed");
    }
    if (!cellularOk) {
        ESP_LOGW(TAG, "Cellular initialization failed");
    }
    
    // At least one interface must work
    if (!loraOk && !wifiOk && !cellularOk) {
        ESP_LOGE(TAG, "All communication interfaces failed to initialize");
        return false;
    }
    
    // Create communication manager task
    BaseType_t result = xTaskCreate(
        communicationTask,
        "comm_mgr",
        4096,
        this,
        SYSTEM_TASK_PRIORITY,
        &taskHandle
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create communication task");
        return false;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Communication manager initialized successfully");
    
    // Select initial interface
    selectBestInterface();
    
    return true;
}

void CommunicationManager::deinitialize() {
    if (!initialized) {
        return;
    }
    
    ESP_LOGI(TAG, "Deinitializing communication manager");
    
    // Stop task
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    // Deinitialize all managers
    if (loraManager) {
        loraManager->deinitialize();
    }
    if (wifiManager) {
        wifiManager->deinitialize();
    }
    if (cellularManager) {
        cellularManager->deinitialize();
    }
    
    activeInterface = COMM_INTERFACE_NONE;
    initialized = false;
    
    ESP_LOGI(TAG, "Communication manager deinitialized");
}

bool CommunicationManager::sendMessage(const uint8_t* data, size_t length, comm_interface_t interface) {
    if (!initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return false;
    }
    
    if (!data || length == 0) {
        ESP_LOGE(TAG, "Invalid data parameters");
        return false;
    }
    
    // Use specified interface or active interface
    comm_interface_t targetInterface = (interface == COMM_INTERFACE_AUTO) ? activeInterface : interface;
    
    if (targetInterface == COMM_INTERFACE_NONE) {
        ESP_LOGE(TAG, "No active interface available");
        return false;
    }
    
    bool success = false;
    
    // Take mutex for thread safety
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        switch (targetInterface) {
            case COMM_INTERFACE_LORA:
                if (loraManager && loraManager->isInitialized()) {
                    success = loraManager->transmit(data, length);
                    if (success) {
                        stats.lora.messagesSent++;
                        stats.lora.bytesSent += length;
                    } else {
                        stats.lora.sendErrors++;
                    }
                }
                break;
                
            case COMM_INTERFACE_WIFI:
                // WiFi sending would typically go through a protocol like TCP/UDP
                // For now, we'll mark as successful if WiFi is connected
                if (wifiManager && wifiManager->isConnected()) {
                    success = true; // Placeholder - implement actual WiFi sending
                    stats.wifi.messagesSent++;
                    stats.wifi.bytesSent += length;
                } else {
                    stats.wifi.sendErrors++;
                }
                break;
                
            case COMM_INTERFACE_CELLULAR:
                // Cellular sending would typically go through SMS or data connection
                if (cellularManager && cellularManager->isConnected()) {
                    success = true; // Placeholder - implement actual cellular sending
                    stats.cellular.messagesSent++;
                    stats.cellular.bytesSent += length;
                } else {
                    stats.cellular.sendErrors++;
                }
                break;
                
            default:
                ESP_LOGE(TAG, "Invalid interface: %d", targetInterface);
                break;
        }
        
        xSemaphoreGive(mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take mutex");
    }
    
    // Handle failover if enabled and send failed
    if (!success && autoFailover && interface == COMM_INTERFACE_AUTO) {
        ESP_LOGW(TAG, "Send failed on interface %d, attempting failover", targetInterface);
        success = attemptFailover(data, length);
    }
    
    return success;
}

bool CommunicationManager::receiveMessage(uint8_t* buffer, size_t bufferSize, size_t* receivedLength, comm_interface_t* sourceInterface) {
    if (!initialized || !buffer || bufferSize == 0) {
        return false;
    }
    
    bool messageReceived = false;
    
    // Take mutex for thread safety
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Check LoRa for messages
        if (loraManager && loraManager->isInitialized()) {
            if (loraManager->receive(buffer, bufferSize, receivedLength)) {
                messageReceived = true;
                if (sourceInterface) *sourceInterface = COMM_INTERFACE_LORA;
                stats.lora.messagesReceived++;
                stats.lora.bytesReceived += *receivedLength;
            }
        }
        
        // Check WiFi for messages (placeholder)
        if (!messageReceived && wifiManager && wifiManager->isConnected()) {
            // Placeholder for WiFi message reception
            // Would typically check TCP/UDP sockets or other protocols
        }
        
        // Check Cellular for messages (placeholder)
        if (!messageReceived && cellularManager && cellularManager->isConnected()) {
            // Placeholder for cellular message reception
            // Would typically check SMS or data connection
        }
        
        xSemaphoreGive(mutex);
    }
    
    return messageReceived;
}

bool CommunicationManager::isInterfaceAvailable(comm_interface_t interface) {
    if (!initialized) {
        return false;
    }
    
    switch (interface) {
        case COMM_INTERFACE_LORA:
            return loraManager && loraManager->isInitialized();
            
        case COMM_INTERFACE_WIFI:
            return wifiManager && wifiManager->isConnected();
            
        case COMM_INTERFACE_CELLULAR:
            return cellularManager && cellularManager->isConnected();
            
        default:
            return false;
    }
}

comm_interface_t CommunicationManager::getActiveInterface() {
    return activeInterface;
}

void CommunicationManager::setPreferredInterface(comm_interface_t interface) {
    preferredInterface = interface;
    
    // If the preferred interface is available, switch to it
    if (isInterfaceAvailable(interface)) {
        activeInterface = interface;
        ESP_LOGI(TAG, "Switched to preferred interface: %d", interface);
    }
}

void CommunicationManager::setAutoFailover(bool enabled) {
    autoFailover = enabled;
    ESP_LOGI(TAG, "Auto failover %s", enabled ? "enabled" : "disabled");
}

comm_stats_t CommunicationManager::getStatistics() {
    comm_stats_t statsCopy;
    
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        statsCopy = stats;
        xSemaphoreGive(mutex);
    } else {
        memset(&statsCopy, 0, sizeof(statsCopy));
    }
    
    return statsCopy;
}

void CommunicationManager::resetStatistics() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        memset(&stats, 0, sizeof(stats));
        xSemaphoreGive(mutex);
        ESP_LOGI(TAG, "Statistics reset");
    }
}

void CommunicationManager::selectBestInterface() {
    comm_interface_t bestInterface = COMM_INTERFACE_NONE;
    
    // Priority order: Preferred -> WiFi -> Cellular -> LoRa
    if (isInterfaceAvailable(preferredInterface)) {
        bestInterface = preferredInterface;
    } else if (isInterfaceAvailable(COMM_INTERFACE_WIFI)) {
        bestInterface = COMM_INTERFACE_WIFI;
    } else if (isInterfaceAvailable(COMM_INTERFACE_CELLULAR)) {
        bestInterface = COMM_INTERFACE_CELLULAR;
    } else if (isInterfaceAvailable(COMM_INTERFACE_LORA)) {
        bestInterface = COMM_INTERFACE_LORA;
    }
    
    if (bestInterface != activeInterface) {
        activeInterface = bestInterface;
        ESP_LOGI(TAG, "Selected interface: %d", activeInterface);
    }
}

bool CommunicationManager::attemptFailover(const uint8_t* data, size_t length) {
    // Try interfaces in priority order, excluding the current active interface
    comm_interface_t interfaces[] = {COMM_INTERFACE_WIFI, COMM_INTERFACE_CELLULAR, COMM_INTERFACE_LORA};
    
    for (int i = 0; i < 3; i++) {
        if (interfaces[i] != activeInterface && isInterfaceAvailable(interfaces[i])) {
            ESP_LOGI(TAG, "Attempting failover to interface %d", interfaces[i]);
            
            if (sendMessage(data, length, interfaces[i])) {
                // Update active interface to the working one
                activeInterface = interfaces[i];
                ESP_LOGI(TAG, "Failover successful to interface %d", activeInterface);
                return true;
            }
        }
    }
    
    ESP_LOGE(TAG, "All failover attempts failed");
    return false;
}

void CommunicationManager::communicationTask(void* parameter) {
    CommunicationManager* manager = static_cast<CommunicationManager*>(parameter);
    
    ESP_LOGI(TAG, "Communication task started");
    
    TickType_t lastInterfaceCheck = 0;
    const TickType_t interfaceCheckInterval = pdMS_TO_TICKS(5000); // Check every 5 seconds
    
    while (true) {
        TickType_t currentTime = xTaskGetTickCount();
        
        // Periodically check interface availability and select best one
        if (currentTime - lastInterfaceCheck >= interfaceCheckInterval) {
            manager->selectBestInterface();
            lastInterfaceCheck = currentTime;
        }
        
        // Handle any pending communication tasks
        // This could include processing queued messages, handling callbacks, etc.
        
        // Sleep for a short time to prevent busy waiting
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}