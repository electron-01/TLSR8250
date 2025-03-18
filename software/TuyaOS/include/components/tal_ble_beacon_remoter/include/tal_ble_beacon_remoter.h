/**
 * @file tal_ble_beacon_remoter.h
 * @brief This is tal_ble_beacon_remoter file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TAL_BLE_BEACON_REMOTER_H__
#define __TAL_BLE_BEACON_REMOTER_H__

#include "tuya_cloud_types.h"
#include "tuya_ble_type.h"
#include "tal_bluetooth_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
typedef enum {
    TAL_BLE_BEACON_LOCAL_CMD = 0,
    TAL_BLE_BEACON_CLOUD_CMD,
} TAL_BLE_BEACON_ACT_TYPE_E;

#define DP_DATA_LEN 12

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
#pragma pack(1)
typedef struct {
    UINT8_T  dp_id;
    UINT8_T  dp_len:4;
    UINT8_T  dp_type:4;
    UINT8_T  key_id;
    UINT8_T  group_id;
    UINT8_T  cmd;
    UINT8_T  cmd_data[4];
    UINT8_T  mic[3];
    UINT8_T  mic_reserve;
} TAL_BLE_BEACON_REMOTE_DP_DATA_T;

typedef struct {
    UINT8_T  len;
    UINT8_T  type;
    UINT8_T  uuid[2];
    UINT8_T  frame_control[2];
    UINT8_T  dp_control;
    UINT8_T  sn[4];
    TAL_BLE_BEACON_REMOTE_DP_DATA_T dp_data;
} TAL_BLE_BEACON_REMOTE_SERVICE_DATA_T;

typedef struct {
    UINT8_T  mac[6];
    UINT8_T  key_id;
    UINT8_T  s1[2];
    UINT8_T  s3[16];
    UINT32_T sn;
    UINT8_T  valid;
    UINT8_T  is_auth;
    UINT8_T  reserve[1]; // Note the 4-byte alignment problem
} TAL_BLE_BEACON_REMOTE_INFO_T;

typedef struct {
    TAL_BLE_BEACON_REMOTE_INFO_T *info;
    UINT8_T device_num_max;
}TAL_BLE_BEACON_DEVICE_T;
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
 * @param[in] p_adv_report: adv data
 * @param[in] p_service_data: adv section of service_data
 * @param[in] mac: tell user mac address
 * @param[in] index: tell user the index of mac address in device list
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_service_data_parser(TAL_BLE_ADV_REPORT_T *p_adv_report, TAL_BLE_BEACON_REMOTE_SERVICE_DATA_T *p_service_data, UINT8_T *mac, UINT8_T *index);

/**
 * @brief
 *
 * @param[in] info: the start address of remote info list index
 * @param[in] max: the max number of device
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_init(TAL_BLE_BEACON_REMOTE_INFO_T *info, UINT8_T max);

/**
 * @brief find the index of device in device info list
 *
 * @param[in] data: the device info of which need authentication
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tal_ble_beacon_remoter_find(UINT8_T* p_mac);

/**
 * @brief app authentication result response
 *
 * @param[in] data: the device info of which need authentication
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_auth_rsp(tuya_ble_remoter_proxy_auth_data_rsp_t data);

/**
 * @brief add remoter, support cycle add. the last device which wait add will replace last add device
 *
 * @param[in] mac: the mac address of delete device
 * @param[in] cmd_addr:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_group_add(UINT8_T* p_mac, UINT8_T* p_cmd_data);

/**
 * @brief delete deivce from APP
 *
 * @param[in] cmd_type: cmd from cloud or local
 * @param[in] mac: the mac address of delete device
 * @param[in] cmd_addr:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_group_delete(TAL_BLE_BEACON_ACT_TYPE_E cmd_type, UINT8_T* p_mac, UINT8_T* p_cmd_data);

/**
 * @brief remoter group subcribe
 *
 * @param[in] mac: targe mac address
 * @param[in] id: group id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_group_subscribe(UINT8_T* p_mac, UINT8_T id);

/**
 * @brief remoter group info query
 *
 * @param[in] mac: target mac addree
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_remoter_group_query(UINT8_T* p_mac);

/**
 * @brief auth flag set 0
 *
 * @param[in] none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tal_ble_beacon_device_auth_reset(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __TAL_BLE_BEACON_REMOTER_H__ */

