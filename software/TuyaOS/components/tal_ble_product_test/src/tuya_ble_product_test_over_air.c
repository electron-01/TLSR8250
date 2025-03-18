/**
 * @file tuya_ble_product_test_over_air.c
 * @brief This is tuya_ble_product_test_over_air file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"

#include "tal_util.h"

#include "tuya_ble_api.h"
#include "tuya_ble_product_test_over_air.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT8_T tx_buffer[128] = {0x66, 0xAA, 0x00, 0xF0};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




TUYA_WEAK_ATTRIBUTE VOID_T tuya_ble_custom_app_production_test_process(UINT8_T channel, UINT8_T* p_in_data, UINT16_T in_len)
{
    ty_product_test_cmd_t* cmd = (VOID_T*)p_in_data;
    tal_util_reverse_byte((VOID_T*)&cmd->len, SIZEOF(UINT16_T));
    tal_util_reverse_byte((VOID_T*)&cmd->sub_id, SIZEOF(UINT16_T));

    if ((cmd->type != 3) || (cmd->len < 3)) {
        return;
    }

//    UINT16_T data_len = cmd->len - 3;

    switch (cmd->sub_id) {
        case PRODUCT_TEST_CMD_ENTER: {
            UINT8_T tmp_buf[] = "{\"ret\":true}";
            tuya_ble_product_test_rsp(channel, cmd->sub_id, tmp_buf, strlen((VOID_T*)tmp_buf));
        } break;

        case PRODUCT_TEST_CMD_EXIT: {
            UINT8_T tmp_buf[] = "{\"ret\":true}";
            tuya_ble_product_test_rsp(channel, cmd->sub_id, tmp_buf, strlen((VOID_T*)tmp_buf));
        } break;

        default: {
        } break;
    }
}

UINT32_T tuya_ble_product_test_rsp(UINT8_T channel, UINT16_T cmdId, UINT8_T* buf, UINT16_T size)
{
    UINT32_T len = 4;

    tx_buffer[len] = (size+3)>>8;
    len++;
    tx_buffer[len] = (size+3)&0xFF;
    len++;

    tx_buffer[len] = 0x03;
    len++;

    tx_buffer[len] = cmdId>>8;
    len++;
    tx_buffer[len] = cmdId&0xFF;
    len++;

    if (size > 0) {
        memcpy(&tx_buffer[len], buf, size);
        len += size;
    }

    tx_buffer[len] = tal_util_check_sum8(tx_buffer, len);
    len += 1;

    tuya_ble_production_test_asynchronous_response(channel, tx_buffer, len);

    return 0;
}

