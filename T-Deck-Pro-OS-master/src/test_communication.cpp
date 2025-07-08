/**
 * @file test_communication.cpp
 * @brief Communication stack test application
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>
#include "core/communication/communication_manager.h"
#include "core/utils/logger.h"
#include "core/hal/board_config.h"

static const char* TAG = "CommTest";

/**
 * @brief Test communication interfaces
 */
void test_communication_interfaces() {
    ESP_LOGI(TAG, "Starting communication interface tests");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    
    // Test interface availability
    ESP_LOGI(TAG, "Interface availability:");
    ESP_LOGI(TAG, "  LoRa: %s", commMgr->isInterfaceAvailable(COMM_INTERFACE_LORA) ? "Available" : "Not Available");
    ESP_LOGI(TAG, "  WiFi: %s", commMgr->isInterfaceAvailable(COMM_INTERFACE_WIFI) ? "Available" : "Not Available");
    ESP_LOGI(TAG, "  Cellular: %s", commMgr->isInterfaceAvailable(COMM_INTERFACE_CELLULAR) ? "Available" : "Not Available");
    
    // Test active interface
    comm_interface_t activeInterface = commMgr->getActiveInterface();
    const char* interfaceName = "Unknown";
    switch (activeInterface) {
        case COMM_INTERFACE_LORA: interfaceName = "LoRa"; break;
        case COMM_INTERFACE_WIFI: interfaceName = "WiFi"; break;
        case COMM_INTERFACE_CELLULAR: interfaceName = "Cellular"; break;
        case COMM_INTERFACE_NONE: interfaceName = "None"; break;
        default: break;
    }
    ESP_LOGI(TAG, "Active interface: %s", interfaceName);
}

/**
 * @brief Test message sending
 */
void test_message_sending() {
    ESP_LOGI(TAG, "Testing message sending");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    
    // Test messages
    const char* testMessages[] = {
        "Hello from T-Deck-Pro!",
        "Testing LoRa communication",
        "Multi-interface test message",
        "Communication stack validation"
    };
    
    for (int i = 0; i < 4; i++) {
        const char* message = testMessages[i];
        size_t messageLen = strlen(message);
        
        ESP_LOGI(TAG, "Sending message %d: %s", i + 1, message);
        
        // Try sending on auto interface
        if (commMgr->sendMessage((uint8_t*)message, messageLen, COMM_INTERFACE_AUTO)) {
            ESP_LOGI(TAG, "Message %d sent successfully", i + 1);
        } else {
            ESP_LOGE(TAG, "Failed to send message %d", i + 1);
        }
        
        delay(2000); // Wait 2 seconds between messages
    }
}

/**
 * @brief Test message receiving
 */
void test_message_receiving() {
    ESP_LOGI(TAG, "Testing message receiving (10 second window)");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    uint8_t rxBuffer[256];
    size_t receivedLength;
    comm_interface_t sourceInterface;
    
    uint32_t startTime = millis();
    uint32_t messageCount = 0;
    
    while (millis() - startTime < 10000) { // 10 second test window
        if (commMgr->receiveMessage(rxBuffer, sizeof(rxBuffer), &receivedLength, &sourceInterface)) {
            messageCount++;
            rxBuffer[receivedLength] = '\0'; // Null terminate for printing
            
            const char* interfaceName = "Unknown";
            switch (sourceInterface) {
                case COMM_INTERFACE_LORA: interfaceName = "LoRa"; break;
                case COMM_INTERFACE_WIFI: interfaceName = "WiFi"; break;
                case COMM_INTERFACE_CELLULAR: interfaceName = "Cellular"; break;
                default: break;
            }
            
            ESP_LOGI(TAG, "Received message #%lu from %s (%d bytes): %s", 
                     messageCount, interfaceName, receivedLength, (char*)rxBuffer);
        }
        
        delay(100); // Check every 100ms
    }
    
    ESP_LOGI(TAG, "Received %lu messages in 10 seconds", messageCount);
}

/**
 * @brief Test interface switching
 */
void test_interface_switching() {
    ESP_LOGI(TAG, "Testing interface switching");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    
    // Test switching to different interfaces
    comm_interface_t interfaces[] = {COMM_INTERFACE_LORA, COMM_INTERFACE_WIFI, COMM_INTERFACE_CELLULAR};
    const char* interfaceNames[] = {"LoRa", "WiFi", "Cellular"};
    
    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "Setting preferred interface to %s", interfaceNames[i]);
        commMgr->setPreferredInterface(interfaces[i]);
        
        delay(1000);
        
        comm_interface_t activeInterface = commMgr->getActiveInterface();
        if (activeInterface == interfaces[i]) {
            ESP_LOGI(TAG, "Successfully switched to %s", interfaceNames[i]);
        } else {
            ESP_LOGW(TAG, "Failed to switch to %s (interface not available)", interfaceNames[i]);
        }
    }
    
    // Reset to auto selection
    ESP_LOGI(TAG, "Resetting to automatic interface selection");
    commMgr->setPreferredInterface(COMM_INTERFACE_WIFI); // Default preference
}

/**
 * @brief Test communication statistics
 */
void test_communication_statistics() {
    ESP_LOGI(TAG, "Testing communication statistics");
    
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    comm_stats_t stats = commMgr->getStatistics();
    
    ESP_LOGI(TAG, "Communication Statistics:");
    ESP_LOGI(TAG, "LoRa Interface:");
    ESP_LOGI(TAG, "  Messages Sent: %lu", stats.lora.messagesSent);
    ESP_LOGI(TAG, "  Messages Received: %lu", stats.lora.messagesReceived);
    ESP_LOGI(TAG, "  Bytes Sent: %lu", stats.lora.bytesSent);
    ESP_LOGI(TAG, "  Bytes Received: %lu", stats.lora.bytesReceived);
    ESP_LOGI(TAG, "  Send Errors: %lu", stats.lora.sendErrors);
    ESP_LOGI(TAG, "  Receive Errors: %lu", stats.lora.receiveErrors);
    
    ESP_LOGI(TAG, "WiFi Interface:");
    ESP_LOGI(TAG, "  Messages Sent: %lu", stats.wifi.messagesSent);
    ESP_LOGI(TAG, "  Messages Received: %lu", stats.wifi.messagesReceived);
    ESP_LOGI(TAG, "  Bytes Sent: %lu", stats.wifi.bytesSent);
    ESP_LOGI(TAG, "  Bytes Received: %lu", stats.wifi.bytesReceived);
    ESP_LOGI(TAG, "  Send Errors: %lu", stats.wifi.sendErrors);
    ESP_LOGI(TAG, "  Receive Errors: %lu", stats.wifi.receiveErrors);
    
    ESP_LOGI(TAG, "Cellular Interface:");
    ESP_LOGI(TAG, "  Messages Sent: %lu", stats.cellular.messagesSent);
    ESP_LOGI(TAG, "  Messages Received: %lu", stats.cellular.messagesReceived);
    ESP_LOGI(TAG, "  Bytes Sent: %lu", stats.cellular.bytesSent);
    ESP_LOGI(TAG, "  Bytes Received: %lu", stats.cellular.bytesReceived);
    ESP_LOGI(TAG, "  Send Errors: %lu", stats.cellular.sendErrors);
    ESP_LOGI(TAG, "  Receive Errors: %lu", stats.cellular.receiveErrors);
}

/**
 * @brief Run comprehensive communication tests
 */
void run_communication_tests() {
    ESP_LOGI(TAG, "=== Starting Communication Stack Tests ===");
    
    // Initialize communication manager
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    if (!commMgr->initialize()) {
        ESP_LOGE(TAG, "Failed to initialize communication manager");
        return;
    }
    
    delay(2000); // Allow initialization to complete
    
    // Run tests
    test_communication_interfaces();
    delay(1000);
    
    test_interface_switching();
    delay(1000);
    
    test_message_sending();
    delay(1000);
    
    test_message_receiving();
    delay(1000);
    
    test_communication_statistics();
    
    ESP_LOGI(TAG, "=== Communication Stack Tests Completed ===");
}