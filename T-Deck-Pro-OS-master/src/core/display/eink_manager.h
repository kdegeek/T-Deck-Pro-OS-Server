/**
 * @file eink_manager.h
 * @brief E-ink Display Manager with burn-in prevention and optimization
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef EINK_MANAGER_H
#define EINK_MANAGER_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "lvgl.h"

// E-ink specific configurations
#define EINK_WIDTH 240
#define EINK_HEIGHT 320
#define EINK_BUFFER_SIZE (EINK_WIDTH * EINK_HEIGHT / 8)

// Refresh strategies to prevent burn-in
enum EinkRefreshMode {
    EINK_REFRESH_PARTIAL,    // Fast partial refresh for UI updates
    EINK_REFRESH_FULL,       // Full refresh to prevent ghosting
    EINK_REFRESH_CLEAR,      // Clear screen to prevent burn-in
    EINK_REFRESH_DEEP_CLEAN  // Deep clean cycle for maintenance
};

// Display update policies
enum EinkUpdatePolicy {
    EINK_POLICY_IMMEDIATE,   // Update immediately (for critical UI)
    EINK_POLICY_BATCHED,     // Batch updates to reduce flicker
    EINK_POLICY_SCHEDULED,   // Schedule updates based on content
    EINK_POLICY_ADAPTIVE     // Adaptive based on usage patterns
};

// Burn-in prevention strategies
struct EinkBurnInPrevention {
    uint32_t partial_refresh_count;     // Track partial refreshes
    uint32_t last_full_refresh_time;    // Last full refresh timestamp
    uint32_t last_clear_time;           // Last clear operation
    uint32_t pixel_usage_map[EINK_HEIGHT][EINK_WIDTH/8]; // Track pixel usage
    bool needs_maintenance;             // Flag for maintenance cycle
};

// Display region tracking for optimization
struct EinkRegion {
    int16_t x, y, width, height;
    bool dirty;
    uint32_t last_update;
    uint8_t update_count;
};

class EinkManager {
private:
    GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>* display;
    
    // Burn-in prevention
    EinkBurnInPrevention burn_in_data;
    uint32_t partial_refresh_limit;
    uint32_t full_refresh_interval;
    uint32_t clear_interval;
    
    // Update optimization
    EinkRegion dirty_regions[16];
    uint8_t dirty_region_count;
    EinkUpdatePolicy current_policy;
    
    // Buffers for optimization
    uint8_t* current_buffer;
    uint8_t* previous_buffer;
    uint8_t* diff_buffer;
    
    // Timing control
    uint32_t last_update_time;
    uint32_t min_update_interval;
    bool update_pending;
    
    // LVGL integration
    lv_disp_drv_t lvgl_driver;
    lv_disp_draw_buf_t lvgl_draw_buf;
    lv_color_t* lvgl_buf1;
    lv_color_t* lvgl_buf2;
    
    // Private methods
    void initializeBuffers();
    void calculateDirtyRegions(const lv_area_t* area);
    bool shouldPerformFullRefresh();
    void performMaintenanceCycle();
    void updatePixelUsageMap(const lv_area_t* area);
    void optimizeRefreshRegion(lv_area_t* area);
    void convertLvglToEink(const lv_color_t* color_p, uint8_t* eink_buf, const lv_area_t* area);
    
public:
    EinkManager();
    ~EinkManager();
    
    // Initialization
    bool initialize();
    void configureLVGL();
    
    // Display control
    void setRefreshMode(EinkRefreshMode mode);
    void setUpdatePolicy(EinkUpdatePolicy policy);
    void forceFullRefresh();
    void performClearCycle();
    void performDeepClean();
    
    // LVGL callbacks
    static void lvglFlushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static void lvglRenderStartCallback(struct _lv_disp_drv_t* disp_drv);
    
    // Update management
    void scheduleUpdate(const lv_area_t* area, EinkRefreshMode mode = EINK_REFRESH_PARTIAL);
    void processScheduledUpdates();
    void flushDisplay(const lv_area_t* area, const uint8_t* buffer, EinkRefreshMode mode);
    
    // Burn-in prevention
    void checkBurnInPrevention();
    void resetBurnInCounters();
    uint32_t getPartialRefreshCount() const { return burn_in_data.partial_refresh_count; }
    bool needsMaintenanceCycle() const { return burn_in_data.needs_maintenance; }
    
    // Power management
    void enterSleepMode();
    void exitSleepMode();
    void hibernate();
    
    // Statistics and monitoring
    void getDisplayStats(uint32_t* partial_count, uint32_t* full_count, uint32_t* clear_count);
    float getPixelUsagePercentage();
    uint32_t getTimeSinceLastFullRefresh();
    
    // Configuration
    void setPartialRefreshLimit(uint32_t limit) { partial_refresh_limit = limit; }
    void setFullRefreshInterval(uint32_t interval) { full_refresh_interval = interval; }
    void setClearInterval(uint32_t interval) { clear_interval = interval; }
    void setMinUpdateInterval(uint32_t interval) { min_update_interval = interval; }
};

// Global instance
extern EinkManager eink_manager;

// Utility functions
void eink_init();
void eink_task_handler();
void eink_maintenance_task(void* parameter);

#endif // EINK_MANAGER_H