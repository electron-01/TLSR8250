/**
 * @file tuya_ble_protocol_callback.h
 * @brief This is tuya_ble_protocol_callback file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_BLE_PROTOCOL_CALLBACK_H__
#define __TUYA_BLE_PROTOCOL_CALLBACK_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
//PID - product id, DID - device id
//FIR - firmware, FVER - firmware version, HVER - hardware version
#define TY_DEVICE_NAME        "TyOS"
#define TY_DEVICE_PID         "3aubjk7p" //oem no use
#define TY_DEVICE_MAC         "DC234DAFCC96"
#define TY_DEVICE_DID         "tuya07d9624bc2e5" //16Bytes
#define TY_DEVICE_AUTH_KEY    "fs1RKoeL4W3AYNFSCRgCW2ev6tOFmeer" //32Bytes

#define TY_ADV_INTERVAL       100   //range: 20~10240ms
#define TY_CONN_INTERVAL_MIN  180   //range: 7.5~4000ms
#define TY_CONN_INTERVAL_MAX  200   //range: 7.5~4000ms

//event id
typedef enum {
    APP_EVT_0,
    APP_EVT_1,
    APP_EVT_2,
    APP_EVT_3,
    APP_EVT_4,
} custom_evtid_t;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
#pragma pack(1)
typedef struct {
    UINT32_T len;
    UINT8_T  value[];
} custom_evt_data_t;
#pragma pack()

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

/**
 * @brief
 *
 * @param[in] param1:
 * @param[in] param2:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_protocol_init(VOID_T);

/**
 * @brief tuya_ble_disconnect_and_reset_timer_init
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_disconnect_and_reset_timer_init(VOID_T);

/**
 * @brief tuya_ble_update_conn_param_timer_init
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_update_conn_param_timer_init(VOID_T);

/**
 * @brief tuya_ble_disconnect_and_reset_timer_start
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_disconnect_and_reset_timer_start(VOID_T);

/**
 * @brief tuya_ble_update_conn_param_timer_start
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_update_conn_param_timer_start(VOID_T);

/**
 * @brief tuya_ble_custom_evt_send
 *
 * @param[in] evtid: evtid
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_custom_evt_send(custom_evtid_t evtid);

/**
 * @brief tuya_ble_custom_evt_send_with_data
 *
 * @param[in] evtid: evtid
 * @param[in] buf: buf
 * @param[in] size: size
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_custom_evt_send_with_data(custom_evtid_t evtid, VOID_T* buf, UINT32_T size);


#ifdef __cplusplus
}
#endif

#endif /* __TUYA_BLE_PROTOCOL_CALLBACK_H__ */

