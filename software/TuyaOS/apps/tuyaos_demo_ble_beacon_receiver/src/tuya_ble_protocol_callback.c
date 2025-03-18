/**
 * @file tuya_ble_protocol_callback.c
 * @brief This is tuya_ble_protocol_callback file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "string.h"

#include "tal_memory.h"
#include "tal_log.h"
#include "tal_rtc.h"
#include "tal_utc.h"
#include "tal_bluetooth.h"
#include "tal_util.h"
#include "tal_sdk_test.h"
#include "tal_flash.h"

#include "tuya_ble_api.h"
#include "tuya_ble_ota.h"
#include "tuya_ble_bulkdata_demo.h"
#include "tuya_ble_protocol_callback.h"
#include "tuya_ble_main.h"
#include "tuya_sdk_callback.h"

#include "app_dp_parser.h"
#include "app_led.h"
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
STATIC CONST UINT8_T auth_key_test[]  = TY_DEVICE_AUTH_KEY;
STATIC CONST UINT8_T device_id_test[] = TY_DEVICE_DID;

STATIC tuya_ble_device_param_t tuya_ble_protocol_param = {
#if (TUYA_SDK_DEBUG_MODE)
    .use_ext_license_key = 1, //1-info in tuya_ble_protocol_callback.h, 0-auth info
    .device_id_len       = DEVICE_ID_LEN,
#else
    .use_ext_license_key = 0,
    .device_id_len       = 0,
#endif
    .p_type              = TUYA_BLE_PRODUCT_ID_TYPE_PID,
#if (TUYA_BLE_PROD_SUPPORT_OEM_TYPE == TUYA_BLE_PROD_OEM_TYPE_NONE)
    .product_id_len      = 8,
#else
    .product_id_len      = 0,
#endif
    .adv_local_name_len  = 4,
};

STATIC tuya_ble_timer_t disconnect_timer;
STATIC tuya_ble_timer_t update_conn_param_timer;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




STATIC VOID_T tuya_ble_protocol_callback(tuya_ble_cb_evt_param_t* event)
{
    switch (event->evt) {
        case TUYA_BLE_CB_EVT_CONNECT_STATUS: {
            if (event->connect_status == BONDING_CONN) {
                TAL_PR_INFO("bonding and connecting");

                if (tuya_ble_ota_get_status() < TUYA_BLE_OTA_REQ) {
                    tuya_ble_update_conn_param_timer_start();
                }
            }
        } break;

        case TUYA_BLE_CB_EVT_DP_DATA_RECEIVED: {
            app_dp_parser(event->dp_received_data.p_data, event->dp_received_data.data_len);
        } break;

        case TUYA_BLE_CB_EVT_TIME_STAMP: {
            UINT32_T timestamp_s = 0;
            UINT32_T timestamp_ms = 0;
            tal_util_str_intstr2int((VOID_T*)event->timestamp_data.timestamp_string, 10, &timestamp_s);
            tal_util_str_intstr2int((VOID_T*)(event->timestamp_data.timestamp_string+10), 3, &timestamp_ms);

            tal_rtc_time_set(timestamp_s);
            tal_utc_set_time_zone(event->timestamp_data.time_zone);

            TAL_PR_INFO("TUYA_BLE_CB_EVT_TIME_STAMP - time_zone: %d", event->timestamp_data.time_zone);
            TAL_PR_INFO("TUYA_BLE_CB_EVT_TIME_STAMP - timestamp: %d", timestamp_s);

            tuya_ble_remoter_proxy_auth_data_t data = {0};
            tuya_ble_remoter_proxy_auth_data_unit_t data_unit[REMOTER_NUM] = {{0}};
            UINT8_T sum = 0;
            app_receiver_info_get(data_unit, &sum);

            data.p_data = (UINT8_T *)data_unit;
            data.num = sum;
            tuya_ble_remoter_proxy_auth(data);
        } break;

        case TUYA_BLE_CB_EVT_UNBOUND: {
            TAL_PR_INFO("TUYA_BLE_CB_EVT_UNBOUND");
            tal_ble_beacon_device_auth_reset();
        } break;

        case TUYA_BLE_CB_EVT_ANOMALY_UNBOUND: {
            TAL_PR_INFO("TUYA_BLE_CB_EVT_ANOMALY_UNBOUND");
            tal_ble_beacon_device_auth_reset();
        } break;

        case TUYA_BLE_CB_EVT_DEVICE_RESET: {
            TAL_PR_INFO("TUYA_BLE_CB_EVT_DEVICE_RESET");
            tal_ble_beacon_device_auth_reset();
        } break;

        case TUYA_BLE_CB_EVT_UNBIND_RESET_RESPONSE: {
            TAL_PR_INFO("TUYA_BLE_CB_EVT_UNBIND_RESET_RESPONSE");
            tal_ble_beacon_device_auth_reset();
        } break;

        case TUYA_BLE_CB_EVT_DP_QUERY: {
//            TAL_PR_HEXDUMP_INFO("TUYA_BLE_CB_EVT_DP_QUERY", event->dp_query_data.p_data, event->dp_query_data.data_len);
            UINT8_T led_on = 1;
            app_dp_report(WR_BASIC_LED, &led_on, SIZEOF(UINT8_T));
        } break;

#if defined(TUYA_BLE_FEATURE_OTA_ENABLE) && (TUYA_BLE_FEATURE_OTA_ENABLE == 1)
        case TUYA_BLE_CB_EVT_OTA_DATA: {
            tuya_ble_ota_handler(&event->ota_data);
        } break;
#endif

#if defined(TUYA_BLE_FEATURE_BULKDATA_ENABLE) && (TUYA_BLE_FEATURE_BULKDATA_ENABLE == 1)
        case TUYA_BLE_CB_EVT_BULK_DATA: {
            TAL_PR_INFO("TUYA_BLE_CB_EVT_BULK_DATA");
            tuya_ble_bulk_data_demo_handler(&event->bulk_req_data);
        } break;
#endif

        case TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID: {
        } break;

        case TUYA_BLE_CB_EVT_REMOTER_PROXY_AUTH_RESP: {
            tal_ble_beacon_remoter_auth_rsp(event->remoter_proxy_auth_data_rsp);
        } break;

        case TUYA_BLE_CB_EVT_REMOTER_GROUP_SET: {
            TAL_PR_HEXDUMP_DEBUG("REMOTER_GROUP_SET", event->remoter_group_set_data.mac, sizeof(tuya_ble_remoter_group_set_data_t));
            tal_ble_beacon_remoter_group_subscribe(event->remoter_group_set_data.mac, event->remoter_group_set_data.id);

        } break;

        case TUYA_BLE_CB_EVT_REMOTER_GROUP_DELETE: {
            TAL_PR_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_REMOTER_GROUP_DELETE", event->remoter_group_delete_data.mac, sizeof(tuya_ble_remoter_group_delete_data_t));
            tal_ble_beacon_remoter_group_delete(TAL_BLE_BEACON_CLOUD_CMD, event->remoter_group_delete_data.mac, &event->remoter_group_delete_data.id);
        } break;

        case TUYA_BLE_CB_EVT_REMOTER_GROUP_GET: {
            TAL_PR_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_REMOTER_GROUP_GET", event->remoter_group_get_data.mac, sizeof(tuya_ble_remoter_group_get_data_t));
            tal_ble_beacon_remoter_group_query(event->remoter_group_get_data.mac);
        } break;
        default: {
            TAL_PR_INFO("tuya_ble_protocol_callback Unprocessed event type 0x%04x", event->evt);
        } break;
    }

#if defined(TUYA_SDK_TEST) && (TUYA_SDK_TEST == 1)
    tal_sdk_test_ble_protocol_callback(event);
#endif

#if TUYA_BLE_USE_OS
    tuya_ble_event_response(event);
#endif
}

VOID_T tuya_ble_protocol_init(VOID_T)
{
    tuya_ble_protocol_param.firmware_version = tal_common_info.firmware_version,
    tuya_ble_protocol_param.hardware_version = tal_common_info.hardware_version,
    memcpy(tuya_ble_protocol_param.device_id,       device_id_test, DEVICE_ID_LEN);
    memcpy(tuya_ble_protocol_param.auth_key,        auth_key_test,  AUTH_KEY_LEN);
    memcpy(tuya_ble_protocol_param.mac_addr_string, TY_DEVICE_MAC,  MAC_STRING_LEN);
    memcpy(tuya_ble_protocol_param.product_id,      TY_DEVICE_PID,  tuya_ble_protocol_param.product_id_len);
    memcpy(tuya_ble_protocol_param.adv_local_name,  TY_DEVICE_NAME, tuya_ble_protocol_param.adv_local_name_len);
    tuya_ble_sdk_init(&tuya_ble_protocol_param);

    tuya_ble_callback_queue_register(tuya_ble_protocol_callback);

#if defined(TUYA_BLE_FEATURE_OTA_ENABLE) && (TUYA_BLE_FEATURE_OTA_ENABLE == 1)
    tuya_ble_ota_init();
#endif
    tuya_ble_disconnect_and_reset_timer_init();
    tuya_ble_update_conn_param_timer_init();

    extern tuya_ble_parameters_settings_t tuya_ble_current_para;
    TAL_PR_HEXDUMP_INFO("auth key", tuya_ble_current_para.auth_settings.auth_key, AUTH_KEY_LEN);
    TAL_PR_HEXDUMP_INFO("device id", tuya_ble_current_para.auth_settings.device_id, DEVICE_ID_LEN);
}

STATIC VOID_T tuya_ble_disconnect_and_reset_timer_cb(tuya_ble_timer_t timer)
{
    tuya_ble_gap_disconnect();
    tuya_ble_device_delay_ms(100);
    tuya_ble_device_reset();
}

STATIC VOID_T tuya_ble_update_conn_param_timer_cb(tuya_ble_timer_t timer)
{
    if (tuya_ble_connect_status_get() == BONDING_CONN) {
        TAL_BLE_PEER_INFO_T peer_info = {0};
        peer_info.conn_handle = tuya_app_get_conn_handle();
        TAL_BLE_CONN_PARAMS_T conn_param = {0};
        conn_param.min_conn_interval = TY_CONN_INTERVAL_MIN*4/5;
        conn_param.max_conn_interval = TY_CONN_INTERVAL_MAX*4/5;
        conn_param.latency = 0;
        conn_param.conn_sup_timeout = 6000/10;
        conn_param.connection_timeout = 0;
        tal_ble_conn_param_update(peer_info, &conn_param);
    }
}

VOID_T tuya_ble_disconnect_and_reset_timer_init(VOID_T)
{
    tuya_ble_timer_create(&disconnect_timer, 1000, TUYA_BLE_TIMER_SINGLE_SHOT, tuya_ble_disconnect_and_reset_timer_cb);
}

VOID_T tuya_ble_update_conn_param_timer_init(VOID_T)
{
    tuya_ble_timer_create(&update_conn_param_timer, 1000, TUYA_BLE_TIMER_SINGLE_SHOT, tuya_ble_update_conn_param_timer_cb);
}

VOID_T tuya_ble_disconnect_and_reset_timer_start(VOID_T)
{
    tuya_ble_timer_start(disconnect_timer);
}

VOID_T tuya_ble_update_conn_param_timer_start(VOID_T)
{
    tuya_ble_timer_start(update_conn_param_timer);
}

STATIC VOID_T tuya_ble_app_data_process(INT32_T evt_id, VOID_T *data)
{
    custom_evt_data_t* custom_data = data;

    switch (evt_id) {
        case APP_EVT_0: {
            TAL_BLE_ADV_REPORT_T adv_report;
            UINT8_T index = 0;
            UINT8_T adv_data[31];

            memcpy(&adv_report.peer_addr.type, &custom_data->value[index], 7);
            index += 7;
            adv_report.adv_type = custom_data->value[index++];
            adv_report.rssi = custom_data->value[index++];
            memcpy(adv_data, &custom_data->value[index], custom_data->len - 10);
            index += custom_data->len - 10;
            adv_report.p_data = adv_data;
            adv_report.data_len = custom_data->value[index++];

            app_receiver_data_handler((VOID_T *)&adv_report);
        } break;

        case APP_EVT_1: {
            app_info_save_event_handler();
        } break;

        case APP_EVT_2: {
        } break;

        case APP_EVT_3: {
        } break;

        case APP_EVT_4: {
        } break;

        default: {
        } break;
    }

    if (custom_data != NULL) {
        tal_free((VOID_T*)custom_data);
    }
}

VOID_T tuya_ble_custom_evt_send(custom_evtid_t evtid)
{
    tuya_ble_custom_evt_t custom_evt;

    custom_evt.evt_id = evtid;
    custom_evt.data = NULL;
    custom_evt.custom_event_handler = tuya_ble_app_data_process;

    tuya_ble_custom_event_send(custom_evt);
}

VOID_T tuya_ble_custom_evt_send_with_data(custom_evtid_t evtid, VOID_T* buf, UINT32_T size)
{
    custom_evt_data_t* custom_data = tal_malloc(SIZEOF(custom_evt_data_t) + size);
    if (custom_data) {
        tuya_ble_custom_evt_t custom_evt;

        custom_data->len = size;
        memcpy(custom_data->value, buf, size);

        custom_evt.evt_id = evtid;
        custom_evt.data = custom_data;
        custom_evt.custom_event_handler = tuya_ble_app_data_process;

        tuya_ble_custom_event_send(custom_evt);
    } else {
        TAL_PR_ERR("tuya_ble_custom_evt_send_with_data: malloc failed");
    }
}

