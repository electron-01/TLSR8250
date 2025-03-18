/**
 * @file app_key.c
 * @brief This is app_key file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "board.h"

#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"
#include "tal_rtc.h"
#include "tal_key.h"
#include "tal_sdk_test.h"

#include "tuya_ble_api.h"
#include "tuya_ble_protocol_callback.h"

#include "app_config.h"

#include "app_misc.h"
#include "app_dp_reporter.h"

#if ENABLE_LED
#include "app_led.h"
#endif

#if ENABLE_KEY
#include "app_key.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
typedef enum {
    KEY_FREE = 0,
    SHORT_PRESS_ACTIVE = 1,
    LONG_PRESS_ACTIVE  = 2,
    LONG_LONG_PRESS_ACTIVE = 3,
    SHORT_PRESS_RELEASE = 5,
    LONG_PRESS_RELEASE = 6,
} APP_KEY_STATUE_E;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC VOID_T app_key_handler(UINT32_T state);
STATIC TIMER_ID app_key_timer_id = NULL;
STATIC tal_key_param_t key_press_param = {
    .pin = APP_KEY_PIN,
    .valid_level = TUYA_KEY_LEVEL_LOW,
    .count_len = 3,
    .count_array = {5, 300, 1000},
    .handler = app_key_handler,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_key_handler(UINT32_T state)
{
   TAL_PR_INFO("Key state: %d", state);
    switch (state) {
        case SHORT_PRESS_ACTIVE: {
            if (app_status_info.enter_pairing == 0) {
#if ENABLE_LED
                app_led_start(APP_LED_KEY_PRESS);
#endif
            }
        } break;
        case LONG_PRESS_ACTIVE: {
            tuya_ble_device_unbond();
#if ENABLE_LED
            app_led_start(APP_LED_PAIRING_START);
#endif
            app_pairing_start();
        } break;
        case LONG_LONG_PRESS_ACTIVE: {
            app_config_param_reset();
            tuya_ble_device_factory_reset();
            tuya_ble_disconnect_and_reset_timer_start();
        } break;
        case SHORT_PRESS_RELEASE: {
            app_dp_reporter_event_set(APP_REPORT_ALL);

            if (app_status_info.enter_pairing == 0) {
#if ENABLE_LED
                app_led_stop(APP_LED_KEY_PRESS);
#endif
            }
        } break;
        case LONG_PRESS_RELEASE: {
            if (app_status_info.enter_pairing == 0) {
#if ENABLE_LED
                app_led_stop(APP_LED_KEY_PRESS);
#endif
            }
        } break;

        default: {
        } break;
    }
}

UINT32_T tal_key_get_pin_level(UINT32_T pin)
{
    // Weak function instance
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tal_gpio_read(pin, &level);
    return level;
}

STATIC VOID_T app_key_irq_cb(VOID_T *args)
{
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tal_gpio_read(APP_KEY_PIN, &level);

    if (level == APP_KEY_ACTIVE_LEVEL) {
        tal_sw_timer_start(app_key_timer_id, 10, TAL_TIMER_CYCLE);
    }
}

STATIC VOID_T app_key_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    if (!tal_key_timeout_handler(&key_press_param)) {
        tal_sw_timer_stop(app_key_timer_id);
    }
}

VOID_T app_key_sw_init(VOID_T)
{
    tal_sw_timer_create(app_key_timeout_handler, NULL, &app_key_timer_id);
}

VOID_T app_key_hw_init(VOID_T)
{
    TUYA_GPIO_BASE_CFG_T gpio_cfg = {
        .mode = TUYA_GPIO_PULLUP,
        .direct = TUYA_GPIO_INPUT,
        .level = APP_KEY_ACTIVE_LEVEL,
    };
    TUYA_GPIO_IRQ_T gpio_irq = {
        .mode = TUYA_GPIO_IRQ_RISE_FALL,
        .cb = app_key_irq_cb,
        .arg = NULL,
    };

    TUYA_WAKEUP_SOURCE_BASE_CFG_T wakeup_cfg = {
        .source = TUYA_WAKEUP_SOURCE_GPIO,
        .wakeup_para.gpio_param.gpio_num = APP_KEY_PIN,
        .wakeup_para.gpio_param.level = APP_KEY_ACTIVE_LEVEL,
    };

    tal_gpio_init(APP_KEY_PIN, &gpio_cfg);
    tal_gpio_irq_init(APP_KEY_PIN, &gpio_irq);
    tal_gpio_irq_enable(APP_KEY_PIN);

    tal_key_init(&key_press_param);

    tkl_wakeup_source_set(&wakeup_cfg);
}

#endif
