/**
 * @file app_misc.h
 * @brief This is app_misc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_MISC_H__
#define __APP_MISC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef APP_SENSOR_RUNNIGN_CYCLE
#define APP_SENSOR_RUNNIGN_CYCLE 2
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT8_T enter_pairing;
    UINT8_T bound_flag;
    UINT8_T unix_time_status;
    UINT8_T bulk_status;
} APP_STATUS_INFO_T;
typedef struct {
    INT32_T temperature_sensitive;
    INT32_T humidity_sensitive;
    UINT32_T history_record_index;
    UINT32_T history_record_pages;
    UINT32_T crc32;
} APP_CONFIG_INFO_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern APP_STATUS_INFO_T app_status_info;
extern APP_CONFIG_INFO_T app_config_info;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

/**
 * @brief app_pairing_start
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_pairing_start(VOID_T);

/**
 * @brief app_pairing_stop
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_pairing_stop(VOID_T);

/**
 * @brief app_misc_init
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_misc_init(VOID_T);

/**
 * @brief app_temperature_sensitive_set
 *
 * @param[in] value: value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_temperature_sensitive_set(INT32_T value);

/**
 * @brief app_humidity_sensitive_set
 *
 * @param[in] value: value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_humidity_sensitive_set(INT32_T value);

/**
 * @brief app_temperature_sensitive_get
 *
 * @param[in] *value: *value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_temperature_sensitive_get(INT32_T *value);

/**
 * @brief app_humidity_sensitive_get
 *
 * @param[in] *value: *value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_humidity_sensitive_get(INT32_T *value);

/**
 * @brief app_config_param_reset
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_config_param_reset(VOID_T);

/**
 * @brief app_config_param_save
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_config_param_save(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_MISC_H__ */

