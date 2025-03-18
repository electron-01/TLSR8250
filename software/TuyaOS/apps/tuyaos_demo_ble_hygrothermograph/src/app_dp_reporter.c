/**
 * @file app_dp_reporter.c
 * @brief This is app_dp_reporter file
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
#include "tal_ble_beacon.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"

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
#include "app_sensor.h"
#include "app_dp_reporter.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define APP_BEACON_DEFAULT_INTERVAL 200
#define APP_BEACON_DURATION_TIME    1000

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT32_T g_con_frame_counter = 0xFFFFFFFF;
STATIC UINT32_T g_adv_frame_counter = 0xFFFFFFFF;
STATIC APP_BLE_BEACON_T sg_app_ble_beacon = {
    .poll_data = {
        .temperature     = 0xFFFF,
        .humidity        = 0xFFFF,
    },

    .tx_status    = 0,
    .report_bit_map = 0,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_ble_beacon_callback(VOID_T *args)
{
    sg_app_ble_beacon.tx_status = 0;
    if (sg_app_ble_beacon.report_bit_map != 0) {
        tuya_ble_custom_evt_send(APP_EVT_1);
    }
}

STATIC VOID_T app_ble_report(VOID_T)
{
    UINT8_T *buf = NULL;
    UINT8_T index = 0;
    OPERATE_RET ret = OPRT_OK;

    buf = (UINT8_T *)tal_malloc(128);
    if (buf == NULL) {
        TAL_PR_ERR("app_ble_report malloc false");
        return;
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_T_H_VALUE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_T_H_VALUE);

        buf[index++] = OR_BASIC_TEMPERATURE;
        buf[index++] = DT_VALUE;
        buf[index++] = (DT_VALUE_LEN >> 8) & 0xFF;
        buf[index++] = (DT_VALUE_LEN >> 0) & 0xFF;
        buf[index++] = (((INT32_T)sg_app_ble_beacon.poll_data.temperature) >> 24) & 0xFF;
        buf[index++] = (((INT32_T)sg_app_ble_beacon.poll_data.temperature) >> 16) & 0xFF;
        buf[index++] = (((INT32_T)sg_app_ble_beacon.poll_data.temperature) >> 8 ) & 0xFF;
        buf[index++] = (((INT32_T)sg_app_ble_beacon.poll_data.temperature) >> 0 ) & 0xFF;

        buf[index++] = OR_BASIC_HUMIDITY;
        buf[index++] = DT_VALUE;
        buf[index++] = (DT_VALUE_LEN >> 8) & 0xFF;
        buf[index++] = (DT_VALUE_LEN >> 0) & 0xFF;
        buf[index++] = (((UINT32_T)sg_app_ble_beacon.poll_data.humidity) >> 24) & 0xFF;
        buf[index++] = (((UINT32_T)sg_app_ble_beacon.poll_data.humidity) >> 16) & 0xFF;
        buf[index++] = (((UINT32_T)sg_app_ble_beacon.poll_data.humidity) >> 8 ) & 0xFF;
        buf[index++] = (((UINT32_T)sg_app_ble_beacon.poll_data.humidity) >> 0 ) & 0xFF;
        TAL_PR_INFO("BLE T/H DATA: %d", sg_app_ble_beacon.poll_data.temperature, sg_app_ble_beacon.poll_data.humidity);
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_BATTERY)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_BATTERY);

        UINT8_T value = 0;
        ret = app_battery_vbat_percent_get(&value);
        if (ret == OPRT_OK) {
            buf[index++] = OR_BASIC_BATTERY;
            buf[index++] = DT_VALUE;
            buf[index++] = (DT_VALUE_LEN >> 8) & 0xFF;
            buf[index++] = (DT_VALUE_LEN >> 0) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 24) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 16) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 8 ) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 0 ) & 0xFF;
            TAL_PR_INFO("BLE VBAT DATA: %d", value);
        }
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_T_SENSITIVE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_T_SENSITIVE);

        UINT32_T value = 0;
        ret = app_temperature_sensitive_get(&value);
        if (ret == OPRT_OK) {
            buf[index++] = WR_BASIC_T_SENSITIVE;
            buf[index++] = DT_VALUE;
            buf[index++] = (DT_VALUE_LEN >> 8) & 0xFF;
            buf[index++] = (DT_VALUE_LEN >> 0) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 24) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 16) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 8 ) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 0 ) & 0xFF;
            TAL_PR_INFO("BLE T SENS DATA: %d", value);
        }
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_H_SENSITIVE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_H_SENSITIVE);

        UINT32_T value = 0;
        ret = app_humidity_sensitive_get(&value);
        if (ret == OPRT_OK) {
            buf[index++] = WR_BASIC_H_SENSITIVE;
            buf[index++] = DT_VALUE;
            buf[index++] = (DT_VALUE_LEN >> 8) & 0xFF;
            buf[index++] = (DT_VALUE_LEN >> 0) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 24) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 16) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 8 ) & 0xFF;
            buf[index++] = (((UINT32_T)value) >> 0 ) & 0xFF;
            TAL_PR_INFO("BLE H SENS DATA: %d", value);
        }
    }

    if (index != 0) {
        g_con_frame_counter++;
        tuya_ble_dp_data_send(g_con_frame_counter, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, buf, index);
        // TAL_PR_HEXDUMP_INFO("BLE DATA:", buf, index);
    }

    tal_free(buf);
}

STATIC VOID_T app_beacon_report(VOID_T)
{
#if ENABLE_BEACON_REPORT
    OPERATE_RET ret = OPRT_OK;

    if (sg_app_ble_beacon.tx_status) {
        TAL_PR_ERR("beacon busy");
        return;
    }

    TAL_PR_DEBUG("beacon report: 0x%x", sg_app_ble_beacon.report_bit_map);
    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_BATTERY)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_BATTERY);

        UINT8_T value = 0;
        ret = app_battery_vbat_percent_get(&value);
        if (ret == OPRT_OK) {
            UINT8_T data[3];
            data[0] = OR_BASIC_BATTERY;
            data[1] = 0x21;
            data[2] = value;
            ret = tal_ble_beacon_dp_data_send(g_adv_frame_counter++, 1, APP_BEACON_DEFAULT_INTERVAL, APP_BEACON_DURATION_TIME, data, SIZEOF(data));
            if (ret == OPRT_OK) {
                sg_app_ble_beacon.tx_status = 1;
                return;
            }
        }
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_T_SENSITIVE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_T_SENSITIVE);

        UINT32_T value = 0;
        ret = app_temperature_sensitive_get(&value);
        if (ret == OPRT_OK) {
            UINT8_T data[6];
            data[0] = WR_BASIC_T_SENSITIVE;
            data[1] = 0x24;
            data[2] = (((UINT32_T)value) >> 24) & 0xFF;
            data[3] = (((UINT32_T)value) >> 16) & 0xFF;
            data[4] = (((UINT32_T)value) >> 8 ) & 0xFF;
            data[5] = (((UINT32_T)value) >> 0 ) & 0xFF;
            ret = tal_ble_beacon_dp_data_send(g_adv_frame_counter++, 1, APP_BEACON_DEFAULT_INTERVAL, APP_BEACON_DURATION_TIME, data, SIZEOF(data));
            if (ret == OPRT_OK) {
                sg_app_ble_beacon.tx_status = 1;
                return;
            }
        }
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_H_SENSITIVE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_H_SENSITIVE);

        UINT32_T value = 0;
        ret = app_humidity_sensitive_get(&value);
        if (ret == OPRT_OK) {
            UINT8_T data[6];
            data[0] = WR_BASIC_H_SENSITIVE;
            data[1] = 0x24;
            data[2] = (((UINT32_T)value) >> 24) & 0xFF;
            data[3] = (((UINT32_T)value) >> 16) & 0xFF;
            data[4] = (((UINT32_T)value) >> 8 ) & 0xFF;
            data[5] = (((UINT32_T)value) >> 0 ) & 0xFF;
            ret = tal_ble_beacon_dp_data_send(g_adv_frame_counter++, 1, APP_BEACON_DEFAULT_INTERVAL, APP_BEACON_DURATION_TIME, data, SIZEOF(data));
            if (ret == OPRT_OK) {
                sg_app_ble_beacon.tx_status = 1;
                return;
            }
        }
    }

    if (sg_app_ble_beacon.report_bit_map & (1 << APP_REPORT_T_H_VALUE)) {
        sg_app_ble_beacon.report_bit_map &= ~(1 << APP_REPORT_T_H_VALUE);

        UINT8_T data[8];
        data[0] = OR_BASIC_TEMPERATURE;
        data[1] = 0x22;
        data[2] = (sg_app_ble_beacon.poll_data.temperature >> 8)  & 0xFF;
        data[3] = (sg_app_ble_beacon.poll_data.temperature >> 0)  & 0xFF;
        data[4] = OR_BASIC_HUMIDITY;
        data[5] = 0x22;
        data[6] = (sg_app_ble_beacon.poll_data.humidity >> 8)  & 0xFF;
        data[7] = (sg_app_ble_beacon.poll_data.humidity >> 0)  & 0xFF;
        ret = tal_ble_beacon_dp_data_send(g_adv_frame_counter++, 1, APP_BEACON_DEFAULT_INTERVAL, APP_BEACON_DURATION_TIME, data, SIZEOF(data));
        if (ret == OPRT_OK) {
            sg_app_ble_beacon.tx_status = 1;
            return;
        }
    }
#endif

}

OPERATE_RET app_dp_reporter_temperature_set(INT16_T temperature)
{
    sg_app_ble_beacon.poll_data.temperature = temperature;
    return OPRT_OK;
}

OPERATE_RET app_dp_reporter_humidity_set(UINT16_T humidity)
{
    sg_app_ble_beacon.poll_data.humidity = humidity;
    return OPRT_OK;
}

OPERATE_RET app_dp_reporter_sensor_data_get(APP_SENSOR_DATA_T *data)
{
    if (data) {
        data->temperature = sg_app_ble_beacon.poll_data.temperature;
        data->humidity    = sg_app_ble_beacon.poll_data.humidity;
        return OPRT_OK;
    }

    return OPRT_INVALID_PARM;
}

OPERATE_RET app_dp_reporter_event_set(APP_DP_REPORTER_E report)
{
    OPERATE_RET ret = OPRT_OK;

    switch (report) {
        case APP_REPORT_T_H_VALUE:
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_T_H_VALUE);
        break;
        case APP_REPORT_BATTERY:
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_BATTERY);
        break;
        case APP_REPORT_T_SENSITIVE:
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_T_SENSITIVE);
        break;
        case APP_REPORT_H_SENSITIVE:
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_H_SENSITIVE);
        break;
        case APP_REPORT_ALL:
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_T_H_VALUE);
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_BATTERY);
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_T_SENSITIVE);
            sg_app_ble_beacon.report_bit_map |= (1 << APP_REPORT_H_SENSITIVE);
        break;
        default:
            TAL_PR_ERR("NOT FOUND REPORT DP TYPE");
            ret = OPRT_NOT_SUPPORTED;
        break;
    }

    if (sg_app_ble_beacon.report_bit_map != 0) {
        tuya_ble_custom_evt_send(APP_EVT_1);
    }

    return ret;
}

VOID_T app_ble_beacon_process(VOID_T)
{
    if (sg_app_ble_beacon.report_bit_map == 0) {
        return;
    }

    tuya_ble_connect_status_t status = tuya_ble_connect_status_get();
    if (status == BONDING_CONN) {
        app_ble_report();
    } else if (status == BONDING_UNCONN) {

#if ENABLE_BEACON_REPORT
        app_beacon_report();
#endif

    }
}

VOID_T app_dp_reporter_init(VOID_T)
{

#if ENABLE_BEACON_REPORT
    tal_ble_beacon_init(APP_BEACON_DEFAULT_INTERVAL, app_ble_beacon_callback);
#endif

}

