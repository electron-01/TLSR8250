/**
 * @file app_dp_parser.h
 * @brief This is app_dp_parser file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __APP_DP_PARSER_H__
#define __APP_DP_PARSER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
//WR-write_report, OW-only_write, OR-only_report
#define OR_DP_STATUS_BATTERY_PERCENT                          ( 8  )    //!< Remaining battery percent.
#define OR_DP_RECORD_BLE_OPEN_LOCK                            ( 19 )    //!< ble unlock record.
#define OR_DP_RECORD_CLOSE_LOCK_COMMON                        ( 20 )    //!< close door lock record
#define WR_DP_SETTING_AUTOLOCK_STU                            ( 33 )    //!< autolock state
#define WR_DP_SETTING_AUTOLOCK_PERIOD                         ( 36 )    //!< autolock period, unit: second
#define WR_DP_MANUAL_CLOSE_DOORLOCK                           ( 46 )    //!< manual close doorlock
#define OR_DP_STATUS_LOCK_STU                                 ( 47 )    //!< lock state, such as clutch
#define WR_SBP_DP_MASTER_OPEN_LOCK                            ( 71 )    //!< new ble open lock [dev with ble parts function]
#define WR_SBP_DP_SET_MASTER_SRAND_NUM                        ( 70 )    //!< set master srand num [dev with ble parts function]

typedef enum {
    APP_CLOSE_LOCK  = 4,
    AUTO_CLOSE_LOCK = 6,
} CLOSE_LOCK_ENUM;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
#pragma pack(1)
typedef struct {
    UINT8_T  dp_id;
    UINT8_T  dp_type;
    UINT16_T dp_data_len;
    UINT8_T  dp_data[600];
} demo_dp_t;
#pragma pack()

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern demo_dp_t g_cmd;
extern demo_dp_t g_rsp;
extern UINT32_T  g_sn;

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
OPERATE_RET app_dp_parser(UINT8_T* buf, UINT32_T size);

/**
 * @brief app_dp_report
 *
 * @param[in] dp_id: dp_id
 * @param[in] buf: buf
 * @param[in] size: size
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size);

/**
 * @brief app_dp_record_report
 *
 * @param[in] dp_id: dp_id
 * @param[in] buf: buf
 * @param[in] size: size
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET app_dp_record_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size);


#ifdef __cplusplus
}
#endif

#endif /* __APP_DP_PARSER_H__ */

