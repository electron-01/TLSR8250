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
#include "tal_rtc.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"

#include "app_dp_parser.h"
#include "app_lock.h"

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

STATIC BOOL_T autolock_stu = FALSE;
STATIC UINT32_T autolock_period = 5000; //!< unit: ms
STATIC BOOL_T doorlock_opened_status = FALSE;

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
        case WR_DP_SETTING_AUTOLOCK_STU: {      //!< autolock state
            autolock_stu = g_cmd.dp_data[0];
            TAL_PR_INFO("dp set autolock stu: %d", autolock_stu);
            app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);

            if (autolock_stu && doorlock_opened_status) {
                app_lock_autolock_timer_stop();

                UINT8_T lock_status = 0x00;     //!< close lock immediately, sync lock status closed
                app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);
            }
        } break;

        case WR_DP_SETTING_AUTOLOCK_PERIOD: {
            autolock_period  = g_cmd.dp_data[0]<<24;
            autolock_period += g_cmd.dp_data[1]<<16;
            autolock_period += g_cmd.dp_data[2]<<8;
            autolock_period += g_cmd.dp_data[3];
            autolock_period *= 1000;
            TAL_PR_INFO("dp set autolock period: %d(ms)", autolock_period);

            app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);
        } break;

        case WR_DP_MANUAL_CLOSE_DOORLOCK: {     //!< manual close doorlock
            TAL_PR_INFO("dp manual close lock");
            doorlock_opened_status = FALSE;

            app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);

            UINT8_T lock_status = 0x00;         //!< lock status closed
            app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);

            UINT8_T data[5] = {0};              //!< report lock closed record
            data[0] = APP_CLOSE_LOCK;
            data[4] = 0xFF;
            app_dp_record_report(OR_DP_RECORD_CLOSE_LOCK_COMMON, data, 5);

            if (autolock_stu) {
                app_lock_autolock_timer_stop();
            }
        } break;

        case WR_SBP_DP_MASTER_OPEN_LOCK: {      //!< new ble open lock [dev with ble parts function]
            TAL_PR_INFO("dp ble open lock");
            doorlock_opened_status = TRUE;

            UINT8_T lock_status = 0x01;         //!< lock status opened
            app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);

            UINT8_T data[4] = {0};              //!< report lock opened record
            data[3] = g_cmd.dp_data[18];        //!< operate user
            app_dp_record_report(OR_DP_RECORD_BLE_OPEN_LOCK, data, 4);

            if (autolock_stu) {
                app_lock_autolock_timer_start(autolock_period);
            }
        } break;

        case WR_SBP_DP_SET_MASTER_SRAND_NUM: {  //!< set master srand num [dev with ble parts function]
            TAL_PR_INFO("dp set ble open lock param");
        } break;

        default: {
        } break;
    }

    return OPRT_OK;
}

OPERATE_RET app_dp_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size)
{
    UINT16_T rsp_len;
    memset(&g_rsp, 0, SIZEOF(demo_dp_t));

    g_rsp.dp_id = dp_id;

    switch (dp_id) {
        case OR_DP_STATUS_BATTERY_PERCENT: {
            g_rsp.dp_type = DT_VALUE;
            g_rsp.dp_data_len = DT_VALUE_LEN;
            memcpy(g_rsp.dp_data, buf, DT_VALUE_LEN);
        } break;

        case WR_DP_SETTING_AUTOLOCK_STU: {
            g_rsp.dp_type = DT_BOOL;
            g_rsp.dp_data_len = DT_BOOL_LEN;
            memcpy(g_rsp.dp_data, buf, DT_BOOL_LEN);
        } break;

        case WR_DP_SETTING_AUTOLOCK_PERIOD: {
            g_rsp.dp_type = DT_VALUE;
            g_rsp.dp_data_len = DT_VALUE_LEN;
            memcpy(g_rsp.dp_data, buf, DT_VALUE_LEN);
        } break;

        case WR_DP_MANUAL_CLOSE_DOORLOCK: {
            g_rsp.dp_type = DT_BOOL;
            g_rsp.dp_data_len = DT_BOOL_LEN;
            memcpy(g_rsp.dp_data, buf, DT_BOOL_LEN);
        } break;

        case OR_DP_STATUS_LOCK_STU: {
            g_rsp.dp_type = DT_BOOL;
            g_rsp.dp_data_len = DT_BOOL_LEN;
            memcpy(g_rsp.dp_data, buf, DT_BOOL_LEN);
        } break;

        case WR_SBP_DP_MASTER_OPEN_LOCK: {
            g_rsp.dp_type = DT_RAW;
            g_rsp.dp_data_len = size;
            memcpy(g_rsp.dp_data, buf, size);
        } break;

        case WR_SBP_DP_SET_MASTER_SRAND_NUM: {
            g_rsp.dp_type = DT_RAW;
            g_rsp.dp_data_len = size;
            memcpy(g_rsp.dp_data, buf, size);
        } break;

        default: {
        } break;
    }

    rsp_len = g_rsp.dp_data_len + 4;
    tal_util_reverse_byte(&g_rsp.dp_data_len, SIZEOF(UINT16_T));

    TAL_PR_HEXDUMP_INFO("dp_rsp", (VOID_T*)&g_rsp, rsp_len);

    return tuya_ble_dp_data_send(g_sn++, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, (VOID_T*)&g_rsp, rsp_len);
}

OPERATE_RET app_dp_record_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size)
{
    UINT32_T ts;
    UINT8_T record_data[16];

    tal_rtc_time_get(&ts);
    TAL_PR_INFO("push record, dp_id=[%d] timestamp=[%d]", dp_id, ts);

    record_data[0] = dp_id;

    switch (dp_id) {
        case OR_DP_RECORD_BLE_OPEN_LOCK: {
            record_data[1] = DT_VALUE;
            record_data[2] = 0;
            record_data[3] = DT_VALUE_LEN;
            memcpy(record_data+4, buf, DT_VALUE_LEN);
        } break;

        case OR_DP_RECORD_CLOSE_LOCK_COMMON: {
            record_data[1] = DT_RAW;
            record_data[2] = 0;
            record_data[3] = size;
            memcpy(record_data+4, buf, size);
        } break;

        default: {
        } break;
    }

    tal_util_reverse_byte(&ts, SIZEOF(UINT32_T));
    TAL_PR_HEXDUMP_INFO("dp_record_report", (VOID_T*)&record_data, size+4);

    return tuya_ble_dp_data_with_time_send(g_sn++, DP_SEND_FOR_CLOUD_PANEL, DP_TIME_TYPE_UNIX_TIMESTAMP, (UINT8_T*)&ts, record_data, size+4);
}

