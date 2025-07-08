/**
 * @file logger.h
 * @brief Logging system for T-Deck-Pro OS
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdarg.h>

// ===== LOG LEVELS =====
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_VERBOSE = 5
} log_level_t;

// ===== LOG DESTINATIONS =====
typedef enum {
    LOG_DEST_SERIAL = (1 << 0),
    LOG_DEST_FILE = (1 << 1),
    LOG_DEST_NETWORK = (1 << 2),
    LOG_DEST_BUFFER = (1 << 3)
} log_destination_t;

// ===== CONFIGURATION =====
#ifndef LOG_MAX_MESSAGE_SIZE
#define LOG_MAX_MESSAGE_SIZE 256
#endif

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 4096
#endif

#ifndef LOG_MAX_TAG_SIZE
#define LOG_MAX_TAG_SIZE 16
#endif

// ===== DEFAULT LOG LEVEL =====
#ifdef DEBUG
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG
#else
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#endif

// ===== LOG MACROS =====
#define LOG_ERROR(format, ...) log_write(LOG_LEVEL_ERROR, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  log_write(LOG_LEVEL_WARN, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  log_write(LOG_LEVEL_INFO, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) log_write(LOG_LEVEL_DEBUG, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_VERBOSE(format, ...) log_write(LOG_LEVEL_VERBOSE, __func__, __LINE__, format, ##__VA_ARGS__)

// ===== TAGGED LOG MACROS =====
#define LOG_ERROR_TAG(tag, format, ...) log_write_tag(LOG_LEVEL_ERROR, tag, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN_TAG(tag, format, ...)  log_write_tag(LOG_LEVEL_WARN, tag, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO_TAG(tag, format, ...)  log_write_tag(LOG_LEVEL_INFO, tag, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG_TAG(tag, format, ...) log_write_tag(LOG_LEVEL_DEBUG, tag, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_VERBOSE_TAG(tag, format, ...) log_write_tag(LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, format, ##__VA_ARGS__)

// ===== CONDITIONAL LOGGING =====
#define LOG_IF(condition, level, format, ...) \
    do { if (condition) log_write(level, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)

// ===== HEXDUMP LOGGING =====
#define LOG_HEXDUMP(level, data, length, format, ...) \
    log_hexdump(level, __func__, __LINE__, data, length, format, ##__VA_ARGS__)

// ===== LOG ENTRY STRUCTURE =====
typedef struct {
    uint32_t timestamp;
    log_level_t level;
    char tag[LOG_MAX_TAG_SIZE];
    char function[32];
    uint16_t line;
    char message[LOG_MAX_MESSAGE_SIZE];
} log_entry_t;

// ===== LOG CONFIGURATION STRUCTURE =====
typedef struct {
    log_level_t level;
    uint8_t destinations;
    bool include_timestamp;
    bool include_function;
    bool include_line_number;
    bool color_output;
    const char* log_file_path;
    uint16_t buffer_size;
} log_config_t;

#ifdef __cplusplus
extern "C" {
#endif

// ===== CORE LOGGING FUNCTIONS =====

/**
 * @brief Initialize the logging system
 * @param config Logging configuration
 * @return true if successful, false otherwise
 */
bool log_init(const log_config_t* config);

/**
 * @brief Deinitialize the logging system
 */
void log_deinit(void);

/**
 * @brief Write a log message
 * @param level Log level
 * @param function Function name
 * @param line Line number
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void log_write(log_level_t level, const char* function, uint16_t line, const char* format, ...);

/**
 * @brief Write a tagged log message
 * @param level Log level
 * @param tag Log tag
 * @param function Function name
 * @param line Line number
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void log_write_tag(log_level_t level, const char* tag, const char* function, uint16_t line, const char* format, ...);

/**
 * @brief Write a log message with variable arguments
 * @param level Log level
 * @param function Function name
 * @param line Line number
 * @param format Printf-style format string
 * @param args Variable argument list
 */
void log_write_va(log_level_t level, const char* function, uint16_t line, const char* format, va_list args);

/**
 * @brief Log hexdump of data
 * @param level Log level
 * @param function Function name
 * @param line Line number
 * @param data Data to dump
 * @param length Length of data
 * @param format Printf-style format string for header
 * @param ... Variable arguments for header
 */
void log_hexdump(log_level_t level, const char* function, uint16_t line, const void* data, size_t length, const char* format, ...);

// ===== CONFIGURATION FUNCTIONS =====

/**
 * @brief Set the global log level
 * @param level New log level
 */
void log_set_level(log_level_t level);

/**
 * @brief Get the current log level
 * @return Current log level
 */
log_level_t log_get_level(void);

/**
 * @brief Set log destinations
 * @param destinations Bitfield of log destinations
 */
void log_set_destinations(uint8_t destinations);

/**
 * @brief Enable or disable colored output
 * @param enable true to enable colors, false to disable
 */
void log_set_color_output(bool enable);

/**
 * @brief Set log file path
 * @param path Path to log file (NULL to disable file logging)
 */
void log_set_file_path(const char* path);

// ===== UTILITY FUNCTIONS =====

/**
 * @brief Get log level name as string
 * @param level Log level
 * @return String representation of log level
 */
const char* log_level_to_string(log_level_t level);

/**
 * @brief Get log level color code for terminal output
 * @param level Log level
 * @return ANSI color code string
 */
const char* log_level_to_color(log_level_t level);

/**
 * @brief Flush all pending log messages
 */
void log_flush(void);

/**
 * @brief Get current timestamp for logging
 * @return Timestamp in milliseconds
 */
uint32_t log_get_timestamp(void);

// ===== BUFFER MANAGEMENT =====

/**
 * @brief Get buffered log entries
 * @param entries Buffer to store entries
 * @param max_entries Maximum number of entries to retrieve
 * @return Number of entries retrieved
 */
uint16_t log_get_buffered_entries(log_entry_t* entries, uint16_t max_entries);

/**
 * @brief Clear the log buffer
 */
void log_clear_buffer(void);

/**
 * @brief Get number of entries in buffer
 * @return Number of buffered entries
 */
uint16_t log_get_buffer_count(void);

// ===== PERFORMANCE MONITORING =====

/**
 * @brief Start a performance timer
 * @param tag Timer tag for identification
 * @return Timer ID
 */
uint32_t log_perf_start(const char* tag);

/**
 * @brief End a performance timer and log the duration
 * @param timer_id Timer ID from log_perf_start
 */
void log_perf_end(uint32_t timer_id);

/**
 * @brief Log a performance measurement
 * @param tag Performance tag
 * @param duration_ms Duration in milliseconds
 */
void log_perf_measure(const char* tag, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

// ===== COMPILE-TIME LOG LEVEL FILTERING =====
#if LOG_LEVEL < LOG_LEVEL_ERROR
#undef LOG_ERROR
#define LOG_ERROR(format, ...) do {} while(0)
#endif

#if LOG_LEVEL < LOG_LEVEL_WARN
#undef LOG_WARN
#define LOG_WARN(format, ...) do {} while(0)
#endif

#if LOG_LEVEL < LOG_LEVEL_INFO
#undef LOG_INFO
#define LOG_INFO(format, ...) do {} while(0)
#endif

#if LOG_LEVEL < LOG_LEVEL_DEBUG
#undef LOG_DEBUG
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

#if LOG_LEVEL < LOG_LEVEL_VERBOSE
#undef LOG_VERBOSE
#define LOG_VERBOSE(format, ...) do {} while(0)
#endif

#endif // LOGGER_H