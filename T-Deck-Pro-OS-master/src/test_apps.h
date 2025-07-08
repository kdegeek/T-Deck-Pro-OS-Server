#ifndef TEST_APPS_H
#define TEST_APPS_H

#include "core/apps/app_manager.h"
#include "apps/meshtastic_app.h"
#include "apps/file_manager_app.h"
#include "apps/settings_app.h"

/**
 * @brief Comprehensive test suite for Phase 3 Application Framework
 * 
 * Tests all components of the application framework including:
 * - AppBase functionality
 * - AppManager operations
 * - Application lifecycle management
 * - Inter-app communication
 * - Memory management
 * - Configuration management
 */
class AppFrameworkTests {
public:
    static bool runAllTests();
    
    // Core framework tests
    static bool testAppBaseLifecycle();
    static bool testAppManagerInitialization();
    static bool testAppRegistration();
    static bool testAppLaunching();
    static bool testAppStateManagement();
    static bool testMemoryManagement();
    static bool testInterAppCommunication();
    static bool testConfigurationManagement();
    
    // Application-specific tests
    static bool testMeshtasticApp();
    static bool testFileManagerApp();
    static bool testSettingsApp();
    
    // Integration tests
    static bool testMultipleAppsRunning();
    static bool testAppSwitching();
    static bool testSystemResourceManagement();
    static bool testErrorHandling();
    
    // Performance tests
    static bool testMemoryUsage();
    static bool testStartupTime();
    static bool testResponseTime();
    
private:
    static void logTestResult(const String& testName, bool passed);
    static bool validateAppState(AppBase* app, AppBase::AppState expectedState);
    static bool validateMemoryUsage(size_t maxExpected);
    static void simulateUserInteraction();
    static void simulateSystemEvents();
};

/**
 * @brief Mock application for testing purposes
 */
class MockApp : public AppBase {
public:
    MockApp(const AppInfo& info) : AppBase(info), initCalled(false), startCalled(false) {}
    
    bool initialize() override {
        initCalled = true;
        return true;
    }
    
    bool start() override {
        startCalled = true;
        return true;
    }
    
    bool pause() override { return true; }
    bool resume() override { return true; }
    bool stop() override { return true; }
    void cleanup() override {}
    
    static AppInfo getAppInfo() {
        return {
            .name = "MockApp",
            .version = "1.0.0",
            .description = "Test application for framework testing",
            .author = "Test Suite",
            .memoryUsage = 0,
            .priority = AppPriority::NORMAL,
            .canRunInBackground = true,
            .requiresNetwork = false,
            .requiresSD = false,
            .iconPath = ""
        };
    }
    
    bool initCalled;
    bool startCalled;
};

#endif // TEST_APPS_H