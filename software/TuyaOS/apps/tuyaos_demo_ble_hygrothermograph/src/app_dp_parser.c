/**
 * @file app_dp_parser.c
 * @brief This is app_dp_parser file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "string.h"

#include "tal_log.h"
#include "tal_util.h"
#include "tal_memory.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"

#include "app_dp_parser.h"
#include "app_dp_reporter.h"
#include "app_misc.h"

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
OPERATE_RET app_dp_parser(UINT8_T* buf, UINT32_T size)
{
    UINT32_T index = 0;

    if (size > 512) {
        TAL_PR_ERR("size too large");
        return OPRT_EXCEED_UPPER_LIMIT;
    }

    TAL_PR_HEXDUMP_INFO("RECEIVE DATA", buf, size);

    app_dp_parser_t dp_parser = {0};
    while (index < size) {
        dp_parser.dp_data = NULL;

        dp_parser.dp_id       = buf[index++];
        dp_parser.dp_type     = buf[index++];
        dp_parser.dp_data_len = (buf[index++] << 8) & 0xFF00;
        dp_parser.dp_data_len |= buf[index++];
        dp_parser.dp_data     = (UINT8_T *)tal_malloc(dp_parser.dp_data_len);
        if (dp_parser.dp_data == NULL) {
            TAL_PR_ERR("Malloc Failed");
            return OPRT_MALLOC_FAILED;
        }
        memcpy(dp_parser.dp_data, &buf[index], dp_parser.dp_data_len);
        index += dp_parser.dp_data_len;

        switch (dp_parser.dp_id) {
            case WR_BASIC_T_SENSITIVE: {
                INT32_T value = dp_parser.dp_data[0] << 24 | dp_parser.dp_data[1] << 16 | dp_parser.dp_data[2] << 8 | dp_parser.dp_data[3];
                app_temperature_sensitive_set(value);
                app_dp_reporter_event_set(APP_REPORT_T_SENSITIVE);
            } break;
            case WR_BASIC_H_SENSITIVE: {
                INT32_T value = dp_parser.dp_data[0] << 24 | dp_parser.dp_data[1] << 16 | dp_parser.dp_data[2] << 8 | dp_parser.dp_data[3];
                app_humidity_sensitive_set(value);
                app_dp_reporter_event_set(APP_REPORT_H_SENSITIVE);
            } break;

            default: {
            } break;
        }
    }

    return OPRT_OK;
}

