#include "app_base.h"
#include "../utils/logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

AppBase::AppBase(const AppInfo& info) 
    : appInfo(info)
    , currentState(AppState::STOPPED)
    , previousState(AppState::STOPPED)
    , startTime(0)
    , pauseTime(0)
    , mainContainer(nullptr)
    , totalAllocatedMemory(0)
{
    memoryMutex = xSemaphoreCreateMutex();
    if (!memoryMutex) {
        Logger::error("AppBase", "Failed to create memory mutex for app: " + appInfo.name);
    }
}

AppBase::~AppBase() {
    cleanup();
    destroyUI();
    
    // Free all allocated memory
    if (memoryMutex) {
        xSemaphoreTake(memoryMutex, portMAX_DELAY);
        for (auto& block : allocatedMemory) {
            free(block.ptr);
        }
        allocatedMemory.clear();
        totalAllocatedMemory = 0;
        xSemaphoreGive(memoryMutex);
        vSemaphoreDelete(memoryMutex);
    }
}

bool AppBase::setState(AppState newState) {
    if (!validateStateTransition(currentState, newState)) {
        Logger::error("AppBase", "Invalid state transition for " + appInfo.name + 
                     " from " + String((int)currentState) + " to " + String((int)newState));
        return false;
    }

    AppState oldState = currentState;
    previousState = currentState;
    currentState = newState;

    logStateChange(oldState, newState);

    // Update timing
    if (newState == AppState::RUNNING && oldState != AppState::RESUMING) {
        startTime = millis();
    } else if (newState == AppState::PAUSED) {
        pauseTime = millis();
    }

    // Notify callback
    if (stateChangeCallback) {
        stateChangeCallback(this, oldState, newState);
    }

    return true;
}

void AppBase::setStateChangeCallback(std::function<void(AppBase*, AppState, AppState)> callback) {
    stateChangeCallback = callback;
}

void* AppBase::allocateMemory(size_t size) {
    if (!memoryMutex) return nullptr;

    xSemaphoreTake(memoryMutex, portMAX_DELAY);

    // Check memory limit
    if (totalAllocatedMemory + size > MAX_MEMORY_PER_APP) {
        Logger::warning("AppBase", "Memory limit exceeded for app: " + appInfo.name);
        xSemaphoreGive(memoryMutex);
        return nullptr;
    }

    void* ptr = malloc(size);
    if (ptr) {
        MemoryBlock block = {ptr, size, millis()};
        allocatedMemory.push_back(block);
        totalAllocatedMemory += size;
        
        Logger::debug("AppBase", "Allocated " + String(size) + " bytes for " + appInfo.name + 
                     " (total: " + String(totalAllocatedMemory) + ")");
    }

    xSemaphoreGive(memoryMutex);
    return ptr;
}

void AppBase::freeMemory(void* ptr) {
    if (!ptr || !memoryMutex) return;

    xSemaphoreTake(memoryMutex, portMAX_DELAY);

    for (auto it = allocatedMemory.begin(); it != allocatedMemory.end(); ++it) {
        if (it->ptr == ptr) {
            totalAllocatedMemory -= it->size;
            allocatedMemory.erase(it);
            free(ptr);
            Logger::debug("AppBase", "Freed memory for " + appInfo.name + 
                         " (total: " + String(totalAllocatedMemory) + ")");
            break;
        }
    }

    xSemaphoreGive(memoryMutex);
}

bool AppBase::checkMemoryLimit() {
    if (!memoryMutex) return false;

    xSemaphoreTake(memoryMutex, portMAX_DELAY);
    bool withinLimit = totalAllocatedMemory < MAX_MEMORY_PER_APP;
    xSemaphoreGive(memoryMutex);

    return withinLimit;
}

size_t AppBase::getCurrentMemoryUsage() const {
    if (!memoryMutex) return 0;

    xSemaphoreTake(memoryMutex, portMAX_DELAY);
    size_t usage = totalAllocatedMemory;
    xSemaphoreGive(memoryMutex);

    return usage;
}

void AppBase::destroyUI() {
    if (mainContainer) {
        lv_obj_del(mainContainer);
        mainContainer = nullptr;
    }
}

bool AppBase::saveConfig() {
    String configPath = getConfigPath();
    if (configPath.isEmpty()) {
        Logger::error("AppBase", "Invalid config path for app: " + appInfo.name);
        return false;
    }

    if (!createConfigDirectory()) {
        Logger::error("AppBase", "Failed to create config directory for app: " + appInfo.name);
        return false;
    }

    // Create basic config JSON
    DynamicJsonDocument doc(1024);
    doc["app_name"] = appInfo.name;
    doc["app_version"] = appInfo.version;
    doc["last_run"] = millis();
    doc["total_runtime"] = getRunTime();
    doc["memory_usage"] = getCurrentMemoryUsage();

    File configFile = SPIFFS.open(configPath, "w");
    if (!configFile) {
        Logger::error("AppBase", "Failed to open config file for writing: " + configPath);
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        Logger::error("AppBase", "Failed to write config for app: " + appInfo.name);
        configFile.close();
        return false;
    }

    configFile.close();
    Logger::info("AppBase", "Config saved for app: " + appInfo.name);
    return true;
}

bool AppBase::loadConfig() {
    String configPath = getConfigPath();
    if (configPath.isEmpty()) {
        Logger::warning("AppBase", "No config path for app: " + appInfo.name);
        return false;
    }

    if (!SPIFFS.exists(configPath)) {
        Logger::info("AppBase", "No existing config for app: " + appInfo.name);
        return true; // Not an error - first run
    }

    File configFile = SPIFFS.open(configPath, "r");
    if (!configFile) {
        Logger::error("AppBase", "Failed to open config file: " + configPath);
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Logger::error("AppBase", "Failed to parse config for app: " + appInfo.name + 
                     " - " + String(error.c_str()));
        return false;
    }

    // Validate config
    if (doc["app_name"] != appInfo.name) {
        Logger::warning("AppBase", "Config app name mismatch for: " + appInfo.name);
        return false;
    }

    Logger::info("AppBase", "Config loaded for app: " + appInfo.name);
    return true;
}

void AppBase::resetConfig() {
    String configPath = getConfigPath();
    if (!configPath.isEmpty() && SPIFFS.exists(configPath)) {
        SPIFFS.remove(configPath);
        Logger::info("AppBase", "Config reset for app: " + appInfo.name);
    }
}

void AppBase::logStateChange(AppState from, AppState to) {
    String fromStr, toStr;
    
    switch (from) {
        case AppState::STOPPED: fromStr = "STOPPED"; break;
        case AppState::STARTING: fromStr = "STARTING"; break;
        case AppState::RUNNING: fromStr = "RUNNING"; break;
        case AppState::PAUSING: fromStr = "PAUSING"; break;
        case AppState::PAUSED: fromStr = "PAUSED"; break;
        case AppState::RESUMING: fromStr = "RESUMING"; break;
        case AppState::STOPPING: fromStr = "STOPPING"; break;
    }
    
    switch (to) {
        case AppState::STOPPED: toStr = "STOPPED"; break;
        case AppState::STARTING: toStr = "STARTING"; break;
        case AppState::RUNNING: toStr = "RUNNING"; break;
        case AppState::PAUSING: toStr = "PAUSING"; break;
        case AppState::PAUSED: toStr = "PAUSED"; break;
        case AppState::RESUMING: toStr = "RESUMING"; break;
        case AppState::STOPPING: toStr = "STOPPING"; break;
    }

    Logger::info("AppBase", appInfo.name + " state: " + fromStr + " -> " + toStr);
}

bool AppBase::validateStateTransition(AppState from, AppState to) {
    // Define valid state transitions
    switch (from) {
        case AppState::STOPPED:
            return to == AppState::STARTING;
        case AppState::STARTING:
            return to == AppState::RUNNING || to == AppState::STOPPING;
        case AppState::RUNNING:
            return to == AppState::PAUSING || to == AppState::STOPPING;
        case AppState::PAUSING:
            return to == AppState::PAUSED || to == AppState::STOPPING;
        case AppState::PAUSED:
            return to == AppState::RESUMING || to == AppState::STOPPING;
        case AppState::RESUMING:
            return to == AppState::RUNNING || to == AppState::STOPPING;
        case AppState::STOPPING:
            return to == AppState::STOPPED;
    }
    return false;
}

void AppBase::updateMemoryUsage() {
    appInfo.memoryUsage = getCurrentMemoryUsage();
}

String AppBase::getConfigPath() const {
    return "/config/apps/" + appInfo.name + ".json";
}

bool AppBase::createConfigDirectory() {
    // SPIFFS doesn't have directories, but we can simulate them
    return true;
}