#include "app_manager.h"
#include "../utils/logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

AppManager& AppManager::getInstance() {
    static AppManager instance;
    return instance;
}

AppManager::~AppManager() {
    shutdown();
    if (managerMutex) {
        vSemaphoreDelete(managerMutex);
    }
}

void AppManager::initialize() {
    if (initialized) {
        Logger::warning("AppManager", "Already initialized");
        return;
    }

    managerMutex = xSemaphoreCreateMutex();
    if (!managerMutex) {
        Logger::error("AppManager", "Failed to create manager mutex");
        return;
    }

    activeAppId = "";
    appSwitcherContainer = nullptr;
    appSwitcherVisible = false;
    lastMemoryCheck = 0;
    lastUpdate = 0;

    // Load system configuration
    loadSystemConfig();

    // Create app switcher UI
    createAppSwitcherUI();

    initialized = true;
    Logger::info("AppManager", "Application manager initialized");
}

bool AppManager::registerApp(const String& appId, AppFactory* factory, 
                           bool autoStart, const std::vector<String>& dependencies) {
    if (!factory) {
        Logger::error("AppManager", "Cannot register app with null factory: " + appId);
        return false;
    }

    if (!managerMutex) {
        Logger::error("AppManager", "Manager not initialized");
        return false;
    }

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    // Check if already registered
    if (registeredApps.find(appId) != registeredApps.end()) {
        Logger::warning("AppManager", "App already registered: " + appId);
        xSemaphoreGive(managerMutex);
        return false;
    }

    // Check for circular dependencies
    std::vector<String> visited;
    if (hasCircularDependency(appId, visited)) {
        Logger::error("AppManager", "Circular dependency detected for app: " + appId);
        xSemaphoreGive(managerMutex);
        return false;
    }

    // Register the app
    AppRegistration registration;
    registration.appId = appId;
    registration.factory = factory;
    registration.autoStart = autoStart;
    registration.dependencies = dependencies;
    registration.registrationTime = millis();

    registeredApps[appId] = registration;

    Logger::info("AppManager", "Registered app: " + appId + 
                (autoStart ? " (auto-start)" : ""));

    xSemaphoreGive(managerMutex);
    return true;
}

bool AppManager::unregisterApp(const String& appId) {
    if (!managerMutex) return false;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    // Stop app if running
    if (runningApps.find(appId) != runningApps.end()) {
        stopApp(appId);
    }

    // Remove registration
    auto it = registeredApps.find(appId);
    if (it != registeredApps.end()) {
        delete it->second.factory;
        registeredApps.erase(it);
        Logger::info("AppManager", "Unregistered app: " + appId);
        xSemaphoreGive(managerMutex);
        return true;
    }

    xSemaphoreGive(managerMutex);
    return false;
}

AppManager::LaunchResult AppManager::launchApp(const String& appId) {
    if (!managerMutex) return LaunchResult::LAUNCH_FAILED;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    // Check if app is registered
    auto regIt = registeredApps.find(appId);
    if (regIt == registeredApps.end()) {
        Logger::error("AppManager", "App not registered: " + appId);
        xSemaphoreGive(managerMutex);
        return LaunchResult::APP_NOT_FOUND;
    }

    // Check if already running
    if (runningApps.find(appId) != runningApps.end()) {
        Logger::warning("AppManager", "App already running: " + appId);
        xSemaphoreGive(managerMutex);
        return LaunchResult::APP_ALREADY_RUNNING;
    }

    // Check dependencies
    if (!checkDependencies(appId)) {
        Logger::error("AppManager", "Dependencies not met for app: " + appId);
        xSemaphoreGive(managerMutex);
        return LaunchResult::DEPENDENCY_MISSING;
    }

    // Check if we can launch (memory, app limits)
    if (!canLaunchApp(appId)) {
        Logger::error("AppManager", "Cannot launch app due to system limits: " + appId);
        xSemaphoreGive(managerMutex);
        return LaunchResult::INSUFFICIENT_MEMORY;
    }

    // Create app instance
    AppBase* app = regIt->second.factory->createApp();
    if (!app) {
        Logger::error("AppManager", "Failed to create app instance: " + appId);
        xSemaphoreGive(managerMutex);
        return LaunchResult::LAUNCH_FAILED;
    }

    // Set state change callback
    app->setStateChangeCallback([this](AppBase* app, AppBase::AppState oldState, AppBase::AppState newState) {
        onAppStateChange(app, oldState, newState);
    });

    // Initialize and start app
    app->setState(AppBase::AppState::STARTING);
    
    if (!app->initialize()) {
        Logger::error("AppManager", "Failed to initialize app: " + appId);
        delete app;
        xSemaphoreGive(managerMutex);
        return LaunchResult::LAUNCH_FAILED;
    }

    if (!app->start()) {
        Logger::error("AppManager", "Failed to start app: " + appId);
        app->cleanup();
        delete app;
        xSemaphoreGive(managerMutex);
        return LaunchResult::LAUNCH_FAILED;
    }

    // Add to running apps
    runningApps[appId] = app;
    app->setState(AppBase::AppState::RUNNING);

    Logger::info("AppManager", "Successfully launched app: " + appId);

    // Set as active if no active app
    if (activeAppId.isEmpty()) {
        setActiveApp(appId);
    }

    xSemaphoreGive(managerMutex);
    return LaunchResult::SUCCESS;
}

bool AppManager::stopApp(const String& appId) {
    if (!managerMutex) return false;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    auto it = runningApps.find(appId);
    if (it == runningApps.end()) {
        Logger::warning("AppManager", "App not running: " + appId);
        xSemaphoreGive(managerMutex);
        return false;
    }

    AppBase* app = it->second;
    app->setState(AppBase::AppState::STOPPING);

    // Stop the app
    app->stop();
    app->cleanup();
    app->setState(AppBase::AppState::STOPPED);

    // Remove from running apps
    runningApps.erase(it);

    // Clean up app instance
    auto regIt = registeredApps.find(appId);
    if (regIt != registeredApps.end()) {
        regIt->second.factory->destroyApp(app);
    }

    Logger::info("AppManager", "Stopped app: " + appId);

    // Switch active app if this was active
    if (activeAppId == appId) {
        activeAppId = "";
        if (!runningApps.empty()) {
            setActiveApp(runningApps.begin()->first);
        }
    }

    xSemaphoreGive(managerMutex);
    return true;
}

void AppManager::update() {
    if (!initialized || !managerMutex) return;

    uint32_t now = millis();

    // Throttle updates
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    lastUpdate = now;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    // Update all running apps
    for (auto& pair : runningApps) {
        if (pair.second->isRunning()) {
            pair.second->updateUI();
        }
    }

    // Periodic memory check
    if (now - lastMemoryCheck > MEMORY_CHECK_INTERVAL) {
        checkMemoryLimits();
        lastMemoryCheck = now;
    }

    // Clean up stopped apps
    cleanupStoppedApps();

    xSemaphoreGive(managerMutex);
}

void AppManager::handleKeyPress(uint8_t key) {
    if (!managerMutex) return;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    // Send to active app first
    if (!activeAppId.isEmpty()) {
        auto it = runningApps.find(activeAppId);
        if (it != runningApps.end()) {
            it->second->onKeyPress(key);
        }
    }

    xSemaphoreGive(managerMutex);
}

void AppManager::setActiveApp(const String& appId) {
    if (!managerMutex) return;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    if (runningApps.find(appId) == runningApps.end()) {
        Logger::warning("AppManager", "Cannot set non-running app as active: " + appId);
        xSemaphoreGive(managerMutex);
        return;
    }

    activeAppId = appId;
    Logger::info("AppManager", "Active app set to: " + appId);

    xSemaphoreGive(managerMutex);
}

AppManager::SystemStats AppManager::getSystemStats() const {
    SystemStats stats = {};
    
    if (!managerMutex) return stats;

    xSemaphoreTake(managerMutex, portMAX_DELAY);

    stats.totalMemoryUsed = getTotalMemoryUsage();
    stats.availableMemory = MAX_TOTAL_MEMORY - stats.totalMemoryUsed;
    stats.runningApps = runningApps.size();
    stats.totalApps = registeredApps.size();
    stats.uptime = millis();
    stats.cpuUsage = 0.0f; // TODO: Implement CPU usage calculation

    xSemaphoreGive(managerMutex);
    return stats;
}

bool AppManager::checkDependencies(const String& appId) const {
    auto it = registeredApps.find(appId);
    if (it == registeredApps.end()) return false;

    for (const String& dep : it->second.dependencies) {
        if (runningApps.find(dep) == runningApps.end()) {
            return false;
        }
    }
    return true;
}

bool AppManager::canLaunchApp(const String& appId) const {
    // Check app count limit
    if (runningApps.size() >= MAX_RUNNING_APPS) {
        return false;
    }

    // Check memory limit (rough estimate)
    if (getTotalMemoryUsage() > MAX_TOTAL_MEMORY * 0.8) {
        return false;
    }

    return true;
}

size_t AppManager::getTotalMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : runningApps) {
        total += pair.second->getCurrentMemoryUsage();
    }
    return total;
}

void AppManager::onAppStateChange(AppBase* app, AppBase::AppState oldState, AppBase::AppState newState) {
    if (appStateChangeCallback) {
        String appId;
        // Find app ID
        for (const auto& pair : runningApps) {
            if (pair.second == app) {
                appId = pair.first;
                break;
            }
        }
        appStateChangeCallback(appId, oldState, newState);
    }
}

void AppManager::cleanupStoppedApps() {
    // Remove apps that are in STOPPED state
    auto it = runningApps.begin();
    while (it != runningApps.end()) {
        if (it->second->getState() == AppBase::AppState::STOPPED) {
            Logger::info("AppManager", "Cleaning up stopped app: " + it->first);
            
            auto regIt = registeredApps.find(it->first);
            if (regIt != registeredApps.end()) {
                regIt->second.factory->destroyApp(it->second);
            }
            
            it = runningApps.erase(it);
        } else {
            ++it;
        }
    }
}

void AppManager::createAppSwitcherUI() {
    // TODO: Implement app switcher UI
    // This will be a simple overlay showing running apps
}

bool AppManager::hasCircularDependency(const String& appId, std::vector<String>& visited) const {
    // Check for circular dependencies in app registration
    if (std::find(visited.begin(), visited.end(), appId) != visited.end()) {
        return true;
    }

    visited.push_back(appId);

    auto it = registeredApps.find(appId);
    if (it != registeredApps.end()) {
        for (const String& dep : it->second.dependencies) {
            if (hasCircularDependency(dep, visited)) {
                return true;
            }
        }
    }

    visited.pop_back();
    return false;
}

void AppManager::shutdown() {
    if (!initialized) return;

    Logger::info("AppManager", "Shutting down application manager");

    if (managerMutex) {
        xSemaphoreTake(managerMutex, portMAX_DELAY);

        // Stop all running apps
        for (auto& pair : runningApps) {
            pair.second->stop();
            pair.second->cleanup();
            
            auto regIt = registeredApps.find(pair.first);
            if (regIt != registeredApps.end()) {
                regIt->second.factory->destroyApp(pair.second);
            }
        }
        runningApps.clear();

        // Clean up registered apps
        for (auto& pair : registeredApps) {
            delete pair.second.factory;
        }
        registeredApps.clear();

        xSemaphoreGive(managerMutex);
    }

    initialized = false;
}

bool AppManager::saveSystemConfig() {
    // TODO: Implement system configuration saving
    return true;
}

bool AppManager::loadSystemConfig() {
    // TODO: Implement system configuration loading
    return true;
}