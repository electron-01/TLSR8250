/**
 * @file app_lock.h
 * @brief This is app_lock file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_LOCK_H__
#define __APP_LOCK_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


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
 * @brief app_lock_timer_init
 *
 * @param[in] param1:
 * @param[in] param2:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_lock_timer_init(VOID_T);

/**
 * @brief app_lock_autolock_timer_start
 *
 * @param[in] param: time_ms
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_lock_autolock_timer_start(UINT32_T time_ms);

/**
 * @brief app_lock_autolock_timer_stop
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T app_lock_autolock_timer_stop(VOID_T);


#ifdef __cplusplus
}
#endif

#endif /* __APP_LOCK_H__ */

