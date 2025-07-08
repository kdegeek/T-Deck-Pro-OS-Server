# Phase 2: Communication Stack Implementation

## Overview

Phase 2 of the T-Deck-Pro OS development focuses on implementing a comprehensive communication stack that manages multiple communication interfaces including LoRa, WiFi, and Cellular (4G). This phase provides the foundation for all network communication in the operating system.

## Architecture

The communication stack follows a layered architecture:

```
┌─────────────────────────────────────────────────────────┐
│                Application Layer                        │
├─────────────────────────────────────────────────────────┤
│            Communication Manager (Coordinator)          │
├─────────────────────────────────────────────────────────┤
│  LoRa Manager  │  WiFi Manager  │  Cellular Manager    │
├─────────────────────────────────────────────────────────┤
│     SX1262     │   ESP32 WiFi   │     A7682E 4G        │
│   (RadioLib)   │   (ESP-IDF)    │   (AT Commands)      │
└─────────────────────────────────────────────────────────┘
```

## Components Implemented

### 1. LoRa Manager (`src/core/communication/lora_manager.h/cpp`)

**Features:**
- Complete SX1262 radio control using RadioLib framework
- Multiple operating modes (Sleep, Standby, Transmit, Receive)
- Interrupt-driven transmission and reception
- Comprehensive configuration management
- Statistics tracking and error handling
- FreeRTOS task integration
- Burn-in prevention for display integration

**Key Methods:**
- `initialize()` - Initialize LoRa radio with SX1262 configuration
- `transmit()` - Send data with interrupt-driven completion
- `receive()` - Receive data with timeout and interrupt handling
- `setMode()` - Switch between operating modes
- `getStatistics()` - Get transmission/reception statistics

**Hardware Integration:**
- SX1262 LoRa radio on SPI bus
- Interrupt handling on DIO1 pin
- Shared SPI with E-ink display (coordinated access)

### 2. WiFi Manager (`src/core/communication/wifi_manager.h/cpp`)

**Features:**
- ESP32 WiFi management with station and access point modes
- Network scanning and connection management
- Automatic reconnection with exponential backoff
- Event-driven architecture with callbacks
- Power management integration
- Comprehensive error handling and statistics

**Key Methods:**
- `initialize()` - Initialize WiFi subsystem
- `connectToNetwork()` - Connect to specified WiFi network
- `startAccessPoint()` - Create WiFi hotspot
- `scanNetworks()` - Scan for available networks
- `isConnected()` - Check connection status

**Operating Modes:**
- Station Mode: Connect to existing WiFi networks
- Access Point Mode: Create WiFi hotspot for configuration
- Dual Mode: Simultaneous station and AP operation

### 3. Cellular Manager (`src/core/communication/cellular_manager.h/cpp`)

**Features:**
- A7682E 4G modem control via AT commands
- SMS functionality for messaging
- Voice call capabilities
- Network registration and signal monitoring
- PDP context management for data connections
- Power management and modem control

**Key Methods:**
- `initialize()` - Initialize cellular modem
- `powerOn()` - Power on and configure modem
- `sendSMS()` - Send SMS messages
- `makeCall()` - Initiate voice calls
- `setupPDP()` - Configure data connection
- `getSignalStrength()` - Monitor signal quality

**AT Command Interface:**
- Robust AT command sending with timeout
- Response parsing and error handling
- Asynchronous event processing

### 4. Communication Manager (`src/core/communication/communication_manager.h/cpp`)

**Features:**
- High-level coordinator for all communication interfaces
- Unified API for sending and receiving messages
- Automatic interface selection and failover
- Interface availability monitoring
- Comprehensive statistics aggregation
- Thread-safe operation with FreeRTOS

**Key Methods:**
- `sendMessage()` - Send message via best available interface
- `receiveMessage()` - Receive messages from any interface
- `setPreferredInterface()` - Set interface preference
- `setAutoFailover()` - Enable/disable automatic failover
- `getStatistics()` - Get aggregated communication statistics

**Interface Selection Logic:**
1. Preferred interface (if available)
2. WiFi (high bandwidth, low power)
3. Cellular (wide coverage, medium power)
4. LoRa (long range, low power, low bandwidth)

## Hardware Configuration

### Pin Assignments (from `board_config.h`)

**LoRa (SX1262):**
- CS: GPIO 9
- RST: GPIO 17
- DIO1: GPIO 45 (interrupt)
- BUSY: GPIO 46
- SCK: GPIO 14 (shared with E-ink)
- MOSI: GPIO 15 (shared with E-ink)
- MISO: GPIO 16

**Cellular (A7682E):**
- PWR: GPIO 4
- DTR: GPIO 5
- RI: GPIO 6
- TX: GPIO 43
- RX: GPIO 44
- PWRKEY: GPIO 7

**WiFi:**
- Built-in ESP32-S3 WiFi (no external pins required)

### Task Priorities

```cpp
#define LORA_TASK_PRIORITY         (configMAX_PRIORITIES - 4)
#define WIFI_TASK_PRIORITY         (configMAX_PRIORITIES - 5)
#define CELLULAR_TASK_PRIORITY     (configMAX_PRIORITIES - 6)
```

## Dependencies

### PlatformIO Libraries

```ini
lib_deps = 
    ; Communication Libraries
    jgromes/RadioLib@^6.4.2          ; LoRa SX1262 control
    knolleary/PubSubClient@^2.8      ; MQTT for WiFi
    
    ; Networking
    https://github.com/tzapu/WiFiManager.git  ; WiFi configuration
```

### System Dependencies

- ESP-IDF WiFi stack
- FreeRTOS for task management
- ESP32 interrupt system
- SPI bus for LoRa communication
- UART for cellular AT commands

## Integration Points

### Main Application Integration

The communication stack is integrated into the main application through:

1. **Initialization** (`setup_communication()` in `main.cpp`):
   ```cpp
   CommunicationManager* commMgr = CommunicationManager::getInstance();
   commMgr->initialize();
   commMgr->setPreferredInterface(COMM_INTERFACE_WIFI);
   commMgr->setAutoFailover(true);
   ```

2. **Communication Task** (`comm_task()` in `main.cpp`):
   - Handles message reception
   - Sends periodic test messages
   - Logs communication statistics
   - Monitors interface health

### Display Integration

- Coordinated SPI bus access with E-ink display
- Burn-in prevention during LoRa operations
- Status display of communication interfaces

## Testing

### Test Application (`src/test_communication.cpp`)

Comprehensive test suite including:

1. **Interface Availability Testing**
   - Check which interfaces are operational
   - Verify hardware initialization

2. **Message Transmission Testing**
   - Send test messages via different interfaces
   - Verify successful transmission

3. **Message Reception Testing**
   - Listen for incoming messages
   - Test message parsing and handling

4. **Interface Switching Testing**
   - Test preferred interface selection
   - Verify automatic failover functionality

5. **Statistics Testing**
   - Verify statistics collection
   - Test statistics aggregation

### Running Tests

```cpp
#include "test_communication.h"

void setup() {
    // Initialize system
    run_communication_tests();
}
```

## Performance Characteristics

### LoRa Interface
- **Range:** Up to 10km line-of-sight
- **Data Rate:** 0.3 - 50 kbps (configurable)
- **Power:** 20mA transmit, 1.5mA receive
- **Latency:** 100-500ms depending on spreading factor

### WiFi Interface
- **Range:** 50-100m typical
- **Data Rate:** Up to 150 Mbps
- **Power:** 80mA active, 5mA idle
- **Latency:** 10-50ms

### Cellular Interface
- **Range:** Cell tower coverage (km)
- **Data Rate:** Up to 100 Mbps (4G LTE)
- **Power:** 200mA transmit, 10mA idle
- **Latency:** 50-200ms

## Error Handling

### Robust Error Recovery

1. **Interface Failures:**
   - Automatic failover to backup interfaces
   - Retry mechanisms with exponential backoff
   - Interface health monitoring

2. **Hardware Errors:**
   - SPI bus error recovery
   - UART communication timeouts
   - Interrupt handling failures

3. **Network Errors:**
   - WiFi disconnection handling
   - Cellular network registration failures
   - LoRa transmission failures

### Logging and Diagnostics

- Comprehensive logging at multiple levels
- Statistics collection for performance monitoring
- Error counters for debugging
- Interface status reporting

## Future Enhancements

### Phase 3 Integration Points

1. **Meshtastic Protocol:**
   - LoRa mesh networking
   - Message routing and forwarding
   - Node discovery and management

2. **Application Framework:**
   - Message queuing system
   - Event-driven communication
   - Protocol abstraction layer

3. **Server Integration:**
   - OTA update delivery
   - Remote configuration
   - Data synchronization

### Planned Features

1. **Advanced Routing:**
   - Multi-hop LoRa mesh
   - Load balancing across interfaces
   - Quality of Service (QoS) management

2. **Security:**
   - Message encryption
   - Authentication protocols
   - Secure key exchange

3. **Power Optimization:**
   - Dynamic power management
   - Sleep mode coordination
   - Battery-aware interface selection

## Status

✅ **Completed:**
- LoRa Manager with SX1262 support
- WiFi Manager with ESP32 integration
- Cellular Manager with A7682E support
- Communication Manager coordination layer
- Hardware configuration and pin assignments
- FreeRTOS task integration
- Comprehensive test suite
- Documentation and examples

🔄 **In Progress:**
- Integration testing with hardware
- Performance optimization
- Power management refinement

📋 **Next Steps:**
- Begin Phase 3: Application Framework
- Implement Meshtastic protocol integration
- Add message queuing and routing
- Develop application APIs

## Conclusion

Phase 2 successfully implements a robust, multi-interface communication stack that provides the foundation for all network operations in the T-Deck-Pro OS. The modular architecture allows for easy extension and modification while providing reliable communication across LoRa, WiFi, and Cellular interfaces.

The implementation includes comprehensive error handling, statistics collection, and automatic failover capabilities, ensuring reliable operation in various network conditions. The unified API simplifies application development while maintaining the flexibility to use specific interfaces when needed.

This communication stack is ready for integration with the Meshtastic protocol and application framework in Phase 3.