/**
 * @file app_dp_reporter.h
 * @brief This is app_dp_reporter file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_DP_REPORTER_H__
#define __APP_DP_REPORTER_H__

#include "tuya_cloud_types.h"
#include "app_dp_parser.h"
#include "app_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
typedef enum {
    APP_REPORT_T_H_VALUE = 0,
    APP_REPORT_BATTERY,
    APP_REPORT_T_SENSITIVE,
    APP_REPORT_H_SENSITIVE,
    APP_REPORT_ALL,
} APP_DP_REPORTER_E;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    INT16_T  temperature;
    UINT16_T humidity;
} APP_POLL_DATA_T;

typedef struct {
    APP_POLL_DATA_T poll_data;
    UINT8_T tx_status;
    UINT8_T tx_step;
    UINT32_T report_bit_map;
} APP_BLE_BEACON_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
/**
 * @brief set report temperature value
 *
 * @param[in] temperature: temperature value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_reporter_temperature_set(INT16_T temperature);

/**
 * @brief set report humidity value
 *
 * @param[in] humidity:  humidity value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_reporter_humidity_set(UINT16_T humidity);

/**
 * @brief recent reported temperature and humidity value get
 *
 * @param[in] data: get temperature and humidity value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_reporter_sensor_data_get(APP_SENSOR_DATA_T *data);

/**
 * @brief report event set
 *
 * @param[in] NONE:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_reporter_event_set(APP_DP_REPORTER_E report);

/**
 * @brief app_dp_reporter_init
 *
 * @param[in] NONE:
 *
 * @return NONE
 */
VOID_T app_ble_beacon_process(VOID_T);

/**
 * @brief app_dp_reporter_init
 *
 * @param[in] NONE:
 *
 * @return NONE
 */
VOID_T app_dp_reporter_init(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_DP_REPORTER_H__ */

