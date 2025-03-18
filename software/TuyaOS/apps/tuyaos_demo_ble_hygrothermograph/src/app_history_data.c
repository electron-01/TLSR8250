/**
 * @file app_history_data.c
 * @brief This is app_history_data file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "app_history_data.h"
#include "string.h"
#include "board.h"

#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"
#include "tal_rtc.h"
#include "tal_key.h"
#include "tal_sdk_test.h"
#include "tal_util.h"
#include "tal_flash.h"

#include "tuya_ble_api.h"
#include "tuya_ble_main.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"
#include "tuya_ble_product_test.h"
#include "tuya_ble_bulkdata_demo.h"

#include "app_dp_parser.h"
#include "app_sensor.h"
#include "app_misc.h"
#include "app_history_data.h"

#if ENABLE_HISTORY_DATA

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define MAX_WRITE_DATA_PAGE 1 // must <= APP_BULKDATA_PAGE_NUM

// do not change the 2 define below
#define TUYA_BLE_BULKDATA_BLOCK_SIZE 512
#define TLD_SIZE    32

// data storage address.
#define APP_BULKDATA_START_ADDR 0x2C000
#define APP_BULKDATA_END_ADDR   0x3F000
#define APP_BULKDATA_PAGE_NUM ((APP_BULKDATA_END_ADDR - APP_BULKDATA_START_ADDR) / 0x1000)

// 300 seconds (5 minute) record one history data
#define APP_HISTORY_RECORD_INTERVAL (300)

// automatic generate history data when device power on and history data empty
#define HISTORY_TEST 0

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC VOID_T tuya_ble_bulkdata_report_cb(UINT8_T* p_block_buf, UINT32_T block_length, UINT32_T block_number);
STATIC VOID_T tuya_ble_bulkdata_info_cb(TUYA_BLE_BULKDATA_INFO_T* info);
STATIC VOID_T tuya_ble_bulkdata_erase_cb(UINT8_T type, UINT8_T* status);
STATIC VOID_T app_history_test(VOID_T);

STATIC CONST UINT8_T TLD_DATA_TEMPLATE[TLD_SIZE] =
{
    0x02,                                                                    /* TYPE:2 */
    0x00,0x1D,                                                               /* LENGTH: 0x0015 = 21*/
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x14,
    0x5F,0xC0,0xAF,0xDD,                                                     /* UNIX TIME: 0x5FC0AFDD = 1606463453 */
    0x01,0x02,0x00,0x04,0x00,0x00,0x00,0x00,                                /* dp id: 1; dp type : value; dp len : 4; dp data : xxx */
    0x02,0x02,0x00,0x04,0x00,0x00,0x00,0x00,                                /* dp id: 2; dp type : value; dp len : 4; dp data : xxx */
    0x00
};

STATIC TUYA_BLE_BULKDATA_CB_T tuya_ble_bulkdata_cb = {
    .info_cb   = tuya_ble_bulkdata_info_cb,
    .report_cb = tuya_ble_bulkdata_report_cb,
    .erase_cb  = tuya_ble_bulkdata_erase_cb,
};

STATIC UINT8_T sg_app_history_timeout_flag  = 0;
STATIC UINT32_T sg_app_history_time_count   = 0;
STATIC TUYA_BLE_BULKDATA_EXTERNAL_PARAM_T tuya_ble_external_param = {
    .type = 1,
    .flag = NEED_PARSING_BY_APP,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC OPERATE_RET app_history_totle_length_get(UINT32_T *value)
{
    OPERATE_RET ret = OPRT_OK;
    UINT32_T total_length = 0;

    if (value == NULL) {
        TAL_PR_ERR("totle length get failed");
        return OPRT_INVALID_PARM;
    }

    if (app_config_info.history_record_pages >= APP_BULKDATA_PAGE_NUM) {
        total_length = 0x1000 * APP_BULKDATA_PAGE_NUM;
    } else {
        total_length = (0x1000 * app_config_info.history_record_pages) + app_config_info.history_record_index;
    }

    *value = total_length;
    return ret;
}

STATIC OPERATE_RET app_history_data_crc32_get(UINT32_T total_length, UINT32_T *value)
{
    UINT32_T crc32 = 0;
    UINT32_T offset = 0;
    UINT8_T *buffer = NULL;
    UINT32_T read_addr;
    UINT32_T move_size = 0;

    if (value == NULL) {
        TAL_PR_ERR("CRC32 Get Failed");
        return OPRT_INVALID_PARM;
    }

    if (total_length == 0) {
        *value = 0xFFFFFFFF;
        TAL_PR_INFO("No Bulk Data");
        return OPRT_INVALID_PARM;
    }

    buffer = (UINT8_T *)tuya_ble_malloc(512);
    if (buffer == NULL) {
        return OPRT_INVALID_PARM;
    }

    while (offset < total_length) {
        read_addr = APP_BULKDATA_START_ADDR + offset;
        if ((offset + 512) <= total_length) {
            move_size = 512;
        } else {
            move_size = total_length - offset;
        }
        tal_flash_read(read_addr, buffer, move_size);
        crc32 = tal_util_crc32(buffer, move_size, &crc32);
        offset += move_size;
    }

    tal_free(buffer);

    *value = crc32;

    return OPRT_OK;
}

STATIC VOID_T tuya_ble_bulkdata_report_cb(UINT8_T* p_block_buf, UINT32_T block_length, UINT32_T block_number)
{
    UINT32_T read_addr = APP_BULKDATA_START_ADDR + block_number * TUYA_BLE_BULKDATA_BLOCK_SIZE;
    tal_flash_read(read_addr, p_block_buf, block_length);
}

STATIC VOID_T tuya_ble_bulkdata_info_cb(TUYA_BLE_BULKDATA_INFO_T* info)
{
    UINT32_T length = 0;
    UINT32_T crc32  = 0;
    OPERATE_RET ret = OPRT_OK;

    info->block_length = TUYA_BLE_BULKDATA_BLOCK_SIZE;

    ret = app_history_totle_length_get(&length);
    if (ret) {
        info->total_length = 0;
        info->total_crc32  = 0xFFFFFFFF;
        app_status_info.bulk_status = 0;
        return;
    }
    ret = app_history_data_crc32_get(length, &crc32);
    if (ret) {
        info->total_length = 0;
        info->total_crc32  = 0xFFFFFFFF;
        app_status_info.bulk_status = 0;
        return;
    }

    info->total_length = length;
    info->total_crc32  = crc32;

    app_status_info.bulk_status = 1;
    TAL_PR_INFO("History Data Info Get: %d", info->total_length);
}

STATIC VOID_T tuya_ble_bulkdata_erase_cb(UINT8_T type, UINT8_T* status)
{
    TAL_PR_INFO("History Data Reset");

    app_status_info.bulk_status = 0;
    // 只要复位计数值，不需要全部清楚flash的内容，这样可以减少阻塞时间，保证蓝牙通信的稳定
    app_config_info.history_record_index  = 0;
    app_config_info.history_record_pages  = 0;
    app_config_param_save();
}

STATIC OPERATE_RET app_history_data_write(UINT32_T unix_time, INT32_T temperature, UINT32_T humidity)
{
    if (app_config_info.history_record_pages >= APP_BULKDATA_PAGE_NUM) {
        TAL_PR_ERR("History Data FULL");
        return OPRT_EXCEED_UPPER_LIMIT;
    }

    OPERATE_RET ret = OPRT_OK;
    UINT32_T write_addr = 0;
    UINT8_T tld_data[TLD_SIZE];

    memcpy(tld_data, TLD_DATA_TEMPLATE, TLD_SIZE);

    tld_data[11] = (unix_time >> 24) & 0xFF;
    tld_data[12] = (unix_time >> 16) & 0xFF;
    tld_data[13] = (unix_time >> 8)  & 0xFF;
    tld_data[14] = (unix_time >> 0)  & 0xFF;

    tld_data[15] = OR_BASIC_TEMPERATURE;
    tld_data[16] = DT_VALUE;
    tld_data[17] = (DT_VALUE_LEN >> 8) & 0xFF;
    tld_data[18] = (DT_VALUE_LEN >> 0) & 0xFF;
    tld_data[19] = (temperature >> 24) & 0xFF;
    tld_data[20] = (temperature >> 16) & 0xFF;
    tld_data[21] = (temperature >> 8)  & 0xFF;
    tld_data[22] = (temperature >> 0)  & 0xFF;

    tld_data[23] = OR_BASIC_HUMIDITY;
    tld_data[24] = DT_VALUE;
    tld_data[25] = (DT_VALUE_LEN >> 8) & 0xFF;
    tld_data[26] = (DT_VALUE_LEN >> 0) & 0xFF;
    tld_data[27] = (humidity >> 24) & 0xFF;
    tld_data[28] = (humidity >> 16) & 0xFF;
    tld_data[29] = (humidity >> 8) & 0xFF;
    tld_data[30] = (humidity >> 0) & 0xFF;

    if ((app_config_info.history_record_index + TLD_SIZE) <= 0x1000) {
        TAL_PR_INFO("History Data Write: clock %d, T %d, H %d, index %d, page %d", unix_time, temperature, humidity, app_config_info.history_record_index, app_config_info.history_record_pages);

        write_addr = APP_BULKDATA_START_ADDR + ((app_config_info.history_record_pages % APP_BULKDATA_PAGE_NUM) * 0x1000) + (app_config_info.history_record_index);
        if ((write_addr % 0x1000) == 0) {
            tal_flash_erase(write_addr, 0x1000);
        }
        TAL_PR_DEBUG("Write Addr: %x", write_addr);
        ret = tal_flash_write(write_addr, tld_data, TLD_SIZE);
        if (ret) {
            TAL_PR_ERR("History Data ERR:%d", ret);
        }

        app_config_info.history_record_index += TLD_SIZE;
        if ((app_config_info.history_record_index) >= 0x1000) {
            app_config_info.history_record_pages += 1;
            app_config_info.history_record_index = 0;
        }
    } else {
        TAL_PR_ERR("History ADDR ERR");
    }

    return ret;
}

OPERATE_RET app_history_init(VOID_T)
{
    tuya_ble_bulk_data_init(&tuya_ble_external_param, &tuya_ble_bulkdata_cb);
#if HISTORY_TEST
    app_history_test();
#endif
    return OPRT_OK;
}

VOID_T app_history_process(VOID_T)
{
    if (app_status_info.unix_time_status == 0) {
        return;
    }

    // device connected app / gateway and direct poll data through BLE
    if (tuya_ble_connect_status_get() == BONDING_CONN) {
        sg_app_history_time_count = 0;
        return;
    }

    sg_app_history_time_count += APP_SENSOR_RUNNIGN_CYCLE;
    if (sg_app_history_time_count  >= APP_HISTORY_RECORD_INTERVAL) {
        TAL_PR_DEBUG("HISTORY TIMEOUT: %d", sg_app_history_time_count);
        sg_app_history_time_count -= APP_HISTORY_RECORD_INTERVAL;
        sg_app_history_timeout_flag = 1;
    }

    if (sg_app_history_timeout_flag) {
        // Do not change history data when history data on bulk
        if (app_status_info.bulk_status) {
            return;
        }

        // Do Not operate Flash too much when device on ota
        if (tuya_ble_ota_get_status() != -1) {
            return;
        }

        sg_app_history_timeout_flag = 0;

        OPERATE_RET ret = OPRT_OK;
        TIME_T time;
        APP_SENSOR_DATA_T data;
        tal_rtc_time_get(&time);
        data = app_sensor_data_get();
        ret = app_history_data_write(time, data.temperature, data.humidity);
        if (ret) {
            TAL_PR_ERR("write error");
            return;
        }

        ret = app_config_param_save();
        if (ret) {
            TAL_PR_ERR("config save error");
            return;
        }
    }
}

#if HISTORY_TEST

STATIC VOID_T app_history_test(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    UINT32_T temp;
    UINT32_T humi;
    // tick must within 7 days
    UINT32_T tick = 1695312000; //2023-09-22 0:0:0
    if ((app_config_info.history_record_pages == 0) && (app_config_info.history_record_index == 0)) {
        for (UINT32_T i = 0; i < MAX_WRITE_DATA_PAGE * 128; i++) {
            temp = 200 + tal_system_get_tick_count() % 100;
            humi = 50  + tal_system_get_tick_count() % 30;
            ret = app_history_data_write(tick, temp, humi);
            if (ret) {
                TAL_PR_ERR("ERROR->history write:%d", ret);
            }
            tick += 60 * 5;
        }
        app_config_param_save();
    }

    TAL_PR_DEBUG("app_history_test:%d, %d", app_config_info.history_record_pages, app_config_info.history_record_index);
}

#endif

#endif

