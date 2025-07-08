/**
 * @file demo_app.h
 * @brief E-ink Display Demo Application Header
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef DEMO_APP_H
#define DEMO_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the demo application
 */
void demo_app_init(void);

/**
 * @brief Get the main screen object
 * @return Pointer to the main screen LVGL object
 */
lv_obj_t* demo_app_get_main_screen(void);

/**
 * @brief Check if demo is currently running
 * @return true if demo is running, false otherwise
 */
bool demo_app_is_running(void);

/**
 * @brief Get demo counter value
 * @return Current demo counter value
 */
uint32_t demo_app_get_counter(void);

/**
 * @brief Force a status update
 */
void demo_app_update_status(void);

/**
 * @brief Demonstrate different refresh modes
 */
void demo_app_test_refresh_modes(void);

#ifdef __cplusplus
}
#endif

#endif // DEMO_APP_H