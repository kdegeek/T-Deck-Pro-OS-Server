#ifndef APP_BASE_H
#define APP_BASE_H

#include <Arduino.h>
#include <lvgl.h>
#include <functional>

/**
 * @brief Base class for all applications in the T-Deck-Pro OS
 * 
 * Provides common functionality and lifecycle management for applications.
 * All applications must inherit from this class and implement the pure virtual methods.
 */
class AppBase {
public:
    enum class AppState {
        STOPPED,
        STARTING,
        RUNNING,
        PAUSING,
        PAUSED,
        RESUMING,
        STOPPING
    };

    enum class AppPriority {
        LOW = 1,
        NORMAL = 2,
        HIGH = 3,
        CRITICAL = 4
    };

    struct AppInfo {
        String name;
        String version;
        String description;
        String author;
        size_t memoryUsage;
        AppPriority priority;
        bool canRunInBackground;
        bool requiresNetwork;
        bool requiresSD;
        String iconPath;
    };

    /**
     * @brief Constructor
     * @param info Application information structure
     */
    AppBase(const AppInfo& info);
    virtual ~AppBase();

    // Lifecycle methods (pure virtual - must be implemented)
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop() = 0;
    virtual void cleanup() = 0;

    // Event handling (virtual - can be overridden)
    virtual void onKeyPress(uint8_t key) {}
    virtual void onTouch(lv_event_t* e) {}
    virtual void onNetworkChange(bool connected) {}
    virtual void onBatteryChange(uint8_t percentage) {}
    virtual void onMemoryWarning() {}

    // UI management (virtual - can be overridden)
    virtual lv_obj_t* createUI(lv_obj_t* parent) { return nullptr; }
    virtual void updateUI() {}
    virtual void destroyUI();

    // Getters
    const AppInfo& getInfo() const { return appInfo; }
    AppState getState() const { return currentState; }
    uint32_t getRunTime() const { return millis() - startTime; }
    size_t getCurrentMemoryUsage() const;
    bool isRunning() const { return currentState == AppState::RUNNING; }
    bool isPaused() const { return currentState == AppState::PAUSED; }
    lv_obj_t* getMainContainer() const { return mainContainer; }

    // State management
    bool setState(AppState newState);
    void setStateChangeCallback(std::function<void(AppBase*, AppState, AppState)> callback);

    // Memory management
    void* allocateMemory(size_t size);
    void freeMemory(void* ptr);
    bool checkMemoryLimit();

    // Configuration management
    virtual bool saveConfig();
    virtual bool loadConfig();
    virtual void resetConfig();

protected:
    AppInfo appInfo;
    AppState currentState;
    AppState previousState;
    uint32_t startTime;
    uint32_t pauseTime;
    lv_obj_t* mainContainer;
    std::function<void(AppBase*, AppState, AppState)> stateChangeCallback;

    // Memory tracking
    struct MemoryBlock {
        void* ptr;
        size_t size;
        uint32_t timestamp;
    };
    std::vector<MemoryBlock> allocatedMemory;
    size_t totalAllocatedMemory;

    // Helper methods
    void logStateChange(AppState from, AppState to);
    bool validateStateTransition(AppState from, AppState to);
    void updateMemoryUsage();

    // Configuration helpers
    String getConfigPath() const;
    bool createConfigDirectory();

private:
    static const size_t MAX_MEMORY_PER_APP = 512 * 1024; // 512KB per app
    mutable SemaphoreHandle_t memoryMutex;
};

/**
 * @brief Application factory interface
 * 
 * Used by the app manager to create application instances
 */
class AppFactory {
public:
    virtual ~AppFactory() = default;
    virtual AppBase* createApp() = 0;
    virtual void destroyApp(AppBase* app) = 0;
    virtual AppBase::AppInfo getAppInfo() = 0;
};

/**
 * @brief Template factory for easy app registration
 */
template<typename T>
class TemplateAppFactory : public AppFactory {
private:
    AppBase::AppInfo info;

public:
    TemplateAppFactory(const AppBase::AppInfo& appInfo) : info(appInfo) {}

    AppBase* createApp() override {
        return new T(info);
    }

    void destroyApp(AppBase* app) override {
        delete app;
    }

    AppBase::AppInfo getAppInfo() override {
        return info;
    }
};

#endif // APP_BASE_H