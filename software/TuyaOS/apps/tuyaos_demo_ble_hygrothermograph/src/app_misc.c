/**
 * @file app_misc.c
 * @brief This is app_misc file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "board.h"

#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"
#include "tal_rtc.h"
#include "tal_key.h"
#include "tal_bluetooth_def.h"
#include "tal_util.h"
#include "tal_sleep.h"
#include "tal_flash.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"
#include "tuya_sdk_callback.h"

#if ENABLE_LED

#include "app_led.h"

#endif

#if ENABLE_KEY

#include "app_key.h"

#endif

#if ENABLE_BATTERY

#include "app_battery.h"

#endif

#include "app_misc.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define DEVICE_APP_CONFIG_ADDR 0x78000

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TIMER_ID app_pairing_timer_id = NULL;
STATIC TIMER_ID app_enable_enter_sleep_timer_id = NULL;
STATIC TIMER_ID app_running_timer_id = NULL;

APP_STATUS_INFO_T app_status_info;
APP_CONFIG_INFO_T app_config_info = {
    .temperature_sensitive = 30,
    .humidity_sensitive    = 3,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_running_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    tuya_ble_custom_evt_send(APP_EVT_0);
}

STATIC VOID_T app_enable_enter_sleep_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    if (tuya_ble_internal_production_test_with_ble_flag_get() == 0) {
        tal_cpu_allow_sleep();
    }
}

STATIC VOID_T app_pairing_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    app_pairing_stop();
}

OPERATE_RET app_pairing_start(VOID_T)
{
    tal_adv_param.adv_interval_min = 200 * 8 / 5;
    tal_adv_param.adv_interval_max = 200 * 8 / 5;
    tal_ble_advertising_start(&tal_adv_param);
    tal_sw_timer_start(app_pairing_timer_id, 60 * 1000, TAL_TIMER_ONCE);

    app_status_info.enter_pairing = 1;
    TAL_PR_DEBUG("START PAIRING");
    return OPRT_OK;
}

OPERATE_RET app_pairing_stop(VOID_T)
{
    tal_adv_param.adv_interval_min = TY_ADV_INTERVAL * 8 / 5;
    tal_adv_param.adv_interval_max = TY_ADV_INTERVAL * 8 / 5;
    tal_ble_advertising_stop();
    tal_sw_timer_stop(app_pairing_timer_id);


    app_status_info.enter_pairing = 0;
    TAL_PR_DEBUG("STOP PAIRING");
    return OPRT_OK;
}

OPERATE_RET app_temperature_sensitive_set(INT32_T value)
{
    if (value > 500) {
        return OPRT_INVALID_PARM;
    }

    app_config_info.temperature_sensitive = value;
    return OPRT_OK;
}

OPERATE_RET app_humidity_sensitive_set(INT32_T value)
{
    if (value > 50) {
        return OPRT_INVALID_PARM;
    }

    app_config_info.humidity_sensitive = value;
    return OPRT_OK;
}

OPERATE_RET app_temperature_sensitive_get(INT32_T *value)
{
    if (value) {
        *value = app_config_info.temperature_sensitive;
        return OPRT_OK;
    }
    return OPRT_INVALID_PARM;
}

OPERATE_RET app_humidity_sensitive_get(INT32_T *value)
{
    if (value) {
        *value = app_config_info.humidity_sensitive;
        return OPRT_OK;
    }
    return OPRT_INVALID_PARM;
}

OPERATE_RET app_config_param_save(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    app_config_info.crc32 = tal_util_crc32((UINT8_T*)&app_config_info, SIZEOF(app_config_info)-4, NULL);
    ret = tal_flash_erase(DEVICE_APP_CONFIG_ADDR, 0x1000);
    if (ret != OPRT_OK) {
        return ret;
    }

    ret = tal_flash_write(DEVICE_APP_CONFIG_ADDR, (CONST UCHAR_T *)&app_config_info, sizeof(app_config_info));
    if (ret != OPRT_OK) {
        return ret;
    }

    return OPRT_OK;
}

OPERATE_RET app_config_param_reset(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    app_config_info.temperature_sensitive = 30;
    app_config_info.humidity_sensitive    = 3;
    app_config_info.history_record_index  = 0;
    app_config_info.history_record_pages  = 0;
    app_config_info.crc32 = tal_util_crc32((UINT8_T*)&app_config_info, SIZEOF(app_config_info)-4, NULL);

    ret = tal_flash_erase(DEVICE_APP_CONFIG_ADDR, 0x1000);
    if (ret != OPRT_OK) {
        TAL_PR_ERR("Erase False");
        return ret;
    }

    ret = tal_flash_write(DEVICE_APP_CONFIG_ADDR, (UINT8_T*)&app_config_info, SIZEOF(app_config_info));
    if (ret != OPRT_OK) {
        TAL_PR_ERR("Write False");
        return ret;
    }

    return OPRT_OK;
}

OPERATE_RET app_misc_init(VOID_T)
{
    // init app status infomation and get device config parameter
    memset(&app_status_info, 0, SIZEOF(APP_STATUS_INFO_T));
    tal_flash_read(DEVICE_APP_CONFIG_ADDR, (UCHAR_T *)&app_config_info, SIZEOF(app_config_info));
    if (app_config_info.crc32 != tal_util_crc32((UINT8_T*)&app_config_info, SIZEOF(app_config_info)-4, NULL)) {
        app_config_param_reset();
    }

    tal_sw_timer_create(app_pairing_timeout_handler, NULL, &app_pairing_timer_id);
    // system running cycle timer
    tal_sw_timer_create(app_running_timeout_handler, NULL, &app_running_timer_id);
    // deivce need keep awake at least 1000ms for tuya uart authorization
    tal_sw_timer_create(app_enable_enter_sleep_timeout_handler, NULL, &app_enable_enter_sleep_timer_id);

    if (tuya_ble_connect_status_get() == BONDING_UNCONN) {
        // automatic enter pairing or advertising when device power on
        tal_adv_param.adv_interval_min = TY_ADV_INTERVAL * 8 / 5;
        tal_adv_param.adv_interval_max = TY_ADV_INTERVAL * 8 / 5;
        tal_ble_advertising_start(&tal_adv_param);

        app_status_info.bound_flag = 1;
#if ENABLE_LED
        app_led_start(APP_LED_POWER_ON);
#endif
    } else {
        app_pairing_start();
#if ENABLE_LED
    app_led_start(APP_LED_PAIRING_START);
#endif
    }

    tal_sw_timer_start(app_enable_enter_sleep_timer_id, 1000, TAL_TIMER_ONCE);
    tal_sw_timer_start(app_running_timer_id, APP_SENSOR_RUNNIGN_CYCLE * 1000, TAL_TIMER_CYCLE);
}

