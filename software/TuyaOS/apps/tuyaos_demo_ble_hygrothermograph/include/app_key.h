/**
 * @file app_key.h
 * @brief This is app_key file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef APP_KEY_PIN
#define APP_KEY_PIN 28
#endif

#ifndef APP_KEY_ACTIVE_LEVEL
#define APP_KEY_ACTIVE_LEVEL TUYA_GPIO_LEVEL_LOW
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
 * @brief key software init, like soft time timer
 *
 * @param[in] NONE
 *
 * @return NONE
 */
VOID_T app_key_sw_init(VOID_T);

/**
 * @brief key hardware init, like peripheral gpio
 *
 * @param[in] NONE
 *
 * @return NONE
 */
VOID_T app_key_hw_init(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_KEY_H__ */

