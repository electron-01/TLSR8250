/**
 * @file tuya_ble_accessory_demo.h
 * @brief This is tuya_ble_accessory_demo file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_BLE_ACCESSORY_DEMO_H__
#define __TUYA_BLE_ACCESSORY_DEMO_H__

#include "tuya_cloud_types.h"
#include "tuya_ble_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define ACCESSORY_MOUNT_MAX 1

typedef enum {
    EM_STORAGE_SUCCESS,
    EM_STORAGE_FAILED,
} EM_ACCESSORY_SRORAGE_RESULT;

typedef enum {
    INACTIVE,
    ACTIVATED_UNCONNECTED,
    ACTIVATED_CONNECTED,
} EM_ACCESSORY_WORK_STATUS;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT8_T channle;
    UINT32_T soft_version;
    UINT32_T hard_version;
} accessory_fw_info_t;

typedef struct {
    UINT8_T                    *p_accessory_fw_info;
    UINT8_T                     device_id[DEVICE_ID_LEN];
    UINT8_T                     common_pid[TUYA_BLE_PRODUCT_ID_MAX_LEN];
    tuya_ble_product_id_type_t  pid_type;
    UINT8_T                     pid_len;
    UINT8_T                     accessory_fw_info_len;
    UINT8_T                     device_id_len;
    UINT8_T                     connect_status; //1-connect 0-disconnect
    UINT8_T                     port_id;
    UINT8_T                     active_status; //1-active 0-no active
    UINT16_T                    short_id;
} tuya_ble_accessory_connection_info_t;

/**@brief    Structure about the accessory active information. */
typedef struct {
    UINT32_T info_crc;
    UINT16_T info_len;
    UINT16_T short_id;
    UINT8_T device_id[DEVICE_ID_LEN];
} tuya_ble_accessory_active_info_t;

/**@brief    Structure about the accessory ID information. */
typedef struct {
    UINT8_T id_type;
    UINT8_T id_len;
    UINT16_T short_id;
} tuya_ble_accessory_id_info_t;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern tuya_ble_accessory_connection_info_t accessory_info[ACCESSORY_MOUNT_MAX];

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
/**
 * @brief Function for reporting a connection information change of an accessory.
 *        Called when this event[@TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT]  is received.
 * @param[in] p_accessory_info: Report accessory connection information, For details see the struct of tuya_ble_accessory_connection_info_t.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_connection_info_report(tuya_ble_accessory_connection_info_t *p_accessory_info);

/**
 * @brief Function for storing an accessory activation information.
 *        Called when this event[@TUYA_BLE_CB_EVT_ACCESSORY_ACTIVE_INFO_RECEIVED]  is received.
 * @param[in] p_accessory_active_info: Report accessory active information, For details see the struct of tuya_ble_accessory_active_info_t.
 *
 * @return EM_STORAGE_SUCCESS on success. EM_STORAGE_FAILED on error
 */
UINT32_T tuya_ble_accessory_storage_write_active_info(tuya_ble_accessory_active_info_t *p_accessory_active_info);

/**
 * @brief Function for finding the accessory activation information by device_id.
 *
 * @param[in] device_id: device id.
 *
 * @param[out] p_accessory_active_info: For details see the struct of tuya_ble_accessory_active_info_t.
 *
 * @return TRUE on success. FALSE on error
 */
BOOL_T tuya_ble_accessory_storage_read_active_info_by_device_id(UINT8_T *device_id, tuya_ble_accessory_active_info_t *p_accessory_active_info);

/**
 * @brief Function for creating an accessory connection information.
 *
 * @param[in] app_accessory_info: For details see the struct of tuya_ble_accessory_connection_info_t.
 *
 * @param[in] port_id: prot id.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_connection_info_create(UINT8_T port_id, tuya_ble_accessory_connection_info_t app_accessory_info);

/**
 * @brief Function for deleting an accessory connection information.
 *
 * @param[in] port_id: prot id.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_connection_info_delete(UINT8_T port_id);

/**
 * @brief Function for finding the connection information of the accessory by device_id.
 * @param[in] device_id: device id.
 * @param[out] index: The index of accessory connection information.
 *
 * @return TRUE on success. FALSE on error
 */
BOOL_T tuya_ble_accessory_connection_info_find_by_device_id(UINT8_T* device_id, UINT8_T *index);

/**
 * @brief Function for finding the connection information of the accessory by short_id.
 *
 * @param[in] short_id     short id.
 *
 * @param[out] index     The index of accessory connection information.
 *
 * @return TRUE on success. FALSE on error
 */
BOOL_T tuya_ble_accessory_connection_info_find_by_short_id(UINT16_T short_id, UINT8_T *index);

/**
 * @brief Function for initialize the accessory function.
 *
 * @param[in] NONE
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_init(VOID_T);

/**
 * @brief Function for handling the callback events of the SDK.
 *        Called in a function registered through this function(@tuya_ble_callback_queue_register).
 *
 * @param[in] tuya_ble_cb_evt_param_t* event
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_sdk_cb_event_handler(tuya_ble_cb_evt_param_t* event);


#ifdef __cplusplus
}
#endif

#endif /* __TUYA_BLE_ACCESSORY_DEMO_H__ */

