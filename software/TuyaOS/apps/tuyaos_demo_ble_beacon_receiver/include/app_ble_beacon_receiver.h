/**
 * @file app_ble_beacon_receiver.h
 * @brief This is app_ble_beacon_receiver file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_BLE_BEACON_RECEIVER_H__
#define __APP_BLE_BEACON_RECEIVER_H__

#include "tuya_cloud_types.h"
#include "tal_ble_beacon_remoter.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define REMOTER_NUM 5

#define CMD_KEY_VALUE 0x01
#define CMD_BONDING 0x02
#define CMD_UNBONDING 0x03
#define CMD_SWITCH 0x04
//#define CMD_ 0x05
//#define CMD_ 0x06
//#define CMD_ 0x07
//#define CMD_ 0x08
//#define CMD_ 0x09
//#define CMD_ 0x0A
//#define CMD_ 0x0B
//#define CMD_ 0x0C
//#define CMD_ 0x0D
//#define CMD_ 0x0E
//#define CMD_ 0x0F
//#define CMD_ 0x10
//#define CMD_ 0x11
//#define CMD_ 0x12
//#define CMD_ 0x13
//#define CMD_ 0x14
//#define CMD_ 0x15
//#define CMD_ 0x16
//#define CMD_ 0x17
//#define CMD_ 0x18
//#define CMD_ 0x19
//#define CMD_ 0x20
//#define CMD_ 0x21
//#define CMD_ 0x22
//#define CMD_ 0x23
//#define CMD_ 0x24
//#define CMD_ 0x25
//#define CMD_ 0x26
//#define CMD_ 0x27
//#define CMD_ 0x28
//#define CMD_ 0x29

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


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
VOID_T app_receiver_init(VOID_T);

/**
 * @brief app_receiver_data_handler
 *
 * @param[in] buf: buf
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_receiver_data_handler(VOID_T* buf);

/**
 * @brief app_receiver_cmd_parser
 *
 * @param[in] idx: idx
 * @param[in] p_mac: p_mac
 * @param[in] TAL_BLE_BEACON_REMOTE_DP_DATA_T: dp data
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_receiver_cmd_parser(UINT8_T idx, UINT8_T* p_mac, TAL_BLE_BEACON_REMOTE_DP_DATA_T dp_data);

/**
 * @brief app_receiver_info_get
 *
 * @param[in] data_unit[]: data_unit[]
 * @param[in] *unit_num: *unit_num
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_receiver_info_get(tuya_ble_remoter_proxy_auth_data_unit_t data_unit[], UINT8_T *unit_num);

/**
 * @brief app_info_save_event_handler
 *
 * @param[in] none: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_info_save_event_handler(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_BLE_BEACON_RECEIVER_H__ */

