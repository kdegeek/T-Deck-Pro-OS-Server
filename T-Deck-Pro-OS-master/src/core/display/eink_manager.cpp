/**
 * @file eink_manager.cpp
 * @brief E-ink Display Manager implementation with advanced burn-in prevention
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "eink_manager.h"
#include "../hal/board_config.h"
#include "../utils/logger.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_timer.h>

// Global instance
EinkManager eink_manager;

// Static callback wrappers
static void eink_flush_wrapper(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    eink_manager.lvglFlushCallback(disp_drv, area, color_p);
}

static void eink_render_start_wrapper(struct _lv_disp_drv_t* disp_drv) {
    eink_manager.lvglRenderStartCallback(disp_drv);
}

EinkManager::EinkManager() :
    display(nullptr),
    partial_refresh_limit(50),      // Force full refresh after 50 partial updates
    full_refresh_interval(300000),  // Full refresh every 5 minutes
    clear_interval(1800000),        // Clear cycle every 30 minutes
    dirty_region_count(0),
    current_policy(EINK_POLICY_ADAPTIVE),
    current_buffer(nullptr),
    previous_buffer(nullptr),
    diff_buffer(nullptr),
    last_update_time(0),
    min_update_interval(100),       // Minimum 100ms between updates
    update_pending(false),
    lvgl_buf1(nullptr),
    lvgl_buf2(nullptr)
{
    // Initialize burn-in prevention data
    memset(&burn_in_data, 0, sizeof(burn_in_data));
    burn_in_data.needs_maintenance = false;
    
    // Initialize dirty regions
    memset(dirty_regions, 0, sizeof(dirty_regions));
}

EinkManager::~EinkManager() {
    if (current_buffer) free(current_buffer);
    if (previous_buffer) free(previous_buffer);
    if (diff_buffer) free(diff_buffer);
    if (lvgl_buf1) free(lvgl_buf1);
    if (lvgl_buf2) free(lvgl_buf2);
}

bool EinkManager::initialize() {
    LOG_INFO("Initializing E-ink Display Manager");
    
    // Initialize display hardware
    display = new GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>(
        GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)
    );
    
    if (!display) {
        LOG_ERROR("Failed to create display instance");
        return false;
    }
    
    // Initialize display with conservative settings
    display->init(115200, true, 2, false);
    display->setRotation(0);
    display->setTextColor(GxEPD_BLACK);
    
    // Initialize buffers
    if (!initializeBuffers()) {
        LOG_ERROR("Failed to initialize display buffers");
        return false;
    }
    
    // Configure LVGL
    configureLVGL();
    
    // Perform initial clear to establish baseline
    performClearCycle();
    
    LOG_INFO("E-ink Display Manager initialized successfully");
    return true;
}

void EinkManager::initializeBuffers() {
    // Allocate display buffers
    current_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    previous_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    diff_buffer = (uint8_t*)ps_malloc(EINK_BUFFER_SIZE);
    
    // Allocate LVGL buffers
    lvgl_buf1 = (lv_color_t*)ps_malloc(sizeof(lv_color_t) * EINK_WIDTH * EINK_HEIGHT);
    lvgl_buf2 = (lv_color_t*)ps_malloc(sizeof(lv_color_t) * EINK_WIDTH * EINK_HEIGHT);
    
    if (!current_buffer || !previous_buffer || !diff_buffer || !lvgl_buf1 || !lvgl_buf2) {
        LOG_ERROR("Failed to allocate display buffers");
        return false;
    }
    
    // Initialize buffers to white (0xFF for e-ink)
    memset(current_buffer, 0xFF, EINK_BUFFER_SIZE);
    memset(previous_buffer, 0xFF, EINK_BUFFER_SIZE);
    memset(diff_buffer, 0x00, EINK_BUFFER_SIZE);
    
    return true;
}

void EinkManager::configureLVGL() {
    LOG_INFO("Configuring LVGL for E-ink display");
    
    // Initialize LVGL draw buffer
    lv_disp_draw_buf_init(&lvgl_draw_buf, lvgl_buf1, lvgl_buf2, EINK_WIDTH * EINK_HEIGHT);
    
    // Initialize display driver
    lv_disp_drv_init(&lvgl_driver);
    lvgl_driver.hor_res = EINK_WIDTH;
    lvgl_driver.ver_res = EINK_HEIGHT;
    lvgl_driver.flush_cb = eink_flush_wrapper;
    lvgl_driver.render_start_cb = eink_render_start_wrapper;
    lvgl_driver.draw_buf = &lvgl_draw_buf;
    lvgl_driver.full_refresh = 0;  // We'll manage refresh strategy ourselves
    
    // Register the driver
    lv_disp_t* disp = lv_disp_drv_register(&lvgl_driver);
    if (!disp) {
        LOG_ERROR("Failed to register LVGL display driver");
        return;
    }
    
    // Set as default display
    lv_disp_set_default(disp);
    
    LOG_INFO("LVGL configured for E-ink display");
}

void EinkManager::lvglFlushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    // Convert LVGL color buffer to E-ink format
    convertLvglToEink(color_p, current_buffer, area);
    
    // Update pixel usage tracking
    updatePixelUsageMap(area);
    
    // Determine refresh strategy
    EinkRefreshMode refresh_mode = EINK_REFRESH_PARTIAL;
    
    if (shouldPerformFullRefresh()) {
        refresh_mode = EINK_REFRESH_FULL;
        LOG_DEBUG("Performing full refresh to prevent burn-in");
    }
    
    // Schedule the display update
    scheduleUpdate(area, refresh_mode);
    
    // Mark flush as ready
    lv_disp_flush_ready(disp_drv);
}

void EinkManager::lvglRenderStartCallback(struct _lv_disp_drv_t* disp_drv) {
    // Called when LVGL starts rendering
    // We can use this to batch updates or prepare for refresh
    update_pending = true;
}

void EinkManager::convertLvglToEink(const lv_color_t* color_p, uint8_t* eink_buf, const lv_area_t* area) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // Convert LVGL 1-bit color to E-ink format
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x += 8) {
            uint8_t byte_val = 0;
            
            // Pack 8 pixels into one byte
            for (int bit = 0; bit < 8 && (x + bit) < w; bit++) {
                uint32_t pixel_idx = y * w + x + bit;
                if (pixel_idx < w * h) {
                    // LVGL uses 0 for black, 1 for white
                    // E-ink uses 0 for black, 1 for white (same)
                    if ((color_p + pixel_idx)->full) {
                        byte_val |= (1 << (7 - bit));
                    }
                }
            }
            
            // Calculate buffer position
            uint32_t buf_x = area->x1 + x;
            uint32_t buf_y = area->y1 + y;
            uint32_t buf_idx = (buf_y * EINK_WIDTH + buf_x) / 8;
            
            if (buf_idx < EINK_BUFFER_SIZE) {
                eink_buf[buf_idx] = byte_val;
            }
        }
    }
}

bool EinkManager::shouldPerformFullRefresh() {
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Check partial refresh count limit
    if (burn_in_data.partial_refresh_count >= partial_refresh_limit) {
        LOG_DEBUG("Full refresh triggered by partial count limit");
        return true;
    }
    
    // Check time-based full refresh interval
    if (current_time - burn_in_data.last_full_refresh_time >= full_refresh_interval) {
        LOG_DEBUG("Full refresh triggered by time interval");
        return true;
    }
    
    // Check if maintenance cycle is needed
    if (burn_in_data.needs_maintenance) {
        LOG_DEBUG("Full refresh triggered by maintenance requirement");
        return true;
    }
    
    return false;
}

void EinkManager::scheduleUpdate(const lv_area_t* area, EinkRefreshMode mode) {
    // Optimize the refresh area to minimize unnecessary updates
    lv_area_t optimized_area = *area;
    optimizeRefreshRegion(&optimized_area);
    
    // Check minimum update interval
    uint32_t current_time = esp_timer_get_time() / 1000;
    if (current_time - last_update_time < min_update_interval) {
        // Too soon, batch this update
        calculateDirtyRegions(&optimized_area);
        return;
    }
    
    // Perform the update
    flushDisplay(&optimized_area, current_buffer, mode);
    last_update_time = current_time;
}

void EinkManager::flushDisplay(const lv_area_t* area, const uint8_t* buffer, EinkRefreshMode mode) {
    if (!display) return;
    
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    
    switch (mode) {
        case EINK_REFRESH_PARTIAL:
            display->setPartialWindow(area->x1, area->y1, w, h);
            display->firstPage();
            do {
                display->drawInvertedBitmap(area->x1, area->y1, buffer, w, h, GxEPD_BLACK);
            } while (display->nextPage());
            burn_in_data.partial_refresh_count++;
            LOG_DEBUG("Partial refresh completed");
            break;
            
        case EINK_REFRESH_FULL:
            display->setFullWindow();
            display->firstPage();
            do {
                display->drawInvertedBitmap(0, 0, buffer, EINK_WIDTH, EINK_HEIGHT, GxEPD_BLACK);
            } while (display->nextPage());
            burn_in_data.partial_refresh_count = 0;
            burn_in_data.last_full_refresh_time = esp_timer_get_time() / 1000;
            burn_in_data.needs_maintenance = false;
            LOG_DEBUG("Full refresh completed");
            break;
            
        case EINK_REFRESH_CLEAR:
            performClearCycle();
            break;
            
        case EINK_REFRESH_DEEP_CLEAN:
            performDeepClean();
            break;
    }
    
    display->hibernate();
}

void EinkManager::performClearCycle() {
    LOG_INFO("Performing E-ink clear cycle");
    
    display->setFullWindow();
    
    // Clear to white
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());
    
    delay(100);
    
    // Clear to black
    display->firstPage();
    do {
        display->fillScreen(GxEPD_BLACK);
    } while (display->nextPage());
    
    delay(100);
    
    // Final clear to white
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());
    
    // Reset burn-in tracking
    burn_in_data.last_clear_time = esp_timer_get_time() / 1000;
    burn_in_data.partial_refresh_count = 0;
    memset(burn_in_data.pixel_usage_map, 0, sizeof(burn_in_data.pixel_usage_map));
    
    display->hibernate();
    LOG_INFO("Clear cycle completed");
}

void EinkManager::performDeepClean() {
    LOG_INFO("Performing E-ink deep clean cycle");
    
    // Perform multiple clear cycles for deep cleaning
    for (int i = 0; i < 3; i++) {
        performClearCycle();
        delay(500);
    }
    
    LOG_INFO("Deep clean cycle completed");
}

void EinkManager::updatePixelUsageMap(const lv_area_t* area) {
    // Track which pixels are being used to detect potential burn-in areas
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x += 8) {
            if (y < EINK_HEIGHT && x < EINK_WIDTH) {
                burn_in_data.pixel_usage_map[y][x/8]++;
                
                // Check for excessive usage
                if (burn_in_data.pixel_usage_map[y][x/8] > 1000) {
                    burn_in_data.needs_maintenance = true;
                }
            }
        }
    }
}

void EinkManager::checkBurnInPrevention() {
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // Check if clear cycle is needed
    if (current_time - burn_in_data.last_clear_time >= clear_interval) {
        LOG_INFO("Scheduling clear cycle for burn-in prevention");
        performClearCycle();
    }
    
    // Check pixel usage patterns
    float usage_percentage = getPixelUsagePercentage();
    if (usage_percentage > 80.0f) {
        LOG_WARN("High pixel usage detected, scheduling maintenance");
        burn_in_data.needs_maintenance = true;
    }
}

float EinkManager::getPixelUsagePercentage() {
    uint32_t total_usage = 0;
    uint32_t max_possible = EINK_HEIGHT * (EINK_WIDTH / 8) * 1000; // Max usage per pixel
    
    for (int y = 0; y < EINK_HEIGHT; y++) {
        for (int x = 0; x < EINK_WIDTH / 8; x++) {
            total_usage += burn_in_data.pixel_usage_map[y][x];
        }
    }
    
    return (float)total_usage / max_possible * 100.0f;
}

void EinkManager::enterSleepMode() {
    if (display) {
        display->hibernate();
    }
    LOG_DEBUG("E-ink display entered sleep mode");
}

void EinkManager::exitSleepMode() {
    // Display will wake up automatically on next update
    LOG_DEBUG("E-ink display exiting sleep mode");
}

// Global initialization function
void eink_init() {
    if (!eink_manager.initialize()) {
        LOG_ERROR("Failed to initialize E-ink manager");
    }
}

// Task handler for periodic maintenance
void eink_task_handler() {
    eink_manager.checkBurnInPrevention();
    eink_manager.processScheduledUpdates();
}

// FreeRTOS task for maintenance
void eink_maintenance_task(void* parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(60000); // Run every minute
    
    while (1) {
        eink_task_handler();
        vTaskDelay(xDelay);
    }
}