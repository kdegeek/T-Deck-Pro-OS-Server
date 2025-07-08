# Phase 3: Application Framework Implementation

## Overview

Phase 3 of the T-Deck-Pro OS development implements a comprehensive application framework that provides:

- **Application Base Class**: Common functionality for all applications
- **Application Manager**: Central coordinator for application lifecycle
- **Core Applications**: Meshtastic, File Manager, and Settings applications
- **Inter-App Communication**: Message passing between applications
- **Memory Management**: Resource allocation and monitoring
- **Configuration Management**: Persistent settings and preferences

## Architecture

### Core Components

#### 1. AppBase Class (`src/core/apps/app_base.h/cpp`)

The foundation class that all applications must inherit from, providing:

**Key Features:**
- **Lifecycle Management**: Initialize, start, pause, resume, stop, cleanup
- **State Management**: Tracked state transitions with validation
- **Memory Management**: Per-app memory allocation tracking with limits
- **UI Management**: LVGL container management and event handling
- **Configuration**: Save/load application-specific settings
- **Event Handling**: Key press, touch, network, and battery events

**State Machine:**
```
STOPPED → STARTING → RUNNING ⇄ PAUSED
    ↑         ↓         ↓         ↓
    └─────── STOPPING ←─────────────┘
```

**Memory Management:**
- Per-app memory limit: 512KB
- Automatic tracking of all allocations
- Memory leak detection and cleanup
- Integration with system memory monitoring

#### 2. AppManager Class (`src/core/apps/app_manager.h/cpp`)

Central application coordinator implementing singleton pattern:

**Key Features:**
- **Application Registration**: Dynamic app registration with factories
- **Lifecycle Control**: Launch, pause, resume, stop applications
- **Resource Management**: Memory limits, app count limits, dependency checking
- **Inter-App Communication**: Message passing with handlers
- **Active App Management**: Focus and UI switching
- **System Monitoring**: Memory usage, performance statistics
- **Configuration**: System-wide settings persistence

**Limits and Constraints:**
- Maximum running apps: 8
- Total memory limit: 2MB
- Dependency resolution with circular dependency detection
- Automatic cleanup of stopped applications

#### 3. Application Factory Pattern

Template-based factory system for dynamic app creation:

```cpp
template<typename T>
class TemplateAppFactory : public AppFactory {
    AppBase* createApp() override { return new T(info); }
    void destroyApp(AppBase* app) override { delete app; }
};

// Registration macro
REGISTER_APP(MeshtasticApp, "meshtastic", true);
```

### Core Applications

#### 1. Meshtastic Application (`src/apps/meshtastic_app.h`)

Comprehensive Meshtastic mesh networking interface:

**Features:**
- **Node Management**: Track mesh nodes with telemetry
- **Message Handling**: Send/receive mesh messages with delivery tracking
- **Channel Management**: Multiple channel support with configuration
- **Map Display**: Geographic visualization of mesh network
- **Telemetry Display**: Battery, signal strength, GPS data
- **Settings**: Configurable mesh parameters

**Data Structures:**
- `MeshNode`: Node information with location and telemetry
- `MeshMessage`: Message with routing and delivery status
- `ChannelConfig`: Channel settings and encryption

**UI Screens:**
- Nodes list with online status
- Message conversation view
- Network map with node positions
- Channel configuration
- Telemetry dashboard
- Settings panel

#### 2. File Manager Application (`src/apps/file_manager_app.h`)

Full-featured file system management:

**Features:**
- **File Operations**: Create, delete, rename, copy, move
- **Directory Navigation**: Breadcrumb navigation with history
- **Multiple View Modes**: List, grid, details views
- **Sorting and Filtering**: By name, size, date, type
- **Clipboard Operations**: Cut, copy, paste with validation
- **File Selection**: Single and multiple selection
- **Bookmarks**: Quick access to frequently used directories
- **Properties**: Detailed file information display

**View Modes:**
- **List View**: Compact file listing
- **Grid View**: Icon-based layout
- **Details View**: Full file information table

**File Type Support:**
- Text files, images, audio, video
- Archives, executables, configuration files
- Custom icons and type detection

#### 3. Settings Application (`src/apps/settings_app.h`)

Comprehensive system configuration:

**Categories:**
- **System**: Basic system settings and information
- **Display**: Brightness, theme, language, rotation
- **Communication**: WiFi, cellular, LoRa configuration
- **Power**: Power saving, sleep settings, CPU frequency
- **Security**: Lock settings, encryption, authentication
- **Applications**: App preferences and permissions
- **Advanced**: Developer settings, debugging options
- **About**: System information and diagnostics

**Setting Types:**
- Boolean toggles, integer/float sliders
- String inputs, enumeration dropdowns
- Color pickers, time selectors
- Password fields with validation

**Features:**
- **Import/Export**: Settings backup and restore
- **Reset Options**: Individual category or factory reset
- **Validation**: Real-time setting validation
- **Dependencies**: Setting interdependencies
- **Restart Notifications**: Settings requiring system restart

## Integration

### Main Application Integration

The application framework is integrated into the main system in `src/main.cpp`:

```cpp
void setup_applications() {
    // Initialize application manager
    AppManager& appManager = AppManager::getInstance();
    appManager.initialize();
    
    // Register core applications
    REGISTER_APP(MeshtasticApp, "meshtastic", true);
    REGISTER_APP(FileManagerApp, "file_manager", false);
    REGISTER_APP(SettingsApp, "settings", false);
    
    // Auto-start applications
    appManager.autoStartApps();
}

void main_task(void* parameter) {
    AppManager& appManager = AppManager::getInstance();
    
    while (1) {
        // Update application manager
        appManager.update();
        
        // Handle system events
        if (low_memory) {
            appManager.handleMemoryWarning();
        }
        
        // Periodic maintenance
        if (periodic_save) {
            appManager.saveSystemConfig();
        }
    }
}
```

### Communication Stack Integration

Applications integrate with the Phase 2 communication stack:

```cpp
class MeshtasticApp : public AppBase {
private:
    CommunicationManager* commManager;
    
public:
    bool initialize() override {
        commManager = CommunicationManager::getInstance();
        // Configure for LoRa mesh networking
        commManager->setPreferredInterface(COMM_INTERFACE_LORA);
        return true;
    }
    
    bool sendMessage(uint32_t toNode, const String& message) {
        // Encode mesh message
        uint8_t meshPacket[256];
        size_t packetSize = encodeMeshMessage(toNode, message, meshPacket);
        
        // Send via communication manager
        return commManager->sendMessage(meshPacket, packetSize, COMM_INTERFACE_LORA);
    }
};
```

## Testing

### Comprehensive Test Suite (`src/test_apps.h/cpp`)

**Test Categories:**
- **Core Framework Tests**: AppBase lifecycle, AppManager operations
- **Application Tests**: Individual app functionality
- **Integration Tests**: Multi-app scenarios, resource management
- **Performance Tests**: Memory usage, startup time, response time
- **Error Handling**: Invalid operations, edge cases

**Mock Application:**
- Simplified test app for framework validation
- Lifecycle tracking and state verification
- Memory allocation testing

**Test Execution:**
```cpp
bool runAllTests() {
    bool allPassed = true;
    allPassed &= testAppBaseLifecycle();
    allPassed &= testAppManagerInitialization();
    allPassed &= testAppRegistration();
    allPassed &= testAppLaunching();
    allPassed &= testMemoryManagement();
    allPassed &= testInterAppCommunication();
    return allPassed;
}
```

## Memory Management

### Per-Application Limits
- **Individual App Limit**: 512KB per application
- **System Total Limit**: 2MB for all applications
- **Automatic Cleanup**: Memory leak detection and cleanup
- **Garbage Collection**: Periodic cleanup of unused resources

### Memory Monitoring
- Real-time memory usage tracking
- Low memory warnings and handling
- Automatic app termination when limits exceeded
- Memory statistics and reporting

## Configuration Management

### System Configuration
- **Persistent Storage**: SPIFFS-based configuration files
- **Hierarchical Settings**: Category-based organization
- **Validation**: Real-time setting validation
- **Backup/Restore**: Configuration import/export

### Application Configuration
- **Per-App Settings**: Individual app configuration files
- **Automatic Save/Load**: Lifecycle-integrated persistence
- **Default Values**: Fallback configuration
- **Reset Capabilities**: Individual and system-wide reset

## Inter-Application Communication

### Message Passing System
```cpp
// Send message between apps
appManager.sendMessage("sender_app", "receiver_app", "message_type", "data");

// Register message handler
appManager.setMessageHandler("receiver_app", 
    [](const String& from, const String& msg, const String& data) {
        // Handle incoming message
    });
```

### Event Broadcasting
- System events (network, battery, memory)
- Application events (state changes, errors)
- User events (key press, touch, gestures)

## Performance Characteristics

### Startup Performance
- **App Registration**: < 10ms per app
- **App Launch**: < 1000ms for typical app
- **Memory Allocation**: < 1ms for small allocations
- **State Transitions**: < 5ms per transition

### Runtime Performance
- **Update Cycle**: 100ms interval for app manager
- **Memory Check**: 5-second interval
- **UI Updates**: 10ms interval for smooth interaction
- **Message Passing**: < 10ms latency

### Memory Efficiency
- **Base Framework**: ~50KB overhead
- **Per-App Overhead**: ~10KB base allocation
- **UI Components**: Variable based on complexity
- **Configuration**: ~1-5KB per app

## Future Enhancements

### Phase 4 Integration Points
- **Server Communication**: OTA updates, cloud sync
- **External Services**: API integration, data sync
- **Advanced UI**: Themes, animations, gestures
- **Plugin System**: Dynamic app loading

### Extensibility
- **Custom App Types**: Specialized application bases
- **Service Apps**: Background-only applications
- **Widget System**: Embeddable app components
- **Scripting Support**: Lua/JavaScript app scripting

## Development Guidelines

### Creating New Applications

1. **Inherit from AppBase**:
```cpp
class MyApp : public AppBase {
public:
    MyApp(const AppInfo& info) : AppBase(info) {}
    
    // Implement required methods
    bool initialize() override;
    bool start() override;
    // ... other lifecycle methods
    
    static AppInfo getAppInfo() {
        return {
            .name = "My Application",
            .version = "1.0.0",
            // ... other info
        };
    }
};
```

2. **Register Application**:
```cpp
REGISTER_APP(MyApp, "my_app", false);
```

3. **Handle Events**:
```cpp
void onKeyPress(uint8_t key) override {
    // Handle key input
}

void onNetworkChange(bool connected) override {
    // Handle network events
}
```

### Best Practices

- **Memory Management**: Always use app-provided allocation methods
- **State Management**: Validate state transitions
- **Error Handling**: Graceful degradation and recovery
- **Configuration**: Provide sensible defaults
- **UI Design**: Follow system design guidelines
- **Testing**: Comprehensive unit and integration tests

## Conclusion

Phase 3 provides a robust, scalable application framework that enables:

- **Modular Development**: Clean separation of concerns
- **Resource Management**: Efficient memory and CPU usage
- **User Experience**: Smooth app switching and interaction
- **Extensibility**: Easy addition of new applications
- **Reliability**: Comprehensive error handling and recovery

The framework is ready for Phase 4 integration with server infrastructure and external services, providing a solid foundation for the complete T-Deck-Pro OS ecosystem.