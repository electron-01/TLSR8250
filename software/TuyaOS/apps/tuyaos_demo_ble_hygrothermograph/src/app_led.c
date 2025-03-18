/**
 * @file app_led.c
 * @brief This is app_led file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"

#include "tuya_ble_api.h"
#include "tuya_ble_protocol_callback.h"

#if ENABLE_LED
#include "app_led.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TIMER_ID app_led_timer_id    = NULL;
STATIC UINT8_T app_led_status       = 0;
STATIC UINT32_T app_led_blink_count = 0;
TUYA_GPIO_BASE_CFG_T led_pin_cfg = {
    .mode   = TUYA_GPIO_PUSH_PULL,
    .direct = TUYA_GPIO_OUTPUT,
    .level  = TUYA_GPIO_LEVEL_LOW,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_led_ctrl(UINT8_T on)
{
    OPERATE_RET ret = OPRT_OK;

    tal_cpu_force_wakeup();
#if (APP_LED_ACTIVE_LEVEL)
    if (on) {
        led_pin_cfg.mode   = TUYA_GPIO_PULLUP;
        led_pin_cfg.direct = TUYA_GPIO_OUTPUT;
        led_pin_cfg.level  = TUYA_GPIO_LEVEL_HIGH;
    } else {
        led_pin_cfg.mode   = TUYA_GPIO_PULLDOWN;
        led_pin_cfg.direct = TUYA_GPIO_OUTPUT;
        led_pin_cfg.level  = TUYA_GPIO_LEVEL_LOW;
    }
#else
    if (on) {
        led_pin_cfg.mode   = TUYA_GPIO_PULLDOWN;
        led_pin_cfg.direct = TUYA_GPIO_OUTPUT;
        led_pin_cfg.level  = TUYA_GPIO_LEVEL_LOW;
    } else {
        led_pin_cfg.mode   = TUYA_GPIO_PULLUP;
        led_pin_cfg.direct = TUYA_GPIO_OUTPUT;
        led_pin_cfg.level  = TUYA_GPIO_LEVEL_HIGH;
    }
#endif

    ret = tal_gpio_init(APP_LED_PIN, &led_pin_cfg);
    if (ret) {
        TAL_PR_ERR("led ctrl err: %d", ret);
    }
}

STATIC VOID_T app_led_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    OPERATE_RET ret = OPRT_OK;

    app_led_blink_count--;
    if (app_led_blink_count == 0) {
        ret = tal_sw_timer_stop(app_led_timer_id);
        if (ret != OPRT_OK) {
            TAL_PR_ERR("led timer stop err: %d", ret);
        }
        app_led_status = 0;
        app_led_ctrl(app_led_status);
        if (tuya_ble_internal_production_test_with_ble_flag_get() == 0) {
            tal_cpu_allow_sleep();
        }
    } else {
        app_led_status ^= 1;
        app_led_ctrl(app_led_status);
    }
}

OPERATE_RET app_led_hw_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

#if (APP_LED_ACTIVE_LEVEL)
    led_pin_cfg.mode   = TUYA_GPIO_PULLDOWN;
    led_pin_cfg.direct = TUYA_GPIO_INPUT;
    led_pin_cfg.level  = TUYA_GPIO_LEVEL_LOW;
#else
    led_pin_cfg.mode   = TUYA_GPIO_PULLUP;
    led_pin_cfg.direct = TUYA_GPIO_INPUT;
    led_pin_cfg.level  = TUYA_GPIO_LEVEL_HIGH;
#endif
    ret = tal_gpio_init(APP_LED_PIN, &led_pin_cfg);
    if (ret) {
        TAL_PR_ERR("led hw init err: %d", ret);
    }

    return ret;
}

OPERATE_RET app_led_sw_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    ret = tal_sw_timer_create(app_led_timeout_handler, &app_led_blink_count, &app_led_timer_id);
    if (ret) {
        TAL_PR_ERR("led sw init err: %d", ret);
    }

    return ret;
}

OPERATE_RET app_led_stop(APP_LED_ACTION_E action)
{

    app_led_status = 0;
    app_led_ctrl(0);
    if (tuya_ble_internal_production_test_with_ble_flag_get() == 0) {
        tal_cpu_allow_sleep();
    }

    if (action != APP_LED_KEY_PRESS) {
        tal_sw_timer_stop(app_led_timer_id);
    }

    return OPRT_OK;
}

OPERATE_RET app_led_start(APP_LED_ACTION_E action)
{
    OPERATE_RET ret = OPRT_OK;

    app_led_status = 1;
    app_led_ctrl(app_led_status);

    UINT32_T interval = 0;
    switch (action) {
        case APP_LED_POWER_ON: {
            interval = 3000;
            app_led_blink_count = 1;
        } break;
        case APP_LED_KEY_PRESS: {

        } break;
        case APP_LED_PAIRING_SUCCESS: {
            interval = 3000;
            app_led_blink_count = 1;
        } break;
        case APP_LED_PAIRING_START: {
            interval = 250;
            app_led_blink_count = 0xFFFFFFFF;
        } break;
        case APP_LED_PRODUCTION: {
            interval = 500;
            app_led_blink_count = 0xFFFFFFFF;
        } break;
        default: {

        } break;
    }

    if (action != APP_LED_KEY_PRESS) {
        ret = tal_sw_timer_start(app_led_timer_id, interval, TAL_TIMER_CYCLE);
        if (ret) {
            TAL_PR_ERR("led start err: %d", ret);
        }
    }

    return ret;
}

#endif
