#ifndef MESHTASTIC_APP_H
#define MESHTASTIC_APP_H

#include "../core/apps/app_base.h"
#include "../core/communication/communication_manager.h"
#include <vector>
#include <map>

/**
 * @brief Meshtastic Fancy UI Application
 * 
 * Provides a comprehensive Meshtastic interface with mesh networking,
 * node management, message handling, and telemetry display.
 */
class MeshtasticApp : public AppBase {
public:
    struct MeshNode {
        uint32_t nodeId;
        String shortName;
        String longName;
        float latitude;
        float longitude;
        uint32_t lastSeen;
        uint8_t batteryLevel;
        float voltage;
        int8_t snr;
        int16_t rssi;
        uint8_t hopLimit;
        bool isOnline;
        String firmwareVersion;
        String hardwareModel;
    };

    struct MeshMessage {
        uint32_t messageId;
        uint32_t fromNode;
        uint32_t toNode;
        String message;
        uint32_t timestamp;
        bool isAck;
        bool isDelivered;
        int8_t snr;
        int16_t rssi;
        uint8_t hopCount;
    };

    struct ChannelConfig {
        uint8_t channelIndex;
        String name;
        String psk;
        uint32_t frequency;
        uint8_t modemConfig;
        bool uplink;
        bool downlink;
    };

    enum class MeshScreen {
        NODES,
        MESSAGES,
        MAP,
        CHANNELS,
        SETTINGS,
        TELEMETRY
    };

    MeshtasticApp(const AppInfo& info);
    virtual ~MeshtasticApp();

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
    void onNetworkChange(bool connected) override;

    // UI management
    lv_obj_t* createUI(lv_obj_t* parent) override;
    void updateUI() override;

    // Static app info
    static AppInfo getAppInfo();

    // Mesh operations
    bool sendMessage(uint32_t toNode, const String& message);
    bool sendBroadcast(const String& message);
    bool requestNodeInfo(uint32_t nodeId);
    bool requestTelemetry(uint32_t nodeId);

    // Node management
    void addNode(const MeshNode& node);
    void updateNode(uint32_t nodeId, const MeshNode& node);
    void removeNode(uint32_t nodeId);
    MeshNode* getNode(uint32_t nodeId);
    std::vector<MeshNode> getOnlineNodes() const;
    std::vector<MeshNode> getAllNodes() const;

    // Message management
    void addMessage(const MeshMessage& message);
    std::vector<MeshMessage> getMessages(uint32_t nodeId = 0) const;
    void markMessageDelivered(uint32_t messageId);
    void clearMessages();

    // Channel management
    bool addChannel(const ChannelConfig& channel);
    bool removeChannel(uint8_t channelIndex);
    bool setActiveChannel(uint8_t channelIndex);
    ChannelConfig* getChannel(uint8_t channelIndex);
    std::vector<ChannelConfig> getChannels() const;

    // Configuration
    bool saveConfig() override;
    bool loadConfig() override;
    void resetConfig() override;

private:
    // UI components
    lv_obj_t* mainContainer;
    lv_obj_t* headerPanel;
    lv_obj_t* contentPanel;
    lv_obj_t* statusBar;
    lv_obj_t* tabView;
    
    // Screen objects
    lv_obj_t* nodesScreen;
    lv_obj_t* messagesScreen;
    lv_obj_t* mapScreen;
    lv_obj_t* channelsScreen;
    lv_obj_t* settingsScreen;
    lv_obj_t* telemetryScreen;

    // UI elements
    lv_obj_t* nodesList;
    lv_obj_t* messagesList;
    lv_obj_t* messageInput;
    lv_obj_t* sendButton;
    lv_obj_t* channelSelector;
    lv_obj_t* statusLabel;
    lv_obj_t* nodeCountLabel;
    lv_obj_t* signalStrengthBar;

    // Data
    std::map<uint32_t, MeshNode> meshNodes;
    std::vector<MeshMessage> meshMessages;
    std::vector<ChannelConfig> channels;
    uint8_t activeChannelIndex;
    MeshScreen currentScreen;
    uint32_t myNodeId;
    String myNodeName;

    // Communication
    CommunicationManager* commManager;
    bool meshInitialized;
    uint32_t lastHeartbeat;
    uint32_t lastNodeUpdate;

    // Settings
    struct Settings {
        bool autoReply;
        bool soundEnabled;
        bool gpsEnabled;
        uint8_t transmitPower;
        uint32_t heartbeatInterval;
        uint32_t nodeTimeout;
        bool showOfflineNodes;
        String defaultChannel;
    } settings;

    // UI creation methods
    void createHeaderPanel();
    void createTabView();
    void createNodesScreen();
    void createMessagesScreen();
    void createMapScreen();
    void createChannelsScreen();
    void createSettingsScreen();
    void createTelemetryScreen();
    void createStatusBar();

    // UI update methods
    void updateNodesScreen();
    void updateMessagesScreen();
    void updateMapScreen();
    void updateChannelsScreen();
    void updateTelemetryScreen();
    void updateStatusBar();

    // Event handlers
    static void onTabChanged(lv_event_t* e);
    static void onNodeSelected(lv_event_t* e);
    static void onMessageSend(lv_event_t* e);
    static void onChannelChanged(lv_event_t* e);
    static void onSettingChanged(lv_event_t* e);

    // Mesh protocol handlers
    void handleIncomingMessage(const String& data);
    void handleNodeInfo(const String& data);
    void handleTelemetry(const String& data);
    void handleAck(const String& data);

    // Utility methods
    String formatTimestamp(uint32_t timestamp);
    String formatDistance(float distance);
    String formatSignalStrength(int16_t rssi, int8_t snr);
    float calculateDistance(float lat1, float lon1, float lat2, float lon2);
    void cleanupOldNodes();
    void cleanupOldMessages();
    bool isValidNodeId(uint32_t nodeId);
    uint32_t generateMessageId();

    // Configuration helpers
    String getConfigPath() const;
    bool saveNodeData();
    bool loadNodeData();
    bool saveChannelData();
    bool loadChannelData();

    // Constants
    static const uint32_t HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const uint32_t NODE_TIMEOUT = 300000; // 5 minutes
    static const uint32_t MESSAGE_CLEANUP_INTERVAL = 3600000; // 1 hour
    static const size_t MAX_MESSAGES = 1000;
    static const size_t MAX_NODES = 100;
    static const uint8_t MAX_CHANNELS = 8;
};

#endif // MESHTASTIC_APP_H