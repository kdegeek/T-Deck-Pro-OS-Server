# T-Deck-Pro OS - E-ink Display System

A comprehensive operating system for the LilyGo T-Deck-Pro 4G device with advanced E-ink display management and burn-in prevention.

## 🖥️ E-ink Display Features

### Advanced Burn-in Prevention
- **Partial Refresh Limiting**: Automatically switches to full refresh after 50 partial updates
- **Time-based Full Refresh**: Forces full refresh every 5 minutes to prevent ghosting
- **Clear Cycles**: Performs complete display clear every 30 minutes
- **Pixel Usage Tracking**: Monitors pixel usage patterns to detect potential burn-in areas
- **Adaptive Refresh Strategy**: Intelligently chooses between partial and full refresh modes

### Display Specifications
- **Resolution**: 240x320 pixels
- **Technology**: E-ink (Electronic Paper Display)
- **Color Depth**: 1-bit (Black & White)
- **Refresh Modes**: Partial, Full, Clear, Deep Clean
- **Interface**: SPI with dedicated control pins

### LVGL Integration
- **Version**: 8.3.11
- **Memory Management**: Custom allocators with PSRAM support
- **Refresh Optimization**: Batched updates and dirty region tracking
- **Font Support**: Montserrat and Unscii fonts optimized for E-ink
- **Widget Support**: Full LVGL widget library with E-ink optimizations

## 🏗️ Architecture

### Core Components

#### E-ink Manager (`src/core/display/eink_manager.h/cpp`)
The heart of the display system, providing:
- Hardware abstraction for the E-ink display
- Burn-in prevention algorithms
- LVGL integration and callbacks
- Refresh strategy management
- Power management for display sleep/wake

#### Board Configuration (`src/core/hal/board_config.h`)
Hardware-specific definitions for the T-Deck-Pro:
- Pin mappings for all peripherals
- I2C addresses and communication settings
- Power management thresholds
- Hardware validation macros

#### Logger System (`src/core/utils/logger.h`)
Comprehensive logging with:
- Multiple log levels (ERROR, WARN, INFO, DEBUG, VERBOSE)
- Multiple destinations (Serial, File, Network, Buffer)
- Performance monitoring
- Hexdump capabilities

### Display Management Strategy

```cpp
// Refresh decision flowchart
if (partial_refresh_count >= 50) {
    perform_full_refresh();
} else if (time_since_last_full > 5_minutes) {
    perform_full_refresh();
} else if (needs_maintenance) {
    perform_full_refresh();
} else {
    perform_partial_refresh();
}
```

### Memory Management
- **Display Buffers**: Triple buffering (current, previous, diff)
- **LVGL Buffers**: Double buffering for smooth rendering
- **PSRAM Usage**: Large buffers allocated in external PSRAM
- **Buffer Size**: 9,600 bytes per buffer (240×320÷8)

## 🚀 Getting Started

### Prerequisites
- PlatformIO IDE or CLI
- LilyGo T-Deck-Pro 4G device
- USB-C cable for programming

### Building the Project

1. **Clone and Setup**:
```bash
git clone <repository-url>
cd T-Deck-Pro-OS
```

2. **Install Dependencies**:
```bash
pio lib install
```

3. **Build**:
```bash
# Debug build
pio run -e t-deck-pro-debug

# Release build
pio run -e t-deck-pro-release
```

4. **Upload**:
```bash
pio run -e t-deck-pro -t upload
```

### Configuration

#### Display Settings
Edit `src/core/display/eink_manager.h` to adjust:
```cpp
#define EINK_PARTIAL_REFRESH_LIMIT 50      // Partial refresh limit
#define EINK_FULL_REFRESH_INTERVAL 300000  // 5 minutes in ms
#define EINK_CLEAR_INTERVAL 1800000        // 30 minutes in ms
```

#### LVGL Configuration
Modify `src/lv_conf.h` for display-specific settings:
```cpp
#define LV_COLOR_DEPTH 1                   // 1-bit for E-ink
#define LV_DISP_DEF_REFR_PERIOD 100       // Slower refresh for E-ink
#define LV_MEM_CUSTOM 1                    // Use custom memory management
```

## 📱 Demo Application

The included demo application (`src/demo_app.cpp`) demonstrates:

### Features Showcased
- **Real-time Status Display**: Battery, refresh counters, pixel usage
- **Interactive Controls**: Start/stop demo functionality
- **Burn-in Prevention Info**: Live display of prevention parameters
- **Refresh Mode Testing**: Demonstrates different refresh strategies

### Demo UI Elements
```
┌─────────────────────────┐
│      T-Deck-Pro OS      │
│    E-ink Display Demo   │
├─────────────────────────┤
│ Battery: [████████░░] │
│ Status: Ready           │
│ Refreshes: 0            │
│ Pixel Usage: 0.0%       │
├─────────────────────────┤
│     [Start Demo]        │
├─────────────────────────┤
│ Burn-in Prevention:     │
│ • Partial limit: 50     │
│ • Full refresh: 5 min   │
│ • Clear cycle: 30 min   │
│ • Pixel usage tracking │
└─────────────────────────┘
```

### Running the Demo
```cpp
#include "demo_app.h"

void setup() {
    // Initialize hardware and display
    eink_manager.initialize();
    
    // Start demo application
    demo_app_init();
}
```

## 🔧 API Reference

### E-ink Manager API

#### Initialization
```cpp
bool EinkManager::initialize();
```

#### Display Control
```cpp
void flushDisplay(const lv_area_t* area, const uint8_t* buffer, EinkRefreshMode mode);
void performClearCycle();
void performDeepClean();
```

#### Burn-in Prevention
```cpp
void checkBurnInPrevention();
bool shouldPerformFullRefresh();
float getPixelUsagePercentage();
```

#### Power Management
```cpp
void enterSleepMode();
void exitSleepMode();
```

### Refresh Modes

| Mode | Description | Use Case |
|------|-------------|----------|
| `EINK_REFRESH_PARTIAL` | Fast update, some ghosting | Text updates, UI changes |
| `EINK_REFRESH_FULL` | Complete refresh, no ghosting | Image changes, periodic cleanup |
| `EINK_REFRESH_CLEAR` | Full clear cycle | Burn-in prevention |
| `EINK_REFRESH_DEEP_CLEAN` | Multiple clear cycles | Deep maintenance |

## 🛠️ Hardware Configuration

### Pin Assignments
```cpp
// E-ink Display
#define BOARD_EPD_CS    10
#define BOARD_EPD_DC    11
#define BOARD_EPD_RST   12
#define BOARD_EPD_BUSY  13
#define BOARD_EPD_SCK   14
#define BOARD_EPD_MOSI  15
```

### SPI Configuration
- **Frequency**: 4 MHz (safe for E-ink)
- **Mode**: SPI_MODE0
- **Bit Order**: MSB First
- **Shared Bus**: LoRa and E-ink share SPI bus

### Power Requirements
- **Active**: ~50mA during refresh
- **Standby**: ~1µA in hibernate
- **Refresh Current**: ~150mA peak during full refresh

## 📊 Performance Characteristics

### Refresh Times
- **Partial Refresh**: ~300ms
- **Full Refresh**: ~2000ms
- **Clear Cycle**: ~5000ms
- **Deep Clean**: ~15000ms

### Memory Usage
- **Display Buffers**: ~29KB (3 × 9.6KB)
- **LVGL Buffers**: ~19KB (2 × 9.6KB)
- **Total Display Memory**: ~48KB

### Power Consumption
- **Active UI**: ~25mA average
- **Sleep Mode**: <1mA
- **Refresh Peak**: ~150mA for 2s

## 🔍 Troubleshooting

### Common Issues

#### Display Not Updating
```cpp
// Check initialization
if (!eink_manager.initialize()) {
    LOG_ERROR("Display initialization failed");
}

// Verify LVGL integration
lv_disp_t* disp = lv_disp_get_default();
if (!disp) {
    LOG_ERROR("LVGL display not registered");
}
```

#### Ghosting/Burn-in
```cpp
// Force immediate clear cycle
eink_manager.performClearCycle();

// Check pixel usage
float usage = eink_manager.getPixelUsagePercentage();
if (usage > 80.0f) {
    LOG_WARN("High pixel usage: %.1f%%", usage);
}
```

#### Memory Issues
```cpp
// Check available memory
size_t free_heap = ESP.getFreeHeap();
if (free_heap < 50000) {
    LOG_WARN("Low memory: %d bytes", free_heap);
}
```

### Debug Configuration
Enable detailed logging in `platformio.ini`:
```ini
build_flags = 
    -DDEBUG=1
    -DCORE_DEBUG_LEVEL=5
    -DLV_LOG_LEVEL=LV_LOG_LEVEL_TRACE
    -DLOG_LEVEL=5
```

## 🚧 Development Roadmap

### Phase 1: Core Display System ✅
- [x] E-ink manager implementation
- [x] LVGL integration
- [x] Burn-in prevention
- [x] Demo application

### Phase 2: Communication Stack (In Progress)
- [ ] LoRa manager (SX1262)
- [ ] WiFi manager
- [ ] Cellular manager (A7682E)
- [ ] Bluetooth integration

### Phase 3: Application Framework
- [ ] App manager and loader
- [ ] Meshtastic application
- [ ] File manager
- [ ] Settings application

### Phase 4: Server Integration
- [ ] Docker server setup
- [ ] Tailscale VPN integration
- [ ] OTA update system
- [ ] External service APIs

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📞 Support

For issues and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the hardware documentation in `.REFERENCE/`

---

**T-Deck-Pro OS** - Building the future of portable mesh communication devices.