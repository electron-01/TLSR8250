/**
 * @file app_sensor.c
 * @brief This is app_sensor file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "board.h"

#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_i2c.h"
#include "tal_rtc.h"
#include "tal_util.h"
#include "tal_sw_timer.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"

#if ENABLE_LED

#include "app_led.h"

#endif

#if ENABLE_KEY

#include "app_key.h"

#endif

#include "app_dp_reporter.h"
#include "app_misc.h"
#include "app_sensor.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT16_T sg_app_sensor_time_pass = 0;
STATIC UINT8_T sg_app_sensor_step       = 0;
STATIC UINT8_T sg_app_sensor_report_flag = 0;
STATIC APP_SENSOR_DATA_T th_sensor_data = {
    .temperature = 500,
    .humidity = 100,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_sensor_t_h_overflow_check(VOID_T)
{
    INT16_T res = 0;
    APP_SENSOR_DATA_T data;
    UINT8_T is_over_sensitive = 0;
    INT32_T sensitive;

    app_dp_reporter_sensor_data_get(&data);

    app_temperature_sensitive_get(&sensitive);
    res = data.temperature - th_sensor_data.temperature;
    if ((res >= sensitive) || (res <= (-sensitive))) {
        is_over_sensitive = 1;
        app_dp_reporter_temperature_set(th_sensor_data.temperature);
        TAL_PR_INFO("T OverFlow: %d, %d - %d", res, data.temperature, th_sensor_data.temperature);
    }

    app_humidity_sensitive_get(&sensitive);
    res = data.humidity - th_sensor_data.humidity;
    if ((res >= sensitive) || (res <= (-sensitive))) {
        is_over_sensitive = 1;
        app_dp_reporter_humidity_set(th_sensor_data.humidity);
        TAL_PR_INFO("H OverFlow: %d, %d - %d", res, data.humidity, th_sensor_data.humidity);
    }

    if (is_over_sensitive) {
        is_over_sensitive = 0;
        app_dp_reporter_event_set(APP_REPORT_T_H_VALUE);
    }
}

STATIC OPERATE_RET app_sensor_trigger(VOID_T)
{
    // sensor start conversion t/h data

    return OPRT_OK;
}

STATIC OPERATE_RET app_sensor_data_read(VOID_T)
{
    // read the converted data from the sensor

    // simulation sensor data:
    // generate temperature data

    UINT32_T sensitive;

    app_temperature_sensitive_get(&sensitive);
    th_sensor_data.temperature = 250 + tal_system_get_random(sensitive * 2);
    if (th_sensor_data.temperature > APP_SENSOR_T_DATA_MAX) {
        th_sensor_data.temperature = APP_SENSOR_T_DATA_MAX;
    }
    if (th_sensor_data.temperature < APP_SENSOR_T_DATA_MIN) {
        th_sensor_data.temperature = APP_SENSOR_T_DATA_MIN;
    }


    app_humidity_sensitive_get(&sensitive);
    th_sensor_data.humidity    = 50 + tal_system_get_random(sensitive * 2);
    if (th_sensor_data.humidity > APP_SENSOR_H_DATA_MAX) {
        th_sensor_data.humidity = APP_SENSOR_H_DATA_MAX;
    }
    if (th_sensor_data.humidity < APP_SENSOR_H_DATA_MIN) {
        th_sensor_data.humidity = APP_SENSOR_H_DATA_MIN;
    }

    return OPRT_OK;
}

VOID_T app_sensor_hw_init(VOID_T)
{
    // peripherals hardware init

}

VOID_T app_sensor_sw_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    // sensor Init and sensor data get
    th_sensor_data.temperature = 250;
    th_sensor_data.humidity    = 50;

    // Check if the temperature and humidity data exceeds the sensitivity.
    app_sensor_t_h_overflow_check();
    TAL_PR_INFO("T: %d, H: %d", th_sensor_data.temperature, th_sensor_data.humidity);
}

OPERATE_RET app_sensor_process(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    if (sg_app_sensor_step == 0) {
        ret = app_sensor_trigger();
        if (ret == OPRT_OK) {
            sg_app_sensor_step = 1;
        }
    } else if (sg_app_sensor_step == 1) {
        sg_app_sensor_step = 0;

        ret = app_sensor_data_read();
        if (ret == OPRT_OK) {
            app_sensor_t_h_overflow_check();
            TAL_PR_INFO("T: %d, H: %d", th_sensor_data.temperature, th_sensor_data.humidity);
        }
    }

    sg_app_sensor_time_pass += APP_SENSOR_RUNNIGN_CYCLE;
    if (sg_app_sensor_time_pass >= APP_SENSOR_REPORT_PERIOD) {
        TAL_PR_INFO("TX TIMEOUT: %d", sg_app_sensor_time_pass);
        sg_app_sensor_time_pass = 0;
        sg_app_sensor_report_flag = 1;
    }

    if (sg_app_sensor_report_flag) {
        sg_app_sensor_report_flag = 0;

        app_dp_reporter_temperature_set(th_sensor_data.temperature);
        app_dp_reporter_humidity_set(th_sensor_data.humidity);
        app_dp_reporter_event_set(APP_REPORT_T_H_VALUE);
    }

    return ret;
}

APP_SENSOR_DATA_T app_sensor_data_get(VOID_T)
{
    return th_sensor_data;
}

