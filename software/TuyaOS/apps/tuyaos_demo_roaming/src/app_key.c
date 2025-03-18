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
#include "tal_oled.h"

#include "tuya_ble_api.h"
#include "tuya_ble_protocol_callback.h"
#include "tuya_sdk_callback.h"

#include "app_roaming.h"
#include "app_dp_parser.h"
#include "app_key.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define APP_KEY_PIN BOARD_KEY_PIN
#define APP_KEY_PIN2    TUYA_GPIO_NUM_13

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TIMER_ID app_key_timer_id = NULL;
STATIC TIMER_ID app_key_timer_id2 = NULL;

STATIC VOID_T app_key_handler(UINT32_T state);
STATIC VOID_T app_key_handler2(UINT32_T state);

STATIC tal_key_param_t key_press_param = {
    .pin = APP_KEY_PIN,
    .valid_level = TUYA_KEY_LEVEL_LOW,
    .count_len = 3,
    .count_array = {5, 300, 500},
    .handler = app_key_handler,
};
STATIC tal_key_param_t key_press_param2 = {
    .pin = APP_KEY_PIN2,
    .valid_level = TUYA_KEY_LEVEL_LOW,
    .count_len = 3,
    .count_array = {5, 300, 500},
    .handler = app_key_handler2,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




STATIC VOID_T app_key_handler(UINT32_T state)
{
//    TAL_PR_INFO("Key state: %d", state);
    switch (state) {
        //Short press
        case 1: {
            g_dp_value.led++;
            if (g_dp_value.led == 2) {
                g_dp_value.led = 0;
            }

            g_dp_value.charge_state++;
            if (g_dp_value.charge_state == 3) {
                g_dp_value.charge_state = 0;
            }

            g_dp_value.temperature++;
            if (g_dp_value.temperature == 1001) {
                g_dp_value.temperature = 0;
            }

            g_dp_value.welcome[8]++;
            if (g_dp_value.welcome[8] == '9') {
                g_dp_value.welcome[8] = '0';
            }

            g_dp_value.custom_data[4]++;
            if (g_dp_value.custom_data[4] == 9) {
                g_dp_value.custom_data[4] = 0;
            }

            g_dp_value.fault_alarm_count++;
            if (g_dp_value.fault_alarm_count == 3) {
                g_dp_value.fault_alarm_count = 0;
            }
            g_dp_value.fault_alarm = 1<<g_dp_value.fault_alarm_count;

            g_dp_report_count = 0;

            VOID_T tal_ble_beacon_cb(VOID_T *arg);
            tal_ble_beacon_cb(NULL);

//#if defined(TUYA_SDK_TEST) && (TUYA_SDK_TEST == 1)
//            tal_ble_sdk_test_wake_up_handler();
//#endif
        } break;

        //Long press
        case 2: {
            tuya_ble_device_factory_reset();
            tuya_ble_disconnect_and_reset_timer_start();
        } break;

        //Long long press timeout
        case 3: {
        } break;

        //Short press release
        case 5: {
        } break;

        //Long press release
        case 6: {
        } break;

        default: {
        } break;
    }
}

STATIC VOID_T app_key_handler2(UINT32_T state)
{
//    TAL_PR_INFO("Key state2: %d", state);
    switch (state) {
        //Short press
        case 1: {
            switch (g_roaming_param.adv_interval) {
                case 100: {
                    g_roaming_param.adv_interval = 200;
                } break;

                case 200: {
                    g_roaming_param.adv_interval = 500;
                } break;

                case 500: {
                    g_roaming_param.adv_interval = 1000;
                } break;

                case 1000: {
                    g_roaming_param.adv_interval = 100;
                } break;

                default: {
                    g_roaming_param.adv_interval = 1000;
                } break;
            }

            tal_oled_show_num(56, 2, g_roaming_param.adv_interval, 4, 16);

            tal_adv_param.adv_interval_min = g_roaming_param.adv_interval*8/5;
            tal_adv_param.adv_interval_max = g_roaming_param.adv_interval*8/5;
            tal_ble_advertising_start(&tal_adv_param);

            tuya_ble_custom_evt_send(APP_EVT_0);
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

    if (level == TUYA_GPIO_LEVEL_LOW) {
        tal_sw_timer_start(app_key_timer_id, 10, TAL_TIMER_CYCLE);
    }
}

STATIC VOID_T app_key_irq_cb2(VOID_T *args)
{
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tal_gpio_read(APP_KEY_PIN2, &level);

    if (level == TUYA_GPIO_LEVEL_LOW) {
        tal_sw_timer_start(app_key_timer_id2, 10, TAL_TIMER_CYCLE);
    }
}

STATIC VOID_T app_key_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    if (!tal_key_timeout_handler(&key_press_param)) {
        tal_sw_timer_stop(app_key_timer_id);
    }
}

STATIC VOID_T app_key_timeout_handler2(TIMER_ID timer_id, VOID_T *arg)
{
    if (!tal_key_timeout_handler(&key_press_param2)) {
        tal_sw_timer_stop(app_key_timer_id2);
    }
}

VOID_T app_key_init(VOID_T)
{
    TUYA_GPIO_BASE_CFG_T gpio_cfg = {
        .mode = TUYA_GPIO_PULLUP,
        .direct = TUYA_GPIO_INPUT,
        .level = TUYA_GPIO_LEVEL_LOW,
    };
    TUYA_GPIO_BASE_CFG_T gpio_cfg2 = {
        .mode = TUYA_GPIO_PULLUP,
        .direct = TUYA_GPIO_INPUT,
        .level = TUYA_GPIO_LEVEL_LOW,
    };

    TUYA_GPIO_IRQ_T gpio_irq = {
        .mode = TUYA_GPIO_IRQ_RISE_FALL,
        .cb = app_key_irq_cb,
        .arg = NULL,
    };
    TUYA_GPIO_IRQ_T gpio_irq2 = {
        .mode = TUYA_GPIO_IRQ_RISE_FALL,
        .cb = app_key_irq_cb2,
        .arg = NULL,
    };

    tal_gpio_init(APP_KEY_PIN, &gpio_cfg);
    tal_gpio_irq_init(APP_KEY_PIN, &gpio_irq);
    tal_gpio_irq_enable(APP_KEY_PIN);
    tal_gpio_init(APP_KEY_PIN2, &gpio_cfg2);
    tal_gpio_irq_init(APP_KEY_PIN2, &gpio_irq2);
    tal_gpio_irq_enable(APP_KEY_PIN2);

    tal_key_init(&key_press_param);
    tal_key_init(&key_press_param2);

    tal_sw_timer_create(app_key_timeout_handler, NULL, &app_key_timer_id);
    tal_sw_timer_create(app_key_timeout_handler2, NULL, &app_key_timer_id2);
}

