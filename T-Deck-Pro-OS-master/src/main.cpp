/**
 * @file main.cpp
 * @brief T-Deck-Pro OS Main Application Entry Point
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

// Core OS Components
#include "core/hal/board_config.h"
#include "core/utils/logger.h"
#include "core/display/eink_manager.h"

// LVGL Configuration
#include "lvgl.h"

// Application Framework
#include "core/apps/app_manager.h"
#include "apps/meshtastic_app.h"
#include "apps/file_manager_app.h"
#include "apps/settings_app.h"

// Communication Stack
#include "core/communication/communication_manager.h"
#include "core/communication/lora_manager.h"
#include "core/communication/wifi_manager.h"
#include "core/communication/cellular_manager.h"

// System Services
#include "services/power_manager.h"
#include "services/file_manager.h"
#include "services/ota_manager.h"

// ===== GLOBAL VARIABLES =====
static TaskHandle_t main_task_handle = NULL;
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t comm_task_handle = NULL;

// ===== FUNCTION DECLARATIONS =====
void setup_hardware(void);
void setup_filesystem(void);
void setup_communication(void);
void setup_applications(void);
void main_task(void* parameter);
void ui_task(void* parameter);
void comm_task(void* parameter);

// ===== LVGL TICK CALLBACK =====
static void lv_tick_task(void* arg) {
    (void) arg;
    lv_tick_inc(portTICK_PERIOD_MS);
}

/**
 * @brief Arduino setup function - System initialization
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    Serial.println("\n=== T-Deck-Pro OS Starting ===");
    Serial.printf("Build: %s %s\n", __DATE__, __TIME__);
    Serial.printf("ESP32-S3 Chip: %s\n", ESP.getChipModel());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM Size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));
    
    // Initialize logging system
    log_config_t log_config = {
        .level = LOG_LEVEL_INFO,
        .destinations = LOG_DEST_SERIAL,
        .include_timestamp = true,
        .include_function = true,
        .include_line_number = true,
        .color_output = true,
        .log_file_path = "/logs/system.log",
        .buffer_size = 1024
    };
    
    if (!log_init(&log_config)) {
        Serial.println("WARNING: Failed to initialize logging system");
    }
    
    LOG_INFO("T-Deck-Pro OS initialization started");
    
    // Initialize hardware
    setup_hardware();
    
    // Initialize filesystem
    setup_filesystem();
    
    // Initialize LVGL
    lv_init();
    
    // Initialize E-ink display
    if (!eink_manager.initialize()) {
        LOG_ERROR("Failed to initialize E-ink display");
        while (1) {
            delay(1000);
        }
    }
    
    // Setup LVGL tick
    esp_timer_handle_t lvgl_tick_timer;
    esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, portTICK_PERIOD_MS * 1000);
    
    // Initialize communication systems
    setup_communication();
    
    // Initialize applications
    setup_applications();
    
    // Create main tasks
    xTaskCreatePinnedToCore(
        main_task,
        "main_task",
        8192,
        NULL,
        1,
        &main_task_handle,
        0
    );
    
    xTaskCreatePinnedToCore(
        ui_task,
        "ui_task",
        16384,
        NULL,
        2,
        &ui_task_handle,
        1
    );
    
    xTaskCreatePinnedToCore(
        comm_task,
        "comm_task",
        8192,
        NULL,
        1,
        &comm_task_handle,
        0
    );
    
    // Start E-ink maintenance task
    xTaskCreate(
        eink_maintenance_task,
        "eink_maintenance",
        4096,
        NULL,
        1,
        NULL
    );
    
    LOG_INFO("T-Deck-Pro OS initialization completed");
    Serial.println("=== System Ready ===\n");
}

/**
 * @brief Arduino loop function - Main system loop
 */
void loop() {
    // Main loop is handled by FreeRTOS tasks
    // This loop just handles watchdog and basic housekeeping
    
    static uint32_t last_heartbeat = 0;
    uint32_t current_time = millis();
    
    if (current_time - last_heartbeat >= 30000) { // 30 second heartbeat
        LOG_DEBUG("System heartbeat - Free heap: %d bytes", ESP.getFreeHeap());
        last_heartbeat = current_time;
    }
    
    // Feed watchdog
    esp_task_wdt_reset();
    
    // Small delay to prevent tight loop
    delay(100);
}

/**
 * @brief Initialize hardware components
 */
void setup_hardware() {
    LOG_INFO("Initializing hardware components");
    
    // Initialize board configuration
    if (!board_init()) {
        LOG_ERROR("Failed to initialize board hardware");
        while (1) delay(1000);
    }
    
    // Initialize power management
    if (!board_set_power_state(BOARD_POWER_ACTIVE)) {
        LOG_WARN("Failed to set initial power state");
    }
    
    // Check battery status
    uint16_t battery_mv = board_get_battery_voltage();
    bool usb_connected = board_is_usb_connected();
    
    LOG_INFO("Battery: %d mV, USB: %s", battery_mv, usb_connected ? "Connected" : "Disconnected");
    
    if (battery_mv < BOARD_BAT_CRIT_MV && !usb_connected) {
        LOG_ERROR("Critical battery level, entering deep sleep");
        esp_deep_sleep_start();
    }
    
    LOG_INFO("Hardware initialization completed");
}

/**
 * @brief Initialize filesystem
 */
void setup_filesystem() {
    LOG_INFO("Initializing filesystem");
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        LOG_ERROR("Failed to initialize SPIFFS");
        return;
    }
    
    // Create necessary directories
    if (!SPIFFS.exists("/apps")) {
        SPIFFS.mkdir("/apps");
    }
    if (!SPIFFS.exists("/logs")) {
        SPIFFS.mkdir("/logs");
    }
    if (!SPIFFS.exists("/config")) {
        SPIFFS.mkdir("/config");
    }
    if (!SPIFFS.exists("/data")) {
        SPIFFS.mkdir("/data");
    }
    
    // Log filesystem info
    size_t total_bytes = SPIFFS.totalBytes();
    size_t used_bytes = SPIFFS.usedBytes();
    LOG_INFO("SPIFFS: %d/%d bytes used (%.1f%%)", 
             used_bytes, total_bytes, (float)used_bytes / total_bytes * 100.0);
    
    LOG_INFO("Filesystem initialization completed");
}

/**
 * @brief Initialize communication systems
 */
void setup_communication() {
    LOG_INFO("Initializing communication systems");
    
    // Initialize communication manager (handles all interfaces)
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    if (!commMgr->initialize()) {
        LOG_ERROR("Failed to initialize communication manager");
        return;
    }
    
    // Set preferred interface to WiFi
    commMgr->setPreferredInterface(COMM_INTERFACE_WIFI);
    
    // Enable auto failover
    commMgr->setAutoFailover(true);
    
    LOG_INFO("Communication systems initialized successfully");
}

/**
 * @brief Initialize applications
 */
void setup_applications() {
    LOG_INFO("Initializing applications");
    
    // Initialize application manager
    AppManager& appManager = AppManager::getInstance();
    appManager.initialize();
    
    // Register core applications
    REGISTER_APP(MeshtasticApp, "meshtastic", true);
    REGISTER_APP(FileManagerApp, "file_manager", false);
    REGISTER_APP(SettingsApp, "settings", false);
    
    // Auto-start applications
    appManager.autoStartApps();
    
    LOG_INFO("Applications initialized - %d apps registered",
             appManager.getRegisteredApps().size());
}

/**
 * @brief Main system task
 */
void main_task(void* parameter) {
    LOG_INFO("Main task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // 1 second
    AppManager& appManager = AppManager::getInstance();
    
    while (1) {
        // Update application manager
        appManager.update();
        
        // System monitoring and maintenance
        
        // Check memory usage
        size_t free_heap = ESP.getFreeHeap();
        size_t min_free_heap = ESP.getMinFreeHeap();
        
        if (free_heap < 50000) { // Less than 50KB free
            LOG_WARN("Low memory warning: %d bytes free", free_heap);
            appManager.handleMemoryWarning();
        }
        
        // Check power status
        uint16_t battery_mv = board_get_battery_voltage();
        if (battery_mv < BOARD_BAT_LOW_MV) {
            LOG_WARN("Low battery: %d mV", battery_mv);
            // TODO: Trigger power saving mode
        }
        
        // Periodic tasks
        static uint32_t task_counter = 0;
        task_counter++;
        
        if (task_counter % 60 == 0) { // Every minute
            log_flush();
            
            // Log system statistics
            AppManager::SystemStats stats = appManager.getSystemStats();
            LOG_INFO("System Stats - Apps: %d/%d, Memory: %d KB, Uptime: %d min",
                     stats.runningApps, stats.totalApps,
                     stats.totalMemoryUsed / 1024,
                     stats.uptime / 60000);
        }
        
        if (task_counter % 300 == 0) { // Every 5 minutes
            // Save application configurations
            appManager.saveSystemConfig();
            // TODO: Sync with server
            // TODO: Check for OTA updates
        }
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief UI task - handles LVGL and display updates
 */
void ui_task(void* parameter) {
    LOG_INFO("UI task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(10); // 10ms for smooth UI
    
    while (1) {
        // Handle LVGL tasks
        lv_timer_handler();
        
        // Handle E-ink display updates
        eink_task_handler();
        
        vTaskDelay(xDelay);
    }
}

/**
 * @brief Communication task - handles all communication protocols
 */
void comm_task(void* parameter) {
    LOG_INFO("Communication task started");
    
    const TickType_t xDelay = pdMS_TO_TICKS(1000); // 1 second
    CommunicationManager* commMgr = CommunicationManager::getInstance();
    
    // Buffer for receiving messages
    uint8_t rxBuffer[256];
    size_t receivedLength;
    comm_interface_t sourceInterface;
    
    // Test message counter
    uint32_t messageCounter = 0;
    
    while (1) {
        // Check for incoming messages
        if (commMgr->receiveMessage(rxBuffer, sizeof(rxBuffer), &receivedLength, &sourceInterface)) {
            LOG_INFO("Received message (%d bytes) from interface %d", receivedLength, sourceInterface);
            // TODO: Process received message
        }
        
        // Send periodic test message every 30 seconds
        static uint32_t lastTestMessage = 0;
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (currentTime - lastTestMessage >= 30000) {
            char testMessage[64];
            snprintf(testMessage, sizeof(testMessage), "Test message #%lu from T-Deck-Pro", messageCounter++);
            
            if (commMgr->sendMessage((uint8_t*)testMessage, strlen(testMessage), COMM_INTERFACE_AUTO)) {
                LOG_INFO("Sent test message: %s", testMessage);
            } else {
                LOG_WARN("Failed to send test message");
            }
            
            lastTestMessage = currentTime;
        }
        
        // Log communication statistics every 60 seconds
        static uint32_t lastStatsLog = 0;
        if (currentTime - lastStatsLog >= 60000) {
            comm_stats_t stats = commMgr->getStatistics();
            LOG_INFO("Communication Stats - LoRa: %lu/%lu msgs, WiFi: %lu/%lu msgs, Cellular: %lu/%lu msgs",
                     stats.lora.messagesSent, stats.lora.messagesReceived,
                     stats.wifi.messagesSent, stats.wifi.messagesReceived,
                     stats.cellular.messagesSent, stats.cellular.messagesReceived);
            lastStatsLog = currentTime;
        }
        
        vTaskDelay(xDelay);
    }
}