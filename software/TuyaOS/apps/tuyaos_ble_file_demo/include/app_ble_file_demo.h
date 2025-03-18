/**
 * @file app_ble_file_demo.h
 * @brief This is app_ble_file_demo file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_BLE_FILE_DEMO_H__
#define __APP_BLE_FILE_DEMO_H__

/***********************************************************************
 ** INCLUDE                                                           **
 **********************************************************************/
#include "stdint.h"

#include "tuya_cloud_types.h"
#include "tuya_ble_type.h"

#include "tal_ble_file.h"
// #include "tuya_ble_app_file_demo.h"
#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/
#define TUYA_BLE_HISTOR_MAX_NUM          25
#define TUYA_BLE_ALIGN_NUM 0

/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/
typedef  struct {
    UINT16_T storage_flag; // flash initialization is complete.
    UINT16_T data_nums;    // store the entry of the file.
    UINT16_T read_index;   // need to read the serial number of the file.
    UINT16_T write_index;  // the serial number of the file that needs to be stored.
    
    TAL_BLE_FILE_INFO_DATA_T file_info_data[TUYA_BLE_HISTOR_MAX_NUM];
} TUYA_BLE_HISTORY_FILE_T;

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTON                                                           **
 **********************************************************************/
/**
 * @brief get the handle of file history
 * 
 * @param[in] none
 *
 * @return TUYA_BLE_HISTORY_FILE_T the handle of file history struct param
 */
TUYA_BLE_HISTORY_FILE_T* tuya_ble_history_file_get_record(VOID_T);

/**
 * @brief delete the storage file
 * 
 * @param[in] file_fid: The FID of file
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tuya_ble_history_file_del_by_fid(UINT16_T file_fid);

/**
 * @brief Get History file by FID
 * 
 * @param[in] file_fid: The FID of File
 *
 * @return TAL_BLE_FILE_INFO_DATA_T the handle of file data
 */
TAL_BLE_FILE_INFO_DATA_T* tuya_ble_history_file_get_by_fid(UINT16_T file_fid);

/**
 * @brief Init file app. must excuted at the first of other file operation
 * 
 * @param[in] none
 *
 * @return none
 */
VOID_T tuya_ble_file_init(VOID_T);

/**
 * @brief process ble event of file. running at the evetn of TUYA_BLE_CB_EVT_FILE_DATA
 * 
 * @param[in] file: the ble event of file
 *
 * @return none
 */
VOID_T tuya_ble_file_handler(tuya_ble_file_data_t *file);

/**
 * @brief reset file transport status when ble disconnect
 * 
 * @param[in] none: 
 *
 * @return none
 */
VOID_T tuya_ble_file_disconn_handler(VOID_T);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BLE_FILE_DEMO_H__ */

