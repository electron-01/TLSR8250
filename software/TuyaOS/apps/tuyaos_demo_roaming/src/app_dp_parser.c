/**
 * @file app_dp_parser.c
 * @brief This is app_dp_parser file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "string.h"

#include "tal_log.h"
#include "tal_util.h"
#include "tal_ble_beacon.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"

#include "app_dp_parser.h"
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
demo_dp_t g_cmd = {0};
demo_dp_t g_rsp = {0};
UINT32_T  g_sn  = 0;
uint8_t   g_dp_report_count = 0;
dp_value_t g_dp_value = {
    .welcome = "welcome-0",
    .custom_data = {1, 2, 3, 4, 0},
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




OPERATE_RET app_dp_parser(UINT8_T* buf, UINT32_T size)
{
    memcpy(&g_cmd, buf, size);
    tal_util_reverse_byte(&g_cmd.dp_data_len, SIZEOF(UINT16_T));
    memcpy(&g_rsp, &g_cmd, size);

    TAL_PR_HEXDUMP_INFO("dp_cmd", (VOID_T*)&g_cmd, (g_cmd.dp_data_len + 4));

    switch (g_cmd.dp_id) {
        case WR_BASIC_LED: {
        } break;

        case WR_BASIC_CHARGE_STATE: {
        } break;

        case WR_BASIC_TEMPERATURE: {
        } break;

        case WR_BASIC_WELCOME: {
        } break;

        case WR_BASIC_CUSTOM_DATA: {
        } break;

        default: {
        } break;
    }

    app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);

    return OPRT_OK;
}

OPERATE_RET app_dp_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size)
{
    memset(&g_rsp, 0, SIZEOF(demo_dp_t));

    g_rsp.dp_id = dp_id;

    switch (dp_id) {
        //dp_type = bool
        case WR_BASIC_LED: {
            g_rsp.dp_type = DT_BOOL;
            g_rsp.dp_data_len = DT_BOOL_LEN;
            memcpy(g_rsp.dp_data, buf, DT_BOOL_LEN);
        } break;

        //dp_type = enum
        case WR_BASIC_CHARGE_STATE: {
            g_rsp.dp_type = DT_ENUM;
            g_rsp.dp_data_len = DT_ENUM_LEN;
            memcpy(g_rsp.dp_data, buf, DT_ENUM_LEN);
        } break;

        //dp_type = value
        case WR_BASIC_TEMPERATURE: {
            g_rsp.dp_type = DT_VALUE;
            g_rsp.dp_data_len = DT_VALUE_LEN;
            memcpy(g_rsp.dp_data, buf, DT_VALUE_LEN);
        } break;

        //dp_type = string
        case WR_BASIC_WELCOME: {
            g_rsp.dp_type = DT_STRING;
            g_rsp.dp_data_len = size;
            memcpy(g_rsp.dp_data, buf, size);
        } break;

        //dp_type = raw
        case WR_BASIC_CUSTOM_DATA: {
            g_rsp.dp_type = DT_RAW;
            g_rsp.dp_data_len = size;
            memcpy(g_rsp.dp_data, buf, size);
        } break;

        //dp_type = fault
        case WR_BASIC_FAULT_ALARM: {
            g_rsp.dp_type = DT_BITMAP;
            g_rsp.dp_data_len = DT_BITMAP_MAX;
            memcpy(g_rsp.dp_data, buf, DT_BITMAP_MAX);
        } break;

        default: {
        } break;
    }

    UINT16_T rsp_len = g_rsp.dp_data_len + 4;

    tal_util_reverse_byte(&g_rsp.dp_data_len, SIZEOF(UINT16_T));

    TAL_PR_HEXDUMP_INFO("dp_rsp", (VOID_T*)&g_rsp, rsp_len);

    if (tuya_ble_connect_status_get() == BONDING_CONN) {
        return tuya_ble_dp_data_send(g_sn++, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, (VOID_T*)&g_rsp, rsp_len);
    } else {
        if (rsp_len > 11+4) {
            return OPRT_NOT_SUPPORTED;
        }

        UINT8_T adv_dp_data[31] = {0};
        adv_dp_data[0] = g_rsp.dp_id;
        adv_dp_data[1] = ((g_rsp.dp_type<<4) & 0xF0) | (g_rsp.dp_data_len>>8);
        memcpy(&adv_dp_data[2], g_rsp.dp_data, rsp_len - 4);

        tal_ble_beacon_dp_data_send(adv_frame_counter++, 1, 20, g_roaming_param.adv_duration, adv_dp_data, rsp_len - 2);
        if (adv_frame_counter % 256 == 0) {
            tuya_ble_custom_evt_send(APP_EVT_1);
        }
    }

    return OPRT_OK;
}

