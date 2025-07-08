/**
 * @file board_config.h
 * @brief Hardware configuration for LilyGo T-Deck-Pro
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ===== DISPLAY PINS (E-ink) =====
#define BOARD_EPD_CS        10
#define BOARD_EPD_DC        11
#define BOARD_EPD_RST       12
#define BOARD_EPD_BUSY      13
#define BOARD_EPD_SCK       14
#define BOARD_EPD_MOSI      15

// ===== LORA PINS (SX1262) =====
#define BOARD_LORA_CS       9
#define BOARD_LORA_RST      17
#define BOARD_LORA_DIO1     45
#define BOARD_LORA_BUSY     46
#define BOARD_LORA_SCK      14  // Shared with EPD
#define BOARD_LORA_MOSI     15  // Shared with EPD
#define BOARD_LORA_MISO     16

// ===== 4G MODEM PINS (A7682E) =====
#define BOARD_MODEM_PWR     4
#define BOARD_MODEM_DTR     5
#define BOARD_MODEM_RI      6
#define BOARD_MODEM_TX      43
#define BOARD_MODEM_RX      44
#define BOARD_MODEM_PWRKEY  7

// ===== KEYBOARD MATRIX =====
#define BOARD_KB_ROW_1      1
#define BOARD_KB_ROW_2      2
#define BOARD_KB_ROW_3      3
#define BOARD_KB_ROW_4      42
#define BOARD_KB_COL_1      21
#define BOARD_KB_COL_2      47
#define BOARD_KB_COL_3      48

// ===== I2C PINS =====
#define BOARD_I2C_SDA       18
#define BOARD_I2C_SCL       8

// ===== SENSORS I2C ADDRESSES =====
#define BOARD_BME280_ADDR   0x76
#define BOARD_PCF8563_ADDR  0x51

// ===== POWER MANAGEMENT =====
#define BOARD_BAT_ADC       38
#define BOARD_VBUS_SENSE    39

// ===== AUDIO =====
#define BOARD_AUDIO_PWR     40
#define BOARD_AUDIO_OUT     41

// ===== SD CARD =====
#define BOARD_SD_CS         37
#define BOARD_SD_SCK        36
#define BOARD_SD_MOSI       35
#define BOARD_SD_MISO       34

// ===== TRACKBALL =====
#define BOARD_TB_UP         0
#define BOARD_TB_DOWN       19
#define BOARD_TB_LEFT       20
#define BOARD_TB_RIGHT      33
#define BOARD_TB_CLICK      32

// ===== DISPLAY SPECIFICATIONS =====
#define BOARD_EPD_WIDTH     240
#define BOARD_EPD_HEIGHT    320
#define BOARD_EPD_ROTATION  0

// ===== SYSTEM CONFIGURATION =====
#define BOARD_CPU_FREQ      240  // MHz
#define BOARD_FLASH_SIZE    16   // MB
#define BOARD_PSRAM_SIZE    8    // MB

// ===== POWER MANAGEMENT THRESHOLDS =====
#define BOARD_BAT_LOW_MV    3300  // Low battery threshold
#define BOARD_BAT_CRIT_MV   3000  // Critical battery threshold
#define BOARD_BAT_FULL_MV   4200  // Full battery voltage

// ===== COMMUNICATION FREQUENCIES =====
#define BOARD_LORA_FREQ     915.0  // MHz (US frequency)
#define BOARD_I2C_FREQ      400000 // 400kHz

// ===== TIMING CONSTANTS =====
#define BOARD_BOOT_DELAY_MS     1000
#define BOARD_MODEM_INIT_MS     5000
#define BOARD_EPD_INIT_MS       2000

// ===== TASK PRIORITIES =====
#define UI_TASK_PRIORITY           (configMAX_PRIORITIES - 1)
#define DISPLAY_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
#define INPUT_TASK_PRIORITY        (configMAX_PRIORITIES - 3)
#define LORA_TASK_PRIORITY         (configMAX_PRIORITIES - 4)
#define WIFI_TASK_PRIORITY         (configMAX_PRIORITIES - 5)
#define CELLULAR_TASK_PRIORITY     (configMAX_PRIORITIES - 6)
#define SYSTEM_TASK_PRIORITY       (configMAX_PRIORITIES - 7)

// ===== MEMORY ALLOCATION =====
#define BOARD_HEAP_SIZE         (200 * 1024)  // 200KB for heap
#define BOARD_STACK_SIZE        (8 * 1024)    // 8KB default stack
#define BOARD_EPD_BUFFER_SIZE   ((BOARD_EPD_WIDTH * BOARD_EPD_HEIGHT) / 8)

// ===== FEATURE ENABLES =====
#define BOARD_ENABLE_PSRAM      1
#define BOARD_ENABLE_BLUETOOTH  1
#define BOARD_ENABLE_WIFI       1
#define BOARD_ENABLE_4G         1
#define BOARD_ENABLE_LORA       1
#define BOARD_ENABLE_SENSORS    1
#define BOARD_ENABLE_AUDIO      1
#define BOARD_ENABLE_SD         1

// ===== DEBUG CONFIGURATION =====
#ifdef DEBUG
#define BOARD_SERIAL_BAUD   115200
#define BOARD_LOG_LEVEL     4  // Debug level
#else
#define BOARD_SERIAL_BAUD   115200
#define BOARD_LOG_LEVEL     2  // Warning level
#endif

// ===== HARDWARE VALIDATION MACROS =====
#define BOARD_VALIDATE_PIN(pin) ((pin) >= 0 && (pin) <= 48)
#define BOARD_VALIDATE_I2C_ADDR(addr) ((addr) >= 0x08 && (addr) <= 0x77)

// ===== UTILITY MACROS =====
#define BOARD_PIN_HIGH(pin)     digitalWrite(pin, HIGH)
#define BOARD_PIN_LOW(pin)      digitalWrite(pin, LOW)
#define BOARD_PIN_READ(pin)     digitalRead(pin)
#define BOARD_PIN_MODE(pin, mode) pinMode(pin, mode)

// ===== POWER STATES =====
typedef enum {
    BOARD_POWER_ACTIVE,
    BOARD_POWER_LIGHT_SLEEP,
    BOARD_POWER_DEEP_SLEEP,
    BOARD_POWER_HIBERNATE
} board_power_state_t;

// ===== PERIPHERAL STATES =====
typedef enum {
    BOARD_PERIPHERAL_OFF,
    BOARD_PERIPHERAL_STANDBY,
    BOARD_PERIPHERAL_ACTIVE,
    BOARD_PERIPHERAL_ERROR
} board_peripheral_state_t;

// ===== BOARD INITIALIZATION =====
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize board hardware
 * @return true if successful, false otherwise
 */
bool board_init(void);

/**
 * @brief Configure power management
 * @param state Power state to enter
 * @return true if successful, false otherwise
 */
bool board_set_power_state(board_power_state_t state);

/**
 * @brief Get battery voltage in millivolts
 * @return Battery voltage in mV
 */
uint16_t board_get_battery_voltage(void);

/**
 * @brief Check if USB power is connected
 * @return true if USB connected, false otherwise
 */
bool board_is_usb_connected(void);

#ifdef __cplusplus
}
#endif

#endif // BOARD_CONFIG_H