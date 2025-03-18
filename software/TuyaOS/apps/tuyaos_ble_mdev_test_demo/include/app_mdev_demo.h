/**
 * @file app_mdev_demo.h
 * @brief This is app_mdev_demo file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_MDEV_DEMO_H__
#define __APP_MDEV_DEMO_H__

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
#define MAX_RSSI_NUM 20

/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/
typedef struct{
    INT8_T mdev_rssi[MAX_RSSI_NUM];
    UINT8_T mdev_index;
    UINT8_T is_start;
    UINT8_T mdev_num;
    UINT8_T test_en;
}TY_RSSI_BASE_TEST_T;

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/

/***********************************************************************
 ** FUNCTON                                                           **
 **********************************************************************/

/**
 * @brief 
 * 
 * @param[in] adv: 
 * @param[in] adv_len: 
 * @param[in] mac: 
 * @param[in] rssi: 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_mdev_rssi_recv(UINT8_T *adv, UINT8_T adv_len, UINT8_T *mac, int rssi);

/**
 * @brief 
 * 
 * @param[in] none: 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_mdev_check_init(VOID_T);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MDEV_DEMO_H__ */

