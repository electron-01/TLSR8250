/**
 * @file app_sensor.h
 * @brief This is app_sensor file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_SENSOR_H__
#define __APP_SENSOR_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef APP_SENSOR_T_DATA_MAX
#define APP_SENSOR_T_DATA_MAX 500
#endif

#ifndef APP_SENSOR_T_DATA_MIN
#define APP_SENSOR_T_DATA_MIN -150
#endif

#ifndef APP_SENSOR_H_DATA_MAX
#define APP_SENSOR_H_DATA_MAX 100
#endif

#ifndef APP_SENSOR_H_DATA_MIN
#define APP_SENSOR_H_DATA_MIN 0
#endif

#ifndef APP_SENSOR_REPORT_PERIOD
#define APP_SENSOR_REPORT_PERIOD 60
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    INT16_T  temperature;
    UINT16_T humidity;
} APP_SENSOR_DATA_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
/**
 * @brief sensor init
 *
 * @param[in] none:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_sensor_sw_init(VOID_T);

/**
 * @brief cycle running process
 *
 * @param[in] none:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_sensor_process(VOID_T);

/**
 * @brief get current temperature and humidity data
 *
 * @param[in] none:
 *
 * @return temperature and humidity data struct.
 */
APP_SENSOR_DATA_T app_sensor_data_get(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_SENSOR_H__ */

