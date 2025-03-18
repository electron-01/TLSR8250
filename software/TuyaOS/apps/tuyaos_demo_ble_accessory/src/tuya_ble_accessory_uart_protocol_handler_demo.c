/**
 * @file tuya_ble_accessory_uart_protocol_handler_demo.c
 * @brief This is tuya_ble_accessory_uart_protocol_handler_demo file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "tuya_ble_type.h"
#include "tal_memory.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tal_util.h"
#include "tuya_ble_event.h"
#include "tuya_ble_log.h"
#include "tuya_ble_internal_config.h"
#include "tuya_ble_accessory_demo.h"
#include "tuya_ble_feature_accessory.h"
#include "tuya_ble_accessory_uart_protocol_handler_demo.h"
#include "tal_log.h"

#if (TUYA_BLE_ACCESSORY_MOUNT_SUPPORTED != 0)

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TUYA_APP_PRODUCT_TEST_ACCESSORY_PASSTHROUGH 0xFF0A
#define TUYA_BLE_ACCESSORY_OTA_DATA_LENGTH_MAX 200

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT8_T accessory_data_buffer[48];
STATIC UINT8_T accessory_srand[6];
STATIC UINT8_T accessory_mcu_ota_flag = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
OPERATE_RET tuya_ble_accessory_uart_send_data(UINT8_T port_id, CONST UINT8_T *p_data, UINT16_T len)
{
    switch (port_id) {
        case 0x00: {
            tuya_ble_common_uart_send_data(p_data, len);
        } break;
        case 0x01: {
            //Multi-accessories can just adapt the send function for different ports here.
        } break;
        case 0x02: {
            //Multi-accessories can just adapt the send function for different ports here.
        } break;
        default: {
            //Multi-accessories can just adapt the send function for different ports here.
        } break;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_ble_accessory_uart_receive_data(UINT8_T port_id, UINT8_T *p_data, UINT16_T len)
{
    switch (port_id) {
        case 0x00: {
            tuya_ble_common_uart_receive_data(p_data, len);
        } break;
        case 0x01: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
        case 0x02: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
        default: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_ble_accessory_uart_full_instruction_received(UINT8_T port_id, UINT8_T *p_data, UINT16_T len)
{
    switch (port_id) {
        case 0x00: {
            tuya_ble_common_uart_send_full_instruction_received(p_data, len);
        } break;
        case 0x01: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
        case 0x02: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
        default: {
            //Multi-accessories can just adapt the receive function for different ports here.
        } break;
    }

    return OPRT_OK;
}

STATIC VOID_T tuya_ble_accessory_uart_protocol_send(UINT8_T port_id, UINT8_T cmd, UINT8_T *pdata, UINT8_T len)
{
    UINT8_T *alloc_buf = NULL;

    alloc_buf = (UINT8_T *)tal_malloc(7 + len);
    if (alloc_buf == NULL) {
        TAL_PR_ERR("malloc failed");
        return;
    }

    alloc_buf[0] = 0x55;
    alloc_buf[1] = 0xaa;
    alloc_buf[2] = 0x10;
    alloc_buf[3] = cmd;
    alloc_buf[4] = len>>8;
    alloc_buf[5] = len;

    memcpy(alloc_buf + 6, pdata,len);
    alloc_buf[6 + len] = tal_util_check_sum8(alloc_buf, 6 + len);
    tuya_ble_accessory_uart_send_data(port_id, alloc_buf,7 + len);

    tal_free(alloc_buf);
}

VOID_T tuya_ble_accessory_ota_data_from_ble_handler(UINT8_T port_id, UINT16_T cmd, UINT8_T*recv_data, UINT32_T recv_len)
{
    UINT8_T *uart_data_buffer = NULL;
    UINT16_T uart_data_len = 0;

    if (cmd == FRM_ACCESSORY_OTA_DATA_REQ) {
        uart_data_buffer = (UINT8_T*)tal_malloc(recv_len + 7);
        if (uart_data_buffer == NULL) {
            TAL_PR_ERR("uart_data_buffer malloc failed.");
            return;
        }
    } else {
        memset(accessory_data_buffer, 0, SIZEOF(accessory_data_buffer));
        uart_data_buffer = accessory_data_buffer;
    }

    uart_data_buffer[0] = 0x55;
    uart_data_buffer[1] = 0xAA;
    uart_data_buffer[2] = 0x10;
    switch (cmd) {
        case TUYA_BLE_ACCESSORY_OTA_REQ: {
            uart_data_buffer[3] = TUYA_BLE_ACCESSORY_UART_CMD_OTA_REQUEST;
            uart_data_buffer[4] = 0;
            uart_data_buffer[5] = 3;
            uart_data_buffer[6] = recv_data[0];
            uart_data_buffer[7] = TUYA_BLE_ACCESSORY_OTA_DATA_LENGTH_MAX>>8;
            uart_data_buffer[8] = (UINT8_T)TUYA_BLE_ACCESSORY_OTA_DATA_LENGTH_MAX;
            uart_data_len = 9;
        } break;
        case TUYA_BLE_ACCESSORY_OTA_FILE_INFO: {
            uart_data_buffer[3] = TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_INFO;
            uart_data_buffer[4] = 0;
            uart_data_buffer[5] = 37;
            memcpy(uart_data_buffer + 6, recv_data, 37);
            uart_data_len = 43;
        } break;
        case TUYA_BLE_ACCESSORY_OTA_FILE_OFFSET_REQ: {
            uart_data_buffer[3] = TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_OFFSET;
            uart_data_buffer[4] = 0;
            uart_data_buffer[5] = 5;
            memcpy(uart_data_buffer + 6, recv_data, 5);
            uart_data_len = 11;
        } break;
        case TUYA_BLE_ACCESSORY_OTA_DATA: {
            uart_data_buffer[3] = TUYA_BLE_ACCESSORY_UART_CMD_OTA_DATA;
            uart_data_buffer[4] = (recv_len>>8)&0xff;
            uart_data_buffer[5] = recv_len&0xff;
            memcpy(uart_data_buffer + 6, recv_data, recv_len);
            uart_data_len = 6+recv_len;
        } break;
        case TUYA_BLE_ACCESSORY_OTA_END: {
            uart_data_buffer[3] = TUYA_BLE_ACCESSORY_UART_CMD_OTA_END;
            uart_data_buffer[4] = 0;
            uart_data_buffer[5] = 1;
            uart_data_buffer[6] = recv_data[0];
            uart_data_len = 7;
        } break;
        default: {
        } break;
    }

    uart_data_buffer[uart_data_len] = tal_util_check_sum8(uart_data_buffer, uart_data_len);

    tuya_ble_accessory_uart_send_data(port_id, uart_data_buffer, uart_data_len + 1);

    // TAL_PR_HEXDUMP_DEBUG("mcu ota uart send data : ", uart_data_buffer, uart_data_len + 1);

    if (cmd == FRM_OTA_DATA_REQ) {
        tal_free(uart_data_buffer);
    }
}

STATIC VOID_T tuya_ble_accessory_ota_data_from_uart_handler(UINT8_T port_id, UINT8_T cmd, UINT8_T *data_buffer, UINT16_T data_len)
{
    STATIC UINT8_T ble_data_len = 0;
    UINT16_T ble_cmd = 0;
    tuya_ble_connect_status_t currnet_connect_status;
    tuya_ble_accessory_id_info_t id_info;

    currnet_connect_status = tuya_ble_connect_status_get();
    if (currnet_connect_status != BONDING_CONN) {
        TAL_PR_ERR("tuya_ble_uart_common_mcu_ota_process FAILED.");
        return;
    }

    id_info.id_type = 0;
    id_info.id_len = 2;
    id_info.short_id = accessory_info[port_id].short_id;

    memset(accessory_data_buffer, 0, SIZEOF(accessory_data_buffer));
    ble_data_len = 0;

    accessory_data_buffer[0] = id_info.id_type;
    accessory_data_buffer[1] = id_info.id_len;
    accessory_data_buffer[2] = id_info.short_id>>8;
    accessory_data_buffer[3] = id_info.short_id;
    switch (cmd) {
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_REQUEST: {
            accessory_data_buffer[4] = data_buffer[1]; //flag
            accessory_data_buffer[5] = 3;
            accessory_data_buffer[6] = data_buffer[0]; //type
            memcpy(&accessory_data_buffer[7], data_buffer + 2, 6);
            ble_data_len = 13;
            ble_cmd = FRM_ACCESSORY_OTA_START_RESP;
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_INFO: {

            memcpy(&accessory_data_buffer[4], data_buffer, 26);
            ble_data_len = 30;
            ble_cmd = FRM_ACCESSORY_OTA_FILE_INFOR_RESP;
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_OFFSET: {
            memcpy(&accessory_data_buffer[4], data_buffer, 5);
            ble_data_len = 9;
            ble_cmd = FRM_ACCESSORY_OTA_FILE_OFFSET_RESP;
            accessory_mcu_ota_flag = 1;
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_DATA: {

            memcpy(&accessory_data_buffer[4], data_buffer, 2);
            ble_data_len = 6;
            ble_cmd = FRM_ACCESSORY_OTA_DATA_RESP;
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_END: {
            memcpy(&accessory_data_buffer[4], data_buffer, 2);
            ble_data_len = 6;
            ble_cmd = FRM_ACCESSORY_OTA_END_RESP;
        } break;
        default: {
        } break;
    };

    if (ble_data_len > 0) {
        tuya_ble_comm_data_send(ble_cmd, 0, accessory_data_buffer, ble_data_len, ENCRYPTION_MODE_SESSION_KEY);
    }
}

STATIC VOID_T tuya_ble_accessory_uart_cmd_handup(UINT8_T port_id, UINT8_T*p_data, UINT16_T data_len)
{
    UINT8_T buf[16] = {0};

    tuya_ble_rand_generator(accessory_srand, 6);
    buf[0] = 0x00;
    memcpy(buf + 1, accessory_srand, 6);
    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_HANDUP, buf, 7);
    return;
}

STATIC VOID_T tuya_ble_accesory_uart_cmd_device_info_report(UINT8_T port_id, UINT8_T *p_data, UINT16_T data_len)
{
    UINT8_T *p_accessory_fw_info = NULL;
    tuya_ble_accessory_connection_info_t temp_accessoty_connection_info = {0};
    UINT8_T ret = 0;

    if ((p_data[0] != DEVICE_ID_LEN) || (data_len < 34) || (p_data[17] > 2) || ((p_data[17]==0) && (p_data[18] != 8)) ||((p_data[17]==1) && (p_data[18] != 16))) {
        ret = 1;
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT, &ret, 1);
        return;
    }

    temp_accessoty_connection_info.device_id_len = p_data[0];
    memcpy(temp_accessoty_connection_info.device_id, p_data + 1, DEVICE_ID_LEN);

    temp_accessoty_connection_info.pid_type = p_data[17];
    temp_accessoty_connection_info.pid_len = p_data[18];
    memcpy(temp_accessoty_connection_info.common_pid, p_data + 19, temp_accessoty_connection_info.pid_len);

    temp_accessoty_connection_info.accessory_fw_info_len = p_data[19 + temp_accessoty_connection_info.pid_len];
    if ((temp_accessoty_connection_info.accessory_fw_info_len % 7) != 0) {
        ret = 2;
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT, &ret, 1);
        return;
    }

    p_accessory_fw_info = (UINT8_T *)tal_malloc(temp_accessoty_connection_info.accessory_fw_info_len);
    if (p_accessory_fw_info==NULL) {
        ret = 3;
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT, &ret, 1);
        return;
    } else {
        memcpy(p_accessory_fw_info, p_data + 20 + temp_accessoty_connection_info.pid_len, temp_accessoty_connection_info.accessory_fw_info_len);
        temp_accessoty_connection_info.p_accessory_fw_info = p_accessory_fw_info;
    }

    ret = tuya_ble_accessory_connection_info_create(port_id, temp_accessoty_connection_info);
    if (ret) {
        ret = 4;
        tal_free((UINT8_T*)p_accessory_fw_info);
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT, &ret, 1);
        return;
    }

    tuya_ble_accessory_connection_info_report(&accessory_info[port_id]); //BLE->APP

    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT, &ret, 1);

    tuya_ble_accessory_uart_cmd_send_work_mode(port_id);

    tal_free((UINT8_T*)p_accessory_fw_info);
}

VOID_T tuya_ble_accessory_disconnect_handler(VOID_T)
{
    UINT8_T i = 0;
    for (i = 0; i < ACCESSORY_MOUNT_MAX; i++) {
        accessory_info[i].connect_status = 0;
        tuya_ble_accessory_connection_info_report(&accessory_info[i]); //BLE->APP
    }
}

VOID_T tuya_ble_accessory_uart_cmd_send_work_mode(UINT8_T port_id)
{
    tuya_ble_connect_status_t ble_connect_status;
    EM_ACCESSORY_WORK_STATUS accessory_work_status;

    ble_connect_status = tuya_ble_connect_status_get();

    if (accessory_info[port_id].active_status != 1) {
        accessory_work_status = INACTIVE;
    } else if (ble_connect_status == BONDING_CONN) {
        accessory_work_status = ACTIVATED_CONNECTED;
    } else if ((ble_connect_status == BONDING_UNCONN) || (ble_connect_status == BONDING_UNAUTH_CONN)) {
        accessory_work_status = ACTIVATED_UNCONNECTED;
        accessory_mcu_ota_flag = 0;
    } else {
        accessory_work_status = INACTIVE;
    }
    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_WORK_STATE_SYNC, &accessory_work_status, 1);
}

VOID_T tuya_ble_accessory_uart_cmd_dp_data_send_to_accessory(UINT8_T port_id, UINT32_T sn, UINT8_T *p_dp_data, UINT16_T data_len)
{
    UINT8_T *buf = NULL;
    UINT16_T index = 0;

    if (data_len > 252 || port_id > ACCESSORY_MOUNT_MAX) {
        return;
    }

    buf = (UINT8_T *)tal_malloc(4 + data_len);
    if (buf == NULL) {
        TAL_PR_ERR("malloc failed");
        return;
    }

    buf[index++] = (sn>>24)&0xff;
    buf[index++] = (sn>>16)&0xff;
    buf[index++] = (sn>>8)&0xff;
    buf[index++] = sn&0xff;

    memcpy(buf + index, p_dp_data, data_len);
    index += data_len;

    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_SNED, buf, index);

    tal_free(buf);
}

VOID_T tuya_ble_accessory_uart_cmd_dp_query(UINT8_T port_id, UINT8_T *p_data, UINT16_T data_len)
{
    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_QUERY, p_data, data_len);
}

STATIC VOID_T tuya_ble_accessory_uart_cmd_dp_report(UINT8_T port_id, UINT8_T *p_data, UINT16_T data_len)
{
    UINT32_T SN;
    UINT8_T flag, time_type;
    tuya_ble_accessory_id_info_t accessory_id_info;
    UINT16_T dp_data_len;
    UINT8_T *dp_data = NULL;
    UINT32_T timestamp;
    INT32_T timezone;
    UINT8_T ret;

    if (port_id > ACCESSORY_MOUNT_MAX) {
        return;
    }
    if (data_len > 250) {
        return;
    }

    dp_data = (UINT8_T *)tal_malloc(data_len);
    if (dp_data == NULL) {
        TAL_PR_ERR("malloc failed");
        return;
    }

    if (accessory_info[port_id].active_status) {
        accessory_id_info.id_len   = 2;
        accessory_id_info.id_type  = 0;
        accessory_id_info.short_id = accessory_info[port_id].short_id;
        tal_util_reverse_byte((UINT8_T *)&accessory_id_info.short_id, SIZEOF(UINT16_T));

        SN = ((UINT32_T)p_data[0] << 24) | ((UINT32_T)p_data[1] << 16) | ((UINT32_T)p_data[2] << 8) | p_data[3];
        flag = p_data[4];
        time_type = p_data[5];

        if (time_type == 0xff) {
            memcpy(dp_data, p_data + 6, data_len - 6);
            dp_data_len = data_len-6;
            TAL_PR_HEXDUMP_DEBUG("Dp Report To App: ", dp_data, dp_data_len);
            ret = tuya_ble_dp_data_with_src_type_send(SN, DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT, DP_SEND_TYPE_ACTIVE,flag,DP_SEND_WITH_RESPONSE, 4, (UINT8_T*)&accessory_id_info, dp_data, dp_data_len);
        } else if (time_type == 0x00) {
            tuya_ble_rtc_get_timestamp(&timestamp, &timezone);
            TAL_PR_DEBUG("timestamp: %d", timestamp);
            tal_util_reverse_byte((UINT8_T*)&timestamp, 4);
            memcpy(dp_data, p_data + 6, data_len - 6);
            dp_data_len = data_len - 6;
            TAL_PR_HEXDUMP_DEBUG("Dp Report To App: ", dp_data, dp_data_len);
            ret = tuya_ble_dp_data_with_src_type_and_time_send(SN, DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT, flag, DP_TIME_TYPE_UNIX_TIMESTAMP, (UINT8_T*)&timestamp, 4, (UINT8_T*)&accessory_id_info, dp_data, dp_data_len);
        } else if (time_type == 0x01) {
            ret = 1;
        } else {
            //error
            ret = 1;
        }
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_REPORT, &ret, 1);
    } else {
        ret = 2;
        tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_REPORT, &ret, 1);
    }

    tal_free(dp_data);
}

STATIC VOID_T tuya_ble_accessory_uart_cmd_query_module_mac(UINT8_T port_id, UINT8_T *p_data, UINT16_T data_len)
{
    UINT8_T mac[6] = {0};

    memcpy(mac, tuya_ble_current_para.auth_settings.mac, 6);

    tal_util_reverse_byte(mac, 6);

    tuya_ble_accessory_uart_protocol_send(port_id, TUYA_BLE_ACCESSORY_UART_CMD_QUERY_MODULE_MAC, mac, 6);
}

STATIC VOID_T tuya_ble_accessory_custom_evt_handler(INT32_T evt_id, VOID_T *data)
{
    tuya_ble_accessory_connection_info_t *p_connection_info;

    switch (evt_id) {
        case TUYA_BLE_ACCESSORY_EVT_CONNECTE_STATUS: {
            p_connection_info = (tuya_ble_accessory_connection_info_t*)data;
            tuya_ble_accessory_uart_cmd_send_work_mode(p_connection_info->port_id);
        } break;
        default: {
        } break;
    }
}

tuya_ble_status_t tuya_ble_accessory_send_custom_event(INT32_T evt_id, UINT8_T *p_data)
{
    tuya_ble_custom_evt_t event;

    event.evt_id = evt_id;
    event.data = p_data;
    event.custom_event_handler = (VOID_T (*)(INT32_T, VOID_T *))tuya_ble_accessory_custom_evt_handler;
    if (tuya_ble_custom_event_send(event)) {
        TUYA_BLE_LOG_ERROR("accessory custom event send event error!");
        return TUYA_BLE_ERR_NO_EVENT;
    }
    return TUYA_BLE_SUCCESS;
}

__TUYA_BLE_WEAK UINT32_T tuya_ble_accessory_product_test_rsp(UINT8_T channel, UINT16_T cmdId, UINT8_T* buf, UINT16_T size)
{
    return 0;
}

UINT8_T tuya_ble_accessory_mcu_ota_status(VOID_T)
{
    return accessory_mcu_ota_flag;
}

VOID_T tuya_ble_accessory_uart_protocol_process(UINT8_T port_id, UINT8_T *p_in_data, UINT16_T in_len)
{
    UINT8_T *data_buffer = p_in_data+6;
    UINT8_T cmd = p_in_data[3];
    UINT8_T version = p_in_data[2];
    UINT16_T data_len = (p_in_data[4]<<8) + p_in_data[5];

    if (port_id > ACCESSORY_MOUNT_MAX) {
        return;
    }

    if (version != 0x10) {
        return;
    }
    TAL_PR_HEXDUMP_DEBUG("accessory:", p_in_data, in_len);
    switch (cmd) {
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_REQUEST:
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_INFO:
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_OFFSET:
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_DATA:
        case TUYA_BLE_ACCESSORY_UART_CMD_OTA_END: {
            tuya_ble_accessory_ota_data_from_uart_handler(port_id, cmd, data_buffer, data_len);
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_HANDUP: {
            tuya_ble_accessory_uart_cmd_handup(port_id, data_buffer, data_len);
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT: {
            tuya_ble_accesory_uart_cmd_device_info_report(port_id, data_buffer, data_len);
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_REPORT: {
            tuya_ble_accessory_uart_cmd_dp_report(port_id, data_buffer, data_len);
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_QUERY_MODULE_MAC: {
            tuya_ble_accessory_uart_cmd_query_module_mac(port_id, data_buffer, data_len);
        } break;
        case TUYA_BLE_ACCESSORY_UART_CMD_FATORY_CMD: {
            tuya_ble_accessory_product_test_rsp(1, TUYA_APP_PRODUCT_TEST_ACCESSORY_PASSTHROUGH, data_buffer, data_len);
        } break;
        default: {
        } break;
    }
}

#endif

