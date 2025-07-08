#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "../core/apps/app_base.h"
#include <map>
#include <functional>

/**
 * @brief System Settings Application
 * 
 * Provides comprehensive system configuration including display settings,
 * communication settings, power management, and application preferences.
 */
class SettingsApp : public AppBase {
public:
    enum class SettingType {
        BOOLEAN,
        INTEGER,
        FLOAT,
        STRING,
        ENUM,
        COLOR,
        TIME,
        PASSWORD
    };

    enum class SettingCategory {
        SYSTEM,
        DISPLAY,
        COMMUNICATION,
        POWER,
        SECURITY,
        APPLICATIONS,
        ADVANCED,
        ABOUT
    };

    struct SettingOption {
        String key;
        String name;
        String description;
        SettingType type;
        SettingCategory category;
        String value;
        String defaultValue;
        std::vector<String> enumOptions;
        String minValue;
        String maxValue;
        bool requiresRestart;
        bool isAdvanced;
        std::function<bool(const String&)> validator;
        std::function<void(const String&)> onChange;
    };

    struct CategoryInfo {
        SettingCategory category;
        String name;
        String description;
        String icon;
        std::vector<String> settingKeys;
    };

    SettingsApp(const AppInfo& info);
    virtual ~SettingsApp();

    // AppBase implementation
    bool initialize() override;
    bool start() override;
    bool pause() override;
    bool resume() override;
    bool stop() override;
    void cleanup() override;

    // Event handling
    void onKeyPress(uint8_t key) override;
    void onTouch(lv_event_t* e) override;

    // UI management
    lv_obj_t* createUI(lv_obj_t* parent) override;
    void updateUI() override;

    // Static app info
    static AppInfo getAppInfo();

    // Settings management
    bool registerSetting(const SettingOption& setting);
    bool unregisterSetting(const String& key);
    bool setSetting(const String& key, const String& value);
    String getSetting(const String& key, const String& defaultValue = "") const;
    bool getSettingBool(const String& key, bool defaultValue = false) const;
    int32_t getSettingInt(const String& key, int32_t defaultValue = 0) const;
    float getSettingFloat(const String& key, float defaultValue = 0.0f) const;

    // Category management
    std::vector<CategoryInfo> getCategories() const;
    std::vector<SettingOption> getSettingsForCategory(SettingCategory category) const;
    void setActiveCategory(SettingCategory category);

    // Import/Export
    bool exportSettings(const String& filePath);
    bool importSettings(const String& filePath);
    bool resetToDefaults();
    bool resetCategory(SettingCategory category);

    // System information
    struct SystemInfo {
        String firmwareVersion;
        String hardwareModel;
        String chipId;
        uint32_t flashSize;
        uint32_t freeHeap;
        uint32_t uptime;
        String buildDate;
        String buildTime;
        float cpuFrequency;
        String macAddress;
    };
    SystemInfo getSystemInfo() const;

    // Configuration
    bool saveConfig() override;
    bool loadConfig() override;
    void resetConfig() override;

private:
    // UI components
    lv_obj_t* mainContainer;
    lv_obj_t* sidebarPanel;
    lv_obj_t* contentPanel;
    lv_obj_t* headerPanel;
    lv_obj_t* footerPanel;

    // Sidebar elements
    lv_obj_t* categoryList;
    lv_obj_t* searchBox;
    lv_obj_t* advancedToggle;

    // Content elements
    lv_obj_t* settingsContainer;
    lv_obj_t* scrollContainer;
    lv_obj_t* titleLabel;
    lv_obj_t* descriptionLabel;

    // Footer elements
    lv_obj_t* saveButton;
    lv_obj_t* resetButton;
    lv_obj_t* importButton;
    lv_obj_t* exportButton;

    // Data
    std::map<String, SettingOption> settings;
    std::map<SettingCategory, CategoryInfo> categories;
    SettingCategory activeCategory;
    String searchFilter;
    bool showAdvanced;
    bool hasUnsavedChanges;

    // System settings structure
    struct SystemSettings {
        // Display settings
        uint8_t brightness;
        bool autoRotate;
        uint32_t screenTimeout;
        String theme;
        String language;
        String timezone;
        bool showStatusBar;
        
        // Communication settings
        bool wifiEnabled;
        String wifiSSID;
        String wifiPassword;
        bool cellularEnabled;
        String apn;
        bool loraEnabled;
        uint32_t loraFrequency;
        uint8_t loraPower;
        
        // Power settings
        bool powerSaveMode;
        uint8_t cpuFrequency;
        uint32_t sleepTimeout;
        bool wakeOnMotion;
        bool wakeOnTouch;
        
        // Security settings
        bool lockEnabled;
        String lockPin;
        uint32_t lockTimeout;
        bool encryptStorage;
        
        // Application settings
        String defaultApp;
        bool autoStartApps;
        uint32_t maxMemoryPerApp;
        bool allowBackgroundApps;
        
        // Advanced settings
        bool debugMode;
        String logLevel;
        bool telemetryEnabled;
        bool developerMode;
    } systemSettings;

    // UI creation methods
    void createSidebar();
    void createContentPanel();
    void createHeaderPanel();
    void createFooterPanel();
    void createCategoryList();
    void createSettingsContainer();

    // UI update methods
    void updateCategoryList();
    void updateSettingsContainer();
    void updateHeaderPanel();
    void updateFooterPanel();
    void refreshCurrentCategory();

    // Setting UI creation
    lv_obj_t* createBooleanSetting(const SettingOption& setting);
    lv_obj_t* createIntegerSetting(const SettingOption& setting);
    lv_obj_t* createFloatSetting(const SettingOption& setting);
    lv_obj_t* createStringSetting(const SettingOption& setting);
    lv_obj_t* createEnumSetting(const SettingOption& setting);
    lv_obj_t* createColorSetting(const SettingOption& setting);
    lv_obj_t* createTimeSetting(const SettingOption& setting);
    lv_obj_t* createPasswordSetting(const SettingOption& setting);

    // Event handlers
    static void onCategorySelected(lv_event_t* e);
    static void onSettingChanged(lv_event_t* e);
    static void onSearchChanged(lv_event_t* e);
    static void onAdvancedToggled(lv_event_t* e);
    static void onSaveClicked(lv_event_t* e);
    static void onResetClicked(lv_event_t* e);
    static void onImportClicked(lv_event_t* e);
    static void onExportClicked(lv_event_t* e);

    // Settings initialization
    void initializeSystemSettings();
    void initializeDisplaySettings();
    void initializeCommunicationSettings();
    void initializePowerSettings();
    void initializeSecuritySettings();
    void initializeApplicationSettings();
    void initializeAdvancedSettings();
    void initializeAboutSettings();

    // Settings validation
    bool validateWifiSSID(const String& ssid);
    bool validateWifiPassword(const String& password);
    bool validatePin(const String& pin);
    bool validateFrequency(const String& frequency);
    bool validateTimeout(const String& timeout);

    // Settings change handlers
    void onBrightnessChanged(const String& value);
    void onThemeChanged(const String& value);
    void onLanguageChanged(const String& value);
    void onWifiSettingsChanged(const String& value);
    void onPowerModeChanged(const String& value);
    void onSecuritySettingsChanged(const String& value);

    // Utility methods
    void markUnsavedChanges();
    void clearUnsavedChanges();
    bool confirmUnsavedChanges();
    void applySettings();
    void revertSettings();
    String formatValue(const SettingOption& setting, const String& value);
    bool isValidValue(const SettingOption& setting, const String& value);

    // File operations
    bool saveSettingsToFile(const String& filePath);
    bool loadSettingsFromFile(const String& filePath);
    String getSettingsFilePath() const;
    String getBackupFilePath() const;

    // System operations
    void restartSystem();
    void factoryReset();
    void updateFirmware();
    void calibrateTouch();
    void calibrateBattery();

    // Dialog helpers
    void showResetConfirmDialog();
    void showRestartDialog();
    void showFactoryResetDialog();
    void showImportDialog();
    void showExportDialog();
    void showErrorDialog(const String& message);
    void showSuccessDialog(const String& message);

    // Constants
    static const uint32_t AUTO_SAVE_INTERVAL = 30000; // 30 seconds
    static const size_t MAX_SETTING_NAME_LENGTH = 64;
    static const size_t MAX_SETTING_VALUE_LENGTH = 256;
    static const uint8_t MIN_BRIGHTNESS = 10;
    static const uint8_t MAX_BRIGHTNESS = 100;
    static const uint32_t MIN_TIMEOUT = 5000; // 5 seconds
    static const uint32_t MAX_TIMEOUT = 3600000; // 1 hour
};

#endif // SETTINGS_APP_H