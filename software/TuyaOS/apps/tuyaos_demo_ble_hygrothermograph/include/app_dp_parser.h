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
#define OR_BASIC_TEMPERATURE        (1)
#define OR_BASIC_HUMIDITY           (2)
#define OR_BASIC_BATTERY            (4)
#define WR_BASIC_T_SENSITIVE        (19)
#define WR_BASIC_H_SENSITIVE        (20)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
#pragma pack(1)
typedef struct {
    UINT8_T  dp_id;
    UINT8_T  dp_type;
    UINT16_T dp_data_len;
    UINT8_T  *dp_data;
} app_dp_parser_t;
#pragma pack()

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


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


#ifdef __cplusplus
}
#endif

#endif /* __APP_DP_PARSER_H__ */

