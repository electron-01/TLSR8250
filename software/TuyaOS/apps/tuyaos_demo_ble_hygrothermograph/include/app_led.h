/**
 * @file app_led.h
 * @brief This is app_led file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_LED_H__
#define __APP_LED_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef APP_LED_PIN
#define APP_LED_PIN 17
#endif

#ifndef APP_LED_ACTIVE_LEVEL
#define APP_LED_ACTIVE_LEVEL 1
#endif

typedef enum {
    APP_LED_POWER_ON = 0,
    APP_LED_KEY_PRESS,
    APP_LED_PAIRING_SUCCESS,
    APP_LED_PAIRING_START,
    APP_LED_PRODUCTION,
    APP_LED_MAX,
} APP_LED_ACTION_E;

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
 * @brief led hardware init
 *
 * @param[in] NONE
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_led_hw_init(VOID_T);

/**
 * @brief led software init
 *
 * @param[in] NONE
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_led_sw_init(VOID_T);

/**
 * @brief led action stop
 *
 * @param[in] action: action type see APP_LED_ACTION_E
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_led_stop(APP_LED_ACTION_E action);

/**
 * @brief led action start
 *
 * @param[in] action: action type see APP_LED_ACTION_E
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_led_start(APP_LED_ACTION_E action);


#ifdef __cplusplus
}
#endif

#endif /* __APP_LED_H__ */

