/**
 * @file tuya_ble_accessory_uart_protocol_handler_demo.h
 * @brief This is tuya_ble_accessory_uart_protocol_handler_demo file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_BLE_ACCESSORY_UART_PROTOCOL_HANDLER_DEMO_H__
#define __TUYA_BLE_ACCESSORY_UART_PROTOCOL_HANDLER_DEMO_H__

#include "tuya_cloud_types.h"
#include "tuya_ble_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TUYA_BLE_ACCESSORY_UART_CMD_HANDUP              0x00
#define TUYA_BLE_ACCESSORY_UART_CMD_DEVICE_INFO_REPORT  0x01
#define TUYA_BLE_ACCESSORY_UART_CMD_WORK_STATE_SYNC     0x02
#define TUYA_BLE_ACCESSORY_UART_CMD_DATA_ENC_REQ        0x03
#define TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_SNED        0x06
#define TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_REPORT      0x07
#define TUYA_BLE_ACCESSORY_UART_CMD_DP_DATA_QUERY       0x08
#define TUYA_BLE_ACCESSORY_UART_CMD_QUERY_MODULE_MAC    0xBE

#define TUYA_BLE_ACCESSORY_UART_CMD_OTA_REQUEST         0xFA
#define TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_INFO       0xFB
#define TUYA_BLE_ACCESSORY_UART_CMD_OTA_FILE_OFFSET     0xFC
#define TUYA_BLE_ACCESSORY_UART_CMD_OTA_DATA            0xFD
#define TUYA_BLE_ACCESSORY_UART_CMD_OTA_END             0xFE
#define TUYA_BLE_ACCESSORY_UART_CMD_FATORY_CMD          0xF0

#define TUYA_BLE_ACCESSORY_EVT_CONNECTE_STATUS  0

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
 * @brief Function for send UART data to the specified port.
 *
 * @param[in] port_id     Refers to the ID number of the UART.
 * @param[in] p_data     The data buffer to send.
 * @param[in] p_data     The length of the data to be sent..
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_uart_send_data(UINT8_T port_id, CONST UINT8_T *p_data, UINT16_T len);

/**
 * @brief Function for receive UART data to the specified port.
 *
 * @param[in] port_id Refers to the ID number of the UART.
 * @param[in] p_data  pointer to full instruction data(Complete instruction,include0x55 or 0x66 0xaa and checksum.)
 * @param[in] len     Number of bytes of pdata.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_uart_receive_data(UINT8_T port_id, UINT8_T *p_data, UINT16_T len);

/**
 * @brief Function for send the full instruction received from specified port uart to the sdk.
 *
 * @param[in] port_id Refers to the ID number of the UART.
 * @param[in] p_data  pointer to full instruction data(Complete instruction,include0x55 or 0x66 0xaa and checksum.)
 * @param[in] len     Number of bytes of pdata.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_accessory_uart_full_instruction_received(UINT8_T port_id, UINT8_T *p_data, UINT16_T len);

/**
 * @brief Function for handling the accessory OTA data from the app.
 *
 * @param[in] cmd    See @FRM_ACCESSORY_OTA_START_REQ ...
 * @param[in] p_data pointer to ota data.
 * @param[in] len    Number of bytes of pdata.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_ota_data_from_ble_handler(UINT8_T port_id, UINT16_T cmd, UINT8_T*recv_data, UINT32_T recv_len);

/**
 * @brief Function for sending the working status of the accessory to the accessory.
 *
 * @param[in] port_id Refers to the ID number of the UART.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_uart_cmd_send_work_mode(UINT8_T port_id);

/**
 * @brief Function for sending the dp data to the accessory.
 *        Called in this event callback.@TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_DATA_RECEIVED
 * @param[in] port_id     Refers to the ID number of the UART.
 * @param[in] sn     Serial number..
 * @param[in] p_dp_data     pointer to dp data.
 * @param[in] data_len     Number of bytes of p_dp_data.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_uart_cmd_dp_data_send_to_accessory(UINT8_T port_id, UINT32_T sn, UINT8_T *p_dp_data, UINT16_T data_len);

/**
 * @brief Function for querying the DP status of the accessory.
 *        Called in this event callback.@TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_QUERY
 *
 * @param[in] port_id     Refers to the ID number of the UART.
 * @param[in] p_data     pointer to add data.
 * @param[in] data_len     Number of bytes of p_data.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_uart_cmd_dp_query(UINT8_T port_id, UINT8_T *p_data, UINT16_T data_len);

/**
 * @brief Function for sending custom event to sdk.
 *
 * @param[in] evt_id     See about @TUYA_BLE_ACCESSORY_EVT_CONNECTE_STATUS
 * @param[in] p_data     pointer to add data.
 *
 * @return NONE
 */
tuya_ble_status_t tuya_ble_accessory_send_custom_event(INT32_T evt_id, UINT8_T *p_data);

/**
 * @brief The function is to handle the accessories UART protocol.
 *        Call this function in tuya_ble_custom_app_uart_common_process.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_uart_protocol_process(UINT8_T port_id, UINT8_T *p_in_data, UINT16_T in_len);

/**
 * @brief Function for accessory disconnect.
 *        Call this function when the accessory pull out.
 *
 * @return NONE
 */
VOID_T tuya_ble_accessory_disconnect_handler(VOID_T);

/**
 * @brief Call this to get ota status.
 *
 * @return OTA Status: 1 in ota, 0 not ota
 */
UINT8_T tuya_ble_accessory_mcu_ota_status(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __TUYA_BLE_ACCESSORY_UART_PROTOCOL_HANDLER_DEMO_H__ */

