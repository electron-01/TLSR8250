/**
 * @file app_roaming.h
 * @brief This is app_roaming file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_ROAMING_H__
#define __APP_ROAMING_H__

#include "board.h"
#include "tuya_ble_internal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define ADV_FRAME_COUNTER_ADDR BOARD_FLASH_SDK_TEST_START_ADDR
#define ADV_INTERVAL_ADDR      (BOARD_FLASH_SDK_TEST_START_ADDR + TUYA_NV_ERASE_MIN_SIZE)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT32_T adv_interval;
    UINT32_T adv_duration;
} roaming_param_t;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern roaming_param_t g_roaming_param;
extern uint32_t adv_frame_counter;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

/**
 * @brief
 *
 * @param[in] param1:
 * @param[in] param2:
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_init(void);

/**
 * @brief app_roaming_sn_write
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_sn_write(void);

/**
 * @brief app_roaming_sn_read
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_sn_read(void);

/**
 * @brief app_roaming_sn_clear
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_sn_clear(void);

/**
 * @brief app_roaming_adv_interval_write
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_adv_interval_write(void);

/**
 * @brief app_roaming_adv_interval_read
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
void app_roaming_adv_interval_read(void);


#ifdef __cplusplus
}
#endif

#endif /* __APP_ROAMING_H__ */

