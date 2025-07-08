/**
 * @file demo_app.cpp
 * @brief E-ink Display Demo Application
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "demo_app.h"
#include "core/display/eink_manager.h"
#include "core/utils/logger.h"
#include "lvgl.h"

// ===== DEMO UI ELEMENTS =====
static lv_obj_t* main_screen = NULL;
static lv_obj_t* status_label = NULL;
static lv_obj_t* battery_bar = NULL;
static lv_obj_t* refresh_counter_label = NULL;
static lv_obj_t* pixel_usage_label = NULL;
static lv_obj_t* demo_button = NULL;

// ===== DEMO STATE =====
static uint32_t demo_counter = 0;
static bool demo_running = false;

// ===== FUNCTION DECLARATIONS =====
static void create_main_ui(void);
static void update_status_display(void);
static void demo_button_event_cb(lv_event_t* e);
static void demo_timer_cb(lv_timer_t* timer);

/**
 * @brief Initialize the demo application
 */
void demo_app_init(void) {
    LOG_INFO("Initializing E-ink Demo Application");
    
    // Create the main UI
    create_main_ui();
    
    // Create a timer for periodic updates
    lv_timer_t* demo_timer = lv_timer_create(demo_timer_cb, 5000, NULL);
    lv_timer_set_repeat_count(demo_timer, -1); // Repeat indefinitely
    
    LOG_INFO("E-ink Demo Application initialized");
}

/**
 * @brief Create the main user interface
 */
static void create_main_ui(void) {
    // Create main screen
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_white(), 0);
    lv_scr_load(main_screen);
    
    // Title label
    lv_obj_t* title_label = lv_label_create(main_screen);
    lv_label_set_text(title_label, "T-Deck-Pro OS");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // Subtitle
    lv_obj_t* subtitle_label = lv_label_create(main_screen);
    lv_label_set_text(subtitle_label, "E-ink Display Demo");
    lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_12, 0);
    lv_obj_align(subtitle_label, LV_ALIGN_TOP_MID, 0, 35);
    
    // Status section
    lv_obj_t* status_container = lv_obj_create(main_screen);
    lv_obj_set_size(status_container, 220, 80);
    lv_obj_align(status_container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_width(status_container, 1, 0);
    lv_obj_set_style_border_color(status_container, lv_color_black(), 0);
    
    // Battery indicator
    lv_obj_t* battery_label = lv_label_create(status_container);
    lv_label_set_text(battery_label, "Battery:");
    lv_obj_align(battery_label, LV_ALIGN_TOP_LEFT, 5, 5);
    
    battery_bar = lv_bar_create(status_container);
    lv_obj_set_size(battery_bar, 100, 10);
    lv_obj_align(battery_bar, LV_ALIGN_TOP_LEFT, 60, 8);
    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, 85, LV_ANIM_OFF); // Example value
    
    // Status text
    status_label = lv_label_create(status_container);
    lv_label_set_text(status_label, "Status: Ready");
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 5, 25);
    
    // Refresh counter
    refresh_counter_label = lv_label_create(status_container);
    lv_label_set_text(refresh_counter_label, "Refreshes: 0");
    lv_obj_align(refresh_counter_label, LV_ALIGN_TOP_LEFT, 5, 45);
    
    // Pixel usage indicator
    pixel_usage_label = lv_label_create(status_container);
    lv_label_set_text(pixel_usage_label, "Pixel Usage: 0.0%");
    lv_obj_align(pixel_usage_label, LV_ALIGN_TOP_LEFT, 5, 65);
    
    // Demo controls
    lv_obj_t* controls_container = lv_obj_create(main_screen);
    lv_obj_set_size(controls_container, 220, 60);
    lv_obj_align(controls_container, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_border_width(controls_container, 1, 0);
    lv_obj_set_style_border_color(controls_container, lv_color_black(), 0);
    
    // Demo button
    demo_button = lv_btn_create(controls_container);
    lv_obj_set_size(demo_button, 100, 30);
    lv_obj_align(demo_button, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(demo_button, demo_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* btn_label = lv_label_create(demo_button);
    lv_label_set_text(btn_label, "Start Demo");
    lv_obj_center(btn_label);
    
    // Information section
    lv_obj_t* info_container = lv_obj_create(main_screen);
    lv_obj_set_size(info_container, 220, 80);
    lv_obj_align(info_container, LV_ALIGN_TOP_MID, 0, 220);
    lv_obj_set_style_border_width(info_container, 1, 0);
    lv_obj_set_style_border_color(info_container, lv_color_black(), 0);
    
    lv_obj_t* info_title = lv_label_create(info_container);
    lv_label_set_text(info_title, "Burn-in Prevention:");
    lv_obj_set_style_text_font(info_title, &lv_font_montserrat_12, 0);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 5, 5);
    
    lv_obj_t* info_text = lv_label_create(info_container);
    lv_label_set_text(info_text, 
        "• Partial refresh limit: 50\n"
        "• Full refresh: 5 min\n"
        "• Clear cycle: 30 min\n"
        "• Pixel usage tracking");
    lv_obj_set_style_text_font(info_text, &lv_font_unscii_8, 0);
    lv_obj_align(info_text, LV_ALIGN_TOP_LEFT, 5, 25);
    
    LOG_INFO("Main UI created successfully");
}

/**
 * @brief Update the status display with current information
 */
static void update_status_display(void) {
    if (!status_label || !refresh_counter_label || !pixel_usage_label) {
        return;
    }
    
    // Update status
    if (demo_running) {
        lv_label_set_text_fmt(status_label, "Status: Demo Running (%lu)", demo_counter);
    } else {
        lv_label_set_text(status_label, "Status: Ready");
    }
    
    // Update refresh counter (simulated)
    static uint32_t refresh_count = 0;
    refresh_count++;
    lv_label_set_text_fmt(refresh_counter_label, "Refreshes: %lu", refresh_count);
    
    // Update pixel usage (get from E-ink manager)
    float pixel_usage = eink_manager.getPixelUsagePercentage();
    lv_label_set_text_fmt(pixel_usage_label, "Pixel Usage: %.1f%%", pixel_usage);
    
    // Update battery (simulated)
    static int battery_level = 85;
    battery_level = (battery_level > 20) ? battery_level - 1 : 100; // Simulate discharge/charge
    lv_bar_set_value(battery_bar, battery_level, LV_ANIM_OFF);
    
    LOG_DEBUG("Status display updated - Demo: %s, Refreshes: %lu, Pixel Usage: %.1f%%", 
              demo_running ? "Running" : "Stopped", refresh_count, pixel_usage);
}

/**
 * @brief Handle demo button events
 */
static void demo_button_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        demo_running = !demo_running;
        
        lv_obj_t* btn_label = lv_obj_get_child(demo_button, 0);
        if (demo_running) {
            lv_label_set_text(btn_label, "Stop Demo");
            demo_counter = 0;
            LOG_INFO("Demo started");
        } else {
            lv_label_set_text(btn_label, "Start Demo");
            LOG_INFO("Demo stopped");
        }
        
        update_status_display();
    }
}

/**
 * @brief Timer callback for periodic demo updates
 */
static void demo_timer_cb(lv_timer_t* timer) {
    (void)timer; // Unused parameter
    
    if (demo_running) {
        demo_counter++;
        
        // Simulate some activity that would affect pixel usage
        if (demo_counter % 3 == 0) {
            // Create a temporary visual element to demonstrate refresh
            lv_obj_t* temp_obj = lv_obj_create(main_screen);
            lv_obj_set_size(temp_obj, 20, 20);
            lv_obj_set_pos(temp_obj, 
                          (demo_counter * 23) % 200 + 10, 
                          (demo_counter * 17) % 50 + 270);
            lv_obj_set_style_bg_color(temp_obj, lv_color_black(), 0);
            
            // Remove it after a short delay to demonstrate partial refresh
            lv_timer_t* cleanup_timer = lv_timer_create([](lv_timer_t* t) {
                lv_obj_t* obj = (lv_obj_t*)lv_timer_get_user_data(t);
                if (obj) {
                    lv_obj_del(obj);
                }
                lv_timer_del(t);
            }, 2000, temp_obj);
            lv_timer_set_repeat_count(cleanup_timer, 1);
        }
        
        // Trigger burn-in prevention check
        eink_manager.checkBurnInPrevention();
    }
    
    // Always update status display
    update_status_display();
}

/**
 * @brief Get the main screen object
 */
lv_obj_t* demo_app_get_main_screen(void) {
    return main_screen;
}

/**
 * @brief Check if demo is currently running
 */
bool demo_app_is_running(void) {
    return demo_running;
}

/**
 * @brief Get demo counter value
 */
uint32_t demo_app_get_counter(void) {
    return demo_counter;
}

/**
 * @brief Force a status update
 */
void demo_app_update_status(void) {
    update_status_display();
}

/**
 * @brief Demonstrate different refresh modes
 */
void demo_app_test_refresh_modes(void) {
    LOG_INFO("Testing E-ink refresh modes");
    
    // Create test pattern
    lv_obj_t* test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(test_screen, lv_color_white(), 0);
    
    // Add test pattern
    for (int i = 0; i < 5; i++) {
        lv_obj_t* rect = lv_obj_create(test_screen);
        lv_obj_set_size(rect, 40, 40);
        lv_obj_set_pos(rect, i * 45 + 10, 100);
        lv_obj_set_style_bg_color(rect, (i % 2) ? lv_color_black() : lv_color_white(), 0);
        lv_obj_set_style_border_width(rect, 1, 0);
    }
    
    // Load test screen
    lv_scr_load(test_screen);
    
    // Schedule return to main screen
    lv_timer_t* return_timer = lv_timer_create([](lv_timer_t* t) {
        lv_scr_load(main_screen);
        lv_timer_del(t);
        LOG_INFO("Returned to main screen");
    }, 5000, NULL);
    lv_timer_set_repeat_count(return_timer, 1);
    
    LOG_INFO("Test pattern displayed for 5 seconds");
}