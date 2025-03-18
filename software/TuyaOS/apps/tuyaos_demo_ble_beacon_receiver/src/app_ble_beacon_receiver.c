/**
 * @file app_ble_beacon_receiver.c
 * @brief This is app_ble_beacon_receiver file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "board.h"
#include "stdint.h"
#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_bluetooth.h"
#include "tal_bluetooth_def.h"
#include "tal_util.h"
#include "tal_flash.h"
#include "tal_sdk_test.h"
#include "tal_ble_beacon_remoter.h"

#include "tuya_ble_type.h"
#include "tuya_ble_port.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_main.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"

#include "app_ble_beacon_receiver.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
#if (TUYA_SDK_TEST && (TUYA_SDK_TEST == 1))
STATIC UINT8_T test_index = 0;
#endif
TAL_BLE_BEACON_REMOTE_INFO_T sg_info[REMOTER_NUM] = {0};
STATIC TIMER_ID app_info_save_timer_id = NULL;
STATIC TIMER_ID app_beacon_pair_windows_timer_id = NULL;
STATIC UINT8_T app_beacon_enable_pair_flag = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




VOID_T app_info_save_event_handler(VOID_T)
{
    tal_flash_erase(BOARD_FLASH_SDK_TEST_START_ADDR, 0x1000);
    tal_flash_write(BOARD_FLASH_SDK_TEST_START_ADDR, sg_info[0].mac, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T) * REMOTER_NUM);
    TAL_PR_INFO("Device Info Save");
}

STATIC VOID_T app_info_save_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    tuya_ble_custom_evt_send(APP_EVT_1);
}

STATIC VOID_T app_beacon_pair_windows_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    tal_sw_timer_stop(app_beacon_pair_windows_timer_id);
    tal_sw_timer_delete(app_beacon_pair_windows_timer_id);
    app_beacon_pair_windows_timer_id = NULL;

    app_beacon_enable_pair_flag = 0;
}

OPERATE_RET tal_ble_beacon_info_save(VOID_T)
{
    tal_sw_timer_start(app_info_save_timer_id, 5*1000, TAL_TIMER_ONCE);
    return OPRT_OK;
}

VOID_T app_receiver_init(VOID_T)
{
    TAL_BLE_SCAN_PARAMS_T tal_scan_param = {
        .type = TAL_BLE_SCAN_TYPE_ACTIVE,
        .scan_interval = 100*8/5,
        .scan_window = 80*8/5,
        .timeout = 0,
        .filter_dup = 0,
    };
    tal_ble_scan_start(&tal_scan_param);

    // search remote device from flash
    UINT8_T temp[sizeof(TAL_BLE_BEACON_REMOTE_INFO_T)];
    UINT8_T check_0x00[sizeof(TAL_BLE_BEACON_REMOTE_INFO_T)];
    UINT8_T check_0xFF[sizeof(TAL_BLE_BEACON_REMOTE_INFO_T)];
    memset(check_0x00, 0x00, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T));
    memset(check_0xFF, 0xFF, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T));
    for (UINT8_T i = 0; i < REMOTER_NUM; i++) {
        tal_flash_read(BOARD_FLASH_SDK_TEST_START_ADDR + (sizeof(TAL_BLE_BEACON_REMOTE_INFO_T) * i), temp, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T));
        if (memcmp(temp, check_0x00, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T)) == 0) {
            continue;
        }
        if (memcmp(temp, check_0xFF, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T)) == 0) {
            continue;
        }

        memcpy(sg_info[i].mac, temp, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T));
        sg_info[i].sn = 0xFFFFFFFF;
        TAL_PR_HEXDUMP_DEBUG("FIND DEVICE: ", sg_info[i].mac, sizeof(TAL_BLE_BEACON_REMOTE_INFO_T));
    }
    tal_ble_beacon_remoter_init((TAL_BLE_BEACON_REMOTE_INFO_T *)&sg_info, REMOTER_NUM);
    tal_sw_timer_create(app_info_save_timeout_handler, NULL, &app_info_save_timer_id);

    app_beacon_enable_pair_flag = 1;
    if(app_beacon_pair_windows_timer_id == NULL) {
        tal_sw_timer_create(app_beacon_pair_windows_timeout_handler, NULL, &app_beacon_pair_windows_timer_id);
    }
    tal_sw_timer_start(app_beacon_pair_windows_timer_id, 1 * 60 * 1000, TAL_TIMER_ONCE);
}

OPERATE_RET app_receiver_info_get(tuya_ble_remoter_proxy_auth_data_unit_t data_unit[], UINT8_T *unit_num)
{
    UINT8_T sum = 0;

    if (data_unit == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (unit_num == NULL) {
        return OPRT_INVALID_PARM;
    }

    for (UINT32_T idx=0; idx<REMOTER_NUM; idx++) {
        if (sg_info[idx].group_id != 0) {
            data_unit[sum].type = sg_info[idx].is_auth;
            memcpy(data_unit[sum].mac, &sg_info[idx], 9);
            sum++;
        }
    }
    *unit_num = sum;

    return OPRT_OK;
}

VOID_T app_receiver_data_handler(VOID_T* buf)
{
    UINT8_T idx;
    UINT8_T mac[6];
    TAL_BLE_BEACON_REMOTE_SERVICE_DATA_T service_data = {0};
    TAL_BLE_ADV_REPORT_T *adv_report;

    adv_report = ((TAL_BLE_ADV_REPORT_T *)buf);
    if (adv_report->data_len <= 7) {
        return;
    }

    if (tal_ble_beacon_service_data_parser(adv_report, &service_data, mac, &idx) == OPRT_OK) {
        app_receiver_cmd_parser(idx, mac, service_data.dp_data);

#if (TUYA_SDK_TEST && (TUYA_SDK_TEST == 1))
        if((app_beacon_enable_pair_flag == 0) && (service_data.dp_data.cmd == CMD_BONDING)) {
            return;
        }

        UINT8_T tmp_buffer[DP_DATA_LEN + 1] = {0};
        tmp_buffer[0] = test_index;
        if(service_data.dp_data.cmd == 0x04) {
            service_data.dp_data.cmd_data[1] = service_data.dp_data.group_id;
        }
        memcpy(&tmp_buffer[1], &service_data.dp_data.dp_id, DP_DATA_LEN);
        test_cmd_send(TEST_ID_GET(TEST_GID_ELSE, TEST_CID_GET_REMOTER_DATA), tmp_buffer, sizeof(tmp_buffer));
#endif
    }
}

VOID_T app_receiver_cmd_parser(UINT8_T idx, UINT8_T* p_mac, TAL_BLE_BEACON_REMOTE_DP_DATA_T dp_data)
{
#if (TUYA_SDK_TEST && (TUYA_SDK_TEST == 1))
    test_index = tal_ble_beacon_remoter_find(p_mac);
#endif
    switch (dp_data.cmd) {
        case CMD_KEY_VALUE: {
        } break;

        case CMD_BONDING: {
            if(app_beacon_enable_pair_flag) {
                tal_ble_beacon_remoter_group_add(p_mac, dp_data.cmd_data);
#if (TUYA_SDK_TEST && (TUYA_SDK_TEST == 1))
                test_index = tal_ble_beacon_remoter_find(p_mac);
                TAL_PR_DEBUG("test_index = %d", test_index);
#endif
            }
        } break;

        case CMD_UNBONDING: {
            tal_ble_beacon_remoter_group_delete(TAL_BLE_BEACON_LOCAL_CMD, p_mac, dp_data.cmd_data);
        } break;

        case CMD_SWITCH: {
        } break;

        default: {
        } break;
    }
}

