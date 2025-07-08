#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "app_base.h"
#include <map>
#include <vector>
#include <functional>

/**
 * @brief Central application manager for the T-Deck-Pro OS
 * 
 * Manages application lifecycle, memory allocation, and inter-app communication.
 * Implements singleton pattern for global access.
 */
class AppManager {
public:
    enum class LaunchResult {
        SUCCESS,
        APP_NOT_FOUND,
        APP_ALREADY_RUNNING,
        INSUFFICIENT_MEMORY,
        DEPENDENCY_MISSING,
        LAUNCH_FAILED
    };

    struct AppRegistration {
        String appId;
        AppFactory* factory;
        bool autoStart;
        std::vector<String> dependencies;
        uint32_t registrationTime;
    };

    struct SystemStats {
        size_t totalMemoryUsed;
        size_t availableMemory;
        uint8_t runningApps;
        uint8_t totalApps;
        uint32_t uptime;
        float cpuUsage;
    };

    // Singleton access
    static AppManager& getInstance();

    // Application registration
    bool registerApp(const String& appId, AppFactory* factory, 
                    bool autoStart = false, 
                    const std::vector<String>& dependencies = {});
    bool unregisterApp(const String& appId);
    bool isAppRegistered(const String& appId) const;

    // Application lifecycle
    LaunchResult launchApp(const String& appId);
    bool pauseApp(const String& appId);
    bool resumeApp(const String& appId);
    bool stopApp(const String& appId);
    bool restartApp(const String& appId);

    // Application queries
    AppBase* getApp(const String& appId) const;
    std::vector<String> getRunningApps() const;
    std::vector<String> getRegisteredApps() const;
    AppBase::AppInfo getAppInfo(const String& appId) const;
    bool isAppRunning(const String& appId) const;

    // System management
    void initialize();
    void update(); // Call from main loop
    void shutdown();
    void autoStartApps();
    
    // Memory management
    SystemStats getSystemStats() const;
    bool checkMemoryLimits();
    void forceGarbageCollection();
    size_t getTotalMemoryUsage() const;

    // Event handling
    void handleKeyPress(uint8_t key);
    void handleTouch(lv_event_t* e);
    void handleNetworkChange(bool connected);
    void handleBatteryChange(uint8_t percentage);
    void handleMemoryWarning();

    // UI management
    void setActiveApp(const String& appId);
    String getActiveApp() const;
    lv_obj_t* getActiveAppContainer() const;
    void showAppSwitcher();
    void hideAppSwitcher();

    // Inter-app communication
    bool sendMessage(const String& fromApp, const String& toApp, 
                    const String& message, const String& data = "");
    void setMessageHandler(const String& appId, 
                          std::function<void(const String&, const String&, const String&)> handler);

    // Configuration
    bool saveSystemConfig();
    bool loadSystemConfig();
    void resetSystemConfig();

    // Callbacks
    void setAppStateChangeCallback(std::function<void(const String&, AppBase::AppState, AppBase::AppState)> callback);
    void setMemoryWarningCallback(std::function<void(size_t, size_t)> callback);

private:
    AppManager() = default;
    ~AppManager();

    // Prevent copying
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;

    // Internal data
    std::map<String, AppRegistration> registeredApps;
    std::map<String, AppBase*> runningApps;
    std::map<String, std::function<void(const String&, const String&, const String&)>> messageHandlers;
    
    String activeAppId;
    lv_obj_t* appSwitcherContainer;
    bool appSwitcherVisible;
    
    // Callbacks
    std::function<void(const String&, AppBase::AppState, AppBase::AppState)> appStateChangeCallback;
    std::function<void(size_t, size_t)> memoryWarningCallback;

    // System limits
    static const size_t MAX_TOTAL_MEMORY = 2 * 1024 * 1024; // 2MB total
    static const uint8_t MAX_RUNNING_APPS = 8;
    static const uint32_t MEMORY_CHECK_INTERVAL = 5000; // 5 seconds
    static const uint32_t UPDATE_INTERVAL = 100; // 100ms

    uint32_t lastMemoryCheck;
    uint32_t lastUpdate;
    bool initialized;

    // Internal methods
    bool checkDependencies(const String& appId) const;
    bool hasCircularDependency(const String& appId, std::vector<String>& visited) const;
    void onAppStateChange(AppBase* app, AppBase::AppState oldState, AppBase::AppState newState);
    void updateSystemStats();
    void cleanupStoppedApps();
    bool canLaunchApp(const String& appId) const;
    void createAppSwitcherUI();
    void updateAppSwitcherUI();
    String findBestAppToKill() const;
    void killAppForMemory();

    // Configuration helpers
    String getConfigPath() const;
    bool createConfigDirectory();

    // Mutex for thread safety
    SemaphoreHandle_t managerMutex;
};

/**
 * @brief Message structure for inter-app communication
 */
struct AppMessage {
    String fromApp;
    String toApp;
    String message;
    String data;
    uint32_t timestamp;
    uint32_t messageId;
};

/**
 * @brief Helper macros for app registration
 */
#define REGISTER_APP(appClass, appId, autoStart, ...) \
    do { \
        auto factory = new TemplateAppFactory<appClass>(appClass::getAppInfo()); \
        AppManager::getInstance().registerApp(appId, factory, autoStart, {__VA_ARGS__}); \
    } while(0)

#define GET_APP_MANAGER() AppManager::getInstance()

#endif // APP_MANAGER_H