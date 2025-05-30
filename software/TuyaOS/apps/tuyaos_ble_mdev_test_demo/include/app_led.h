/**
 * @file app_led.h
 * @brief This is app_led file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_LED_H__
#define __APP_LED_H__

/***********************************************************************
 ** INCLUDE                                                           **
 **********************************************************************/
#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/


/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/


/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/


/***********************************************************************
 ** FUNCTON                                                           **
 **********************************************************************/




/**
 * @brief 
 * 
 * @param[in] param1: 
 * @param[in] param2: 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_led_timer_init(VOID_T);
VOID_T app_led_timer_start(VOID_T);
VOID_T app_led_timer_stop(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_LED_H__ */

