#include "test_apps.h"
#include "core/utils/logger.h"

bool AppFrameworkTests::runAllTests() {
    Logger::info("AppFrameworkTests", "Starting Phase 3 Application Framework Tests");
    
    bool allPassed = true;
    
    // Core framework tests
    allPassed &= testAppBaseLifecycle();
    allPassed &= testAppManagerInitialization();
    allPassed &= testAppRegistration();
    allPassed &= testAppLaunching();
    allPassed &= testAppStateManagement();
    allPassed &= testMemoryManagement();
    allPassed &= testInterAppCommunication();
    allPassed &= testConfigurationManagement();
    
    // Application-specific tests
    allPassed &= testMeshtasticApp();
    allPassed &= testFileManagerApp();
    allPassed &= testSettingsApp();
    
    // Integration tests
    allPassed &= testMultipleAppsRunning();
    allPassed &= testAppSwitching();
    allPassed &= testSystemResourceManagement();
    allPassed &= testErrorHandling();
    
    // Performance tests
    allPassed &= testMemoryUsage();
    allPassed &= testStartupTime();
    allPassed &= testResponseTime();
    
    Logger::info("AppFrameworkTests", 
                String("Phase 3 Application Framework Tests ") + 
                (allPassed ? "PASSED" : "FAILED"));
    
    return allPassed;
}

bool AppFrameworkTests::testAppBaseLifecycle() {
    Logger::info("AppFrameworkTests", "Testing AppBase lifecycle management");
    
    // Create mock app
    MockApp::AppInfo info = MockApp::getAppInfo();
    MockApp* app = new MockApp(info);
    
    bool passed = true;
    
    // Test initial state
    passed &= validateAppState(app, AppBase::AppState::STOPPED);
    logTestResult("Initial state", passed);
    
    // Test state transitions
    passed &= app->setState(AppBase::AppState::STARTING);
    passed &= validateAppState(app, AppBase::AppState::STARTING);
    logTestResult("State transition to STARTING", passed);
    
    // Test initialization
    passed &= app->initialize();
    passed &= app->initCalled;
    logTestResult("App initialization", passed);
    
    // Test start
    passed &= app->start();
    passed &= app->startCalled;
    passed &= app->setState(AppBase::AppState::RUNNING);
    passed &= validateAppState(app, AppBase::AppState::RUNNING);
    logTestResult("App start", passed);
    
    // Test pause/resume
    passed &= app->setState(AppBase::AppState::PAUSING);
    passed &= app->pause();
    passed &= app->setState(AppBase::AppState::PAUSED);
    passed &= validateAppState(app, AppBase::AppState::PAUSED);
    logTestResult("App pause", passed);
    
    passed &= app->setState(AppBase::AppState::RESUMING);
    passed &= app->resume();
    passed &= app->setState(AppBase::AppState::RUNNING);
    passed &= validateAppState(app, AppBase::AppState::RUNNING);
    logTestResult("App resume", passed);
    
    // Test stop
    passed &= app->setState(AppBase::AppState::STOPPING);
    passed &= app->stop();
    passed &= app->setState(AppBase::AppState::STOPPED);
    passed &= validateAppState(app, AppBase::AppState::STOPPED);
    logTestResult("App stop", passed);
    
    // Cleanup
    app->cleanup();
    delete app;
    
    logTestResult("AppBase lifecycle", passed);
    return passed;
}

bool AppFrameworkTests::testAppManagerInitialization() {
    Logger::info("AppFrameworkTests", "Testing AppManager initialization");
    
    AppManager& manager = AppManager::getInstance();
    
    // Test singleton pattern
    AppManager& manager2 = AppManager::getInstance();
    bool passed = (&manager == &manager2);
    logTestResult("Singleton pattern", passed);
    
    // Test initialization
    manager.initialize();
    
    // Test initial state
    passed &= (manager.getRunningApps().size() == 0);
    passed &= (manager.getRegisteredApps().size() == 0);
    passed &= (manager.getActiveApp().isEmpty());
    logTestResult("Initial state", passed);
    
    logTestResult("AppManager initialization", passed);
    return passed;
}

bool AppFrameworkTests::testAppRegistration() {
    Logger::info("AppFrameworkTests", "Testing app registration");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Create mock app factory
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    
    // Test registration
    passed &= manager.registerApp("test_app", factory, false);
    passed &= manager.isAppRegistered("test_app");
    passed &= (manager.getRegisteredApps().size() == 1);
    logTestResult("App registration", passed);
    
    // Test duplicate registration
    auto factory2 = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    passed &= !manager.registerApp("test_app", factory2, false);
    delete factory2; // Clean up unused factory
    logTestResult("Duplicate registration prevention", passed);
    
    // Test unregistration
    passed &= manager.unregisterApp("test_app");
    passed &= !manager.isAppRegistered("test_app");
    passed &= (manager.getRegisteredApps().size() == 0);
    logTestResult("App unregistration", passed);
    
    logTestResult("App registration", passed);
    return passed;
}

bool AppFrameworkTests::testAppLaunching() {
    Logger::info("AppFrameworkTests", "Testing app launching");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Register test app
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    manager.registerApp("test_app", factory, false);
    
    // Test launching
    AppManager::LaunchResult result = manager.launchApp("test_app");
    passed &= (result == AppManager::LaunchResult::SUCCESS);
    passed &= manager.isAppRunning("test_app");
    passed &= (manager.getRunningApps().size() == 1);
    logTestResult("App launching", passed);
    
    // Test duplicate launch
    result = manager.launchApp("test_app");
    passed &= (result == AppManager::LaunchResult::APP_ALREADY_RUNNING);
    logTestResult("Duplicate launch prevention", passed);
    
    // Test launching non-existent app
    result = manager.launchApp("non_existent");
    passed &= (result == AppManager::LaunchResult::APP_NOT_FOUND);
    logTestResult("Non-existent app launch", passed);
    
    // Test stopping
    passed &= manager.stopApp("test_app");
    passed &= !manager.isAppRunning("test_app");
    passed &= (manager.getRunningApps().size() == 0);
    logTestResult("App stopping", passed);
    
    // Cleanup
    manager.unregisterApp("test_app");
    
    logTestResult("App launching", passed);
    return passed;
}

bool AppFrameworkTests::testAppStateManagement() {
    Logger::info("AppFrameworkTests", "Testing app state management");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Register and launch test app
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    manager.registerApp("test_app", factory, false);
    manager.launchApp("test_app");
    
    AppBase* app = manager.getApp("test_app");
    passed &= (app != nullptr);
    passed &= app->isRunning();
    logTestResult("App state after launch", passed);
    
    // Test pause/resume
    passed &= manager.pauseApp("test_app");
    passed &= app->isPaused();
    logTestResult("App pause", passed);
    
    passed &= manager.resumeApp("test_app");
    passed &= app->isRunning();
    logTestResult("App resume", passed);
    
    // Test restart
    passed &= manager.restartApp("test_app");
    passed &= app->isRunning();
    logTestResult("App restart", passed);
    
    // Cleanup
    manager.stopApp("test_app");
    manager.unregisterApp("test_app");
    
    logTestResult("App state management", passed);
    return passed;
}

bool AppFrameworkTests::testMemoryManagement() {
    Logger::info("AppFrameworkTests", "Testing memory management");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Test system memory stats
    AppManager::SystemStats stats = manager.getSystemStats();
    passed &= (stats.availableMemory > 0);
    passed &= (stats.totalMemoryUsed >= 0);
    logTestResult("System memory stats", passed);
    
    // Test memory limits
    passed &= manager.checkMemoryLimits();
    logTestResult("Memory limits check", passed);
    
    // Test memory usage tracking
    size_t initialMemory = manager.getTotalMemoryUsage();
    
    // Launch app and check memory increase
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    manager.registerApp("test_app", factory, false);
    manager.launchApp("test_app");
    
    size_t memoryAfterLaunch = manager.getTotalMemoryUsage();
    passed &= (memoryAfterLaunch >= initialMemory);
    logTestResult("Memory usage tracking", passed);
    
    // Cleanup
    manager.stopApp("test_app");
    manager.unregisterApp("test_app");
    
    logTestResult("Memory management", passed);
    return passed;
}

bool AppFrameworkTests::testInterAppCommunication() {
    Logger::info("AppFrameworkTests", "Testing inter-app communication");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Register two test apps
    auto factory1 = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    auto factory2 = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    
    manager.registerApp("app1", factory1, false);
    manager.registerApp("app2", factory2, false);
    manager.launchApp("app1");
    manager.launchApp("app2");
    
    // Test message sending
    bool messageReceived = false;
    manager.setMessageHandler("app2", [&messageReceived](const String& from, const String& msg, const String& data) {
        messageReceived = (from == "app1" && msg == "test_message");
    });
    
    passed &= manager.sendMessage("app1", "app2", "test_message", "test_data");
    
    // Give some time for message processing
    delay(100);
    
    passed &= messageReceived;
    logTestResult("Message sending", passed);
    
    // Cleanup
    manager.stopApp("app1");
    manager.stopApp("app2");
    manager.unregisterApp("app1");
    manager.unregisterApp("app2");
    
    logTestResult("Inter-app communication", passed);
    return passed;
}

bool AppFrameworkTests::testConfigurationManagement() {
    Logger::info("AppFrameworkTests", "Testing configuration management");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Test system config save/load
    passed &= manager.saveSystemConfig();
    passed &= manager.loadSystemConfig();
    logTestResult("System configuration", passed);
    
    // Test app-specific config
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    manager.registerApp("test_app", factory, false);
    manager.launchApp("test_app");
    
    AppBase* app = manager.getApp("test_app");
    passed &= app->saveConfig();
    passed &= app->loadConfig();
    logTestResult("App configuration", passed);
    
    // Cleanup
    manager.stopApp("test_app");
    manager.unregisterApp("test_app");
    
    logTestResult("Configuration management", passed);
    return passed;
}

bool AppFrameworkTests::testMeshtasticApp() {
    Logger::info("AppFrameworkTests", "Testing Meshtastic application");
    
    // Test app info
    MeshtasticApp::AppInfo info = MeshtasticApp::getAppInfo();
    bool passed = (info.name == "Meshtastic");
    logTestResult("Meshtastic app info", passed);
    
    // TODO: Add more specific Meshtastic tests when implementation is complete
    
    logTestResult("Meshtastic application", passed);
    return passed;
}

bool AppFrameworkTests::testFileManagerApp() {
    Logger::info("AppFrameworkTests", "Testing File Manager application");
    
    // Test app info
    FileManagerApp::AppInfo info = FileManagerApp::getAppInfo();
    bool passed = (info.name == "File Manager");
    logTestResult("File Manager app info", passed);
    
    // TODO: Add more specific File Manager tests when implementation is complete
    
    logTestResult("File Manager application", passed);
    return passed;
}

bool AppFrameworkTests::testSettingsApp() {
    Logger::info("AppFrameworkTests", "Testing Settings application");
    
    // Test app info
    SettingsApp::AppInfo info = SettingsApp::getAppInfo();
    bool passed = (info.name == "Settings");
    logTestResult("Settings app info", passed);
    
    // TODO: Add more specific Settings tests when implementation is complete
    
    logTestResult("Settings application", passed);
    return passed;
}

bool AppFrameworkTests::testMultipleAppsRunning() {
    Logger::info("AppFrameworkTests", "Testing multiple apps running");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Register multiple apps
    for (int i = 0; i < 3; i++) {
        auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
        String appId = "test_app_" + String(i);
        manager.registerApp(appId, factory, false);
        manager.launchApp(appId);
    }
    
    passed &= (manager.getRunningApps().size() == 3);
    logTestResult("Multiple apps running", passed);
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        String appId = "test_app_" + String(i);
        manager.stopApp(appId);
        manager.unregisterApp(appId);
    }
    
    logTestResult("Multiple apps running", passed);
    return passed;
}

bool AppFrameworkTests::testAppSwitching() {
    Logger::info("AppFrameworkTests", "Testing app switching");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Register and launch two apps
    auto factory1 = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    auto factory2 = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    
    manager.registerApp("app1", factory1, false);
    manager.registerApp("app2", factory2, false);
    manager.launchApp("app1");
    manager.launchApp("app2");
    
    // Test active app switching
    manager.setActiveApp("app1");
    passed &= (manager.getActiveApp() == "app1");
    logTestResult("Set active app", passed);
    
    manager.setActiveApp("app2");
    passed &= (manager.getActiveApp() == "app2");
    logTestResult("Switch active app", passed);
    
    // Cleanup
    manager.stopApp("app1");
    manager.stopApp("app2");
    manager.unregisterApp("app1");
    manager.unregisterApp("app2");
    
    logTestResult("App switching", passed);
    return passed;
}

bool AppFrameworkTests::testSystemResourceManagement() {
    Logger::info("AppFrameworkTests", "Testing system resource management");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Test resource monitoring
    AppManager::SystemStats stats = manager.getSystemStats();
    passed &= (stats.uptime > 0);
    passed &= (stats.totalApps >= 0);
    passed &= (stats.runningApps >= 0);
    logTestResult("Resource monitoring", passed);
    
    // Test memory management
    passed &= manager.checkMemoryLimits();
    logTestResult("Memory limits", passed);
    
    logTestResult("System resource management", passed);
    return passed;
}

bool AppFrameworkTests::testErrorHandling() {
    Logger::info("AppFrameworkTests", "Testing error handling");
    
    AppManager& manager = AppManager::getInstance();
    bool passed = true;
    
    // Test invalid operations
    passed &= !manager.stopApp("non_existent");
    passed &= !manager.pauseApp("non_existent");
    passed &= !manager.resumeApp("non_existent");
    passed &= (manager.getApp("non_existent") == nullptr);
    logTestResult("Invalid operations", passed);
    
    // Test null pointer handling
    passed &= !manager.registerApp("null_test", nullptr, false);
    logTestResult("Null pointer handling", passed);
    
    logTestResult("Error handling", passed);
    return passed;
}

bool AppFrameworkTests::testMemoryUsage() {
    Logger::info("AppFrameworkTests", "Testing memory usage");
    
    bool passed = validateMemoryUsage(1024 * 1024); // 1MB limit
    logTestResult("Memory usage", passed);
    
    return passed;
}

bool AppFrameworkTests::testStartupTime() {
    Logger::info("AppFrameworkTests", "Testing startup time");
    
    uint32_t startTime = millis();
    
    AppManager& manager = AppManager::getInstance();
    auto factory = new TemplateAppFactory<MockApp>(MockApp::getAppInfo());
    manager.registerApp("startup_test", factory, false);
    manager.launchApp("startup_test");
    
    uint32_t endTime = millis();
    uint32_t startupTime = endTime - startTime;
    
    bool passed = (startupTime < 1000); // Should start in less than 1 second
    logTestResult("Startup time: " + String(startupTime) + "ms", passed);
    
    // Cleanup
    manager.stopApp("startup_test");
    manager.unregisterApp("startup_test");
    
    return passed;
}

bool AppFrameworkTests::testResponseTime() {
    Logger::info("AppFrameworkTests", "Testing response time");
    
    // Test app manager update cycle
    uint32_t startTime = millis();
    
    AppManager& manager = AppManager::getInstance();
    manager.update();
    
    uint32_t endTime = millis();
    uint32_t responseTime = endTime - startTime;
    
    bool passed = (responseTime < 100); // Should update in less than 100ms
    logTestResult("Response time: " + String(responseTime) + "ms", passed);
    
    return passed;
}

void AppFrameworkTests::logTestResult(const String& testName, bool passed) {
    if (passed) {
        Logger::info("AppFrameworkTests", "✓ " + testName + " PASSED");
    } else {
        Logger::error("AppFrameworkTests", "✗ " + testName + " FAILED");
    }
}

bool AppFrameworkTests::validateAppState(AppBase* app, AppBase::AppState expectedState) {
    return app && (app->getState() == expectedState);
}

bool AppFrameworkTests::validateMemoryUsage(size_t maxExpected) {
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;
    
    return usedHeap <= maxExpected;
}

void AppFrameworkTests::simulateUserInteraction() {
    // Simulate key presses
    AppManager& manager = AppManager::getInstance();
    manager.handleKeyPress(1); // Simulate key 1
    manager.handleKeyPress(2); // Simulate key 2
}

void AppFrameworkTests::simulateSystemEvents() {
    AppManager& manager = AppManager::getInstance();
    
    // Simulate network change
    manager.handleNetworkChange(true);
    delay(10);
    manager.handleNetworkChange(false);
    
    // Simulate battery change
    manager.handleBatteryChange(75);
    delay(10);
    manager.handleBatteryChange(50);
}