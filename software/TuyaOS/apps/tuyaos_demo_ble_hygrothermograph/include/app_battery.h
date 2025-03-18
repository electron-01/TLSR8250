/**
 * @file app_battery.h
 * @brief This is app_battery file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_BATTERY_H__
#define __APP_BATTERY_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef APP_BATTERY_VBAT_MV_MAX
#define APP_BATTERY_VBAT_MV_MAX 3000
#endif

#ifndef APP_BATTERY_VBAT_MV_MIN
#define APP_BATTERY_VBAT_MV_MIN 2500
#endif

#ifndef APP_BATTERY_LOWPOWER_PERCENT
#define APP_BATTERY_LOWPOWER_PERCENT 20
#endif

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
 * @brief get battery percent
 *
 * @param[in] value: the percent of battery
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_battery_vbat_percent_get(UINT8_T *value);

/**
 * @brief get battery voltage value
 *
 * @param[in] value: battery voltage value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_battery_vbat_value_get(UINT16_T *value);

/**
 * @brief battery cycle running
 *
 * @param[in] none:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_battery_process(VOID_T);

/**
 * @brief hardware init
 *
 * @param[in] none:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_battery_hw_init(VOID_T);

/**
 * @brief software init
 *
 * @param[in] none:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_battery_sw_init(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_BATTERY_H__ */

