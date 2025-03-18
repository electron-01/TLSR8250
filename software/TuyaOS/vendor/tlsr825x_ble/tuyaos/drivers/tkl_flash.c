/**
 * @file tkl_flash.c
 * @brief This is tkl_flash file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "stack/ble/ble.h"
#include "pm.h"
#include "board.h"
#include "vendor/8258_module/telink_app_config.h"

#include "battery_check.h"
#include "tkl_flash.h"

/*
 * 关于 Flash 使用的情况 ，详情见Tuya Developer 平台说明:
 * https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_platform_ble_tlsr825x?id=Kcnreatkaj9er
 * 
 * Please turn to the Tuya Developer Platform to get detail info: 
 * https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_platform_ble_tlsr825x?id=Kcnreatkaj9er
*/

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef CODE1_START_ADDR
#define CODE1_START_ADDR  0x00000
#endif

#ifndef CODE1_STOP_ADDR
#define CODE1_STOP_ADDR   0x2B000
#endif

#ifndef CODE2_START_ADDR
#define CODE2_START_ADDR  0x40000
#endif

#ifndef CODE2_STOP_ADDR
#define CODE2_STOP_ADDR   0x6B000
#endif

#ifndef CODE_MAX_ADDR
#define CODE_MAX_ADDR     0x80000
#endif

#define USER1_DATA_START_ADDR 0x2C000
#define USER1_DATA_STOP_ADDR  0x3F000

#define USER2_DATA_START_ADDR 0x6C000
#define USER2_DATA_STOP_ADDR  0x74000

#define USER3_DATA_START_ADDR 0x78000
#define USER3_DATA_STOP_ADDR  0x80000

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC INT32_T tkl_flash_addr_operability_check(UINT32_T addr)
{
    if ((addr >= USER1_DATA_START_ADDR) && (addr <= USER1_DATA_STOP_ADDR)) {
        return 0;
    }

    if ((addr >= USER2_DATA_START_ADDR) && (addr <= USER2_DATA_STOP_ADDR)) {
        return 0;
    }

    if ((addr >= USER3_DATA_START_ADDR) && (addr <= USER3_DATA_STOP_ADDR)) {
        return 0;
    }

    return -1;
}

STATIC INT32_T tkl_flash_addr_operability_check_ota(UINT32_T addr)
{
    if ((addr >= CODE1_START_ADDR) && (addr <= CODE1_STOP_ADDR)) {
        return 0;
    }

    if ((addr >= CODE2_START_ADDR) && (addr <= CODE2_STOP_ADDR)) {
        return 0;
    }

    if ((addr >= BOARD_FLASH_OTA_INFO_ADDR) && (addr <= (BOARD_FLASH_OTA_INFO_ADDR + 0x1000))) {
        return 0;
    }

    return -1;
}

UINT32_T tuya_port_flash_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    return ret;
}

OPERATE_RET tkl_flash_read(UINT32_T addr, UCHAR_T *dst, UINT32_T size)
{
    OPERATE_RET ret = OPRT_OK;
    flash_read_page(addr, size, dst);

    return ret;
}

OPERATE_RET tkl_flash_write(UINT32_T addr, CONST UCHAR_T *src, UINT32_T size)
{
    OPERATE_RET ret = OPRT_OK;
    UINT32_T offset = addr&0xff;
    UINT32_T write_len = 0;
    UINT32_T write_offset = 0;
    UINT8_T *addr_data = (UINT8_T*)src;

    if (0 != tkl_flash_addr_operability_check(addr)) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check(addr + size)) {
        return OPRT_INVALID_PARM;
    }

#if (BATT_CHECK_ENABLE)
    if (battery_get_detect_enable()) {
        if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
            app_battery_power_check(VBAT_ALRAM_THRES_MV + 200);  //2.2 V
        }
        else {
            app_battery_power_check(VBAT_ALRAM_THRES_MV);  //2.0 V
        }
    }
#endif

    for (write_offset=0; write_offset<size; write_offset+=write_len) {
        if ((size - write_offset) > (256 - offset)) {
            write_len = 256 - offset;
        } else {
            write_len = size - write_offset;
        }

        flash_write_page(addr+write_offset, write_len, addr_data+write_offset);
        offset = (addr+write_len)& 0xff;
    }

    return ret;
}

OPERATE_RET tkl_flash_erase(UINT32_T addr, UINT32_T size)
{
    if (size == 0) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check(addr)) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check(addr + size)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET ret = OPRT_OK;
#if (BATT_CHECK_ENABLE)
    if (battery_get_detect_enable()) {
        if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
            app_battery_power_check(VBAT_ALRAM_THRES_MV + 200);  //2.2 V
        }
        else {
            app_battery_power_check(VBAT_ALRAM_THRES_MV);  //2.0 V
        }
    }
#endif

    UINT8_T i = 0;
    UINT8_T block_num = ((size + 0x1000 - 1) / 0x1000);
    do {
        flash_erase_sector(addr + (4096 * i));
        i++;
    }while (i < block_num);

    return ret;
}

OPERATE_RET tkl_flash_lock(UINT32_T addr, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_flash_unlock(UINT32_T addr, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_E type, TUYA_FLASH_BASE_INFO_T* info)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_flash_write_ota(UINT32_T addr, CONST UCHAR_T *src, UINT32_T size)
{
    OPERATE_RET ret = OPRT_OK;
    UINT32_T offset = addr&0xff;
    UINT32_T write_len = 0;
    UINT32_T write_offset = 0;
    UINT8_T *addr_data = (UINT8_T*)src;

    if (0 != tkl_flash_addr_operability_check_ota(addr)) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check_ota(addr + size)) {
        return OPRT_INVALID_PARM;
    }

#if (BATT_CHECK_ENABLE)
    if (battery_get_detect_enable()) {
        if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
            app_battery_power_check(VBAT_ALRAM_THRES_MV + 200);  //2.2 V
        }
        else {
            app_battery_power_check(VBAT_ALRAM_THRES_MV);  //2.0 V
        }
    }
#endif

    for (write_offset=0; write_offset<size; write_offset+=write_len) {
        if ((size - write_offset) > (256 - offset)) {
            write_len = 256 - offset;
        } else {
            write_len = size - write_offset;
        }
        flash_write_page(addr+write_offset, write_len, addr_data+write_offset);
        offset = (addr+write_len)& 0xff;
    }

    return ret;
}

OPERATE_RET tkl_flash_erase_ota(UINT32_T addr, UINT32_T size)
{
    if (size == 0) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check_ota(addr)) {
        return OPRT_INVALID_PARM;
    }

    if (0 != tkl_flash_addr_operability_check_ota(addr + size)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET ret = OPRT_OK;
#if (BATT_CHECK_ENABLE)
    if (battery_get_detect_enable()) {
        if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
            app_battery_power_check(VBAT_ALRAM_THRES_MV + 200);  //2.2 V
        }
        else {
            app_battery_power_check(VBAT_ALRAM_THRES_MV);  //2.0 V
        }
    }
#endif

    UINT32_T erase_pages = size/0x1000;
    if (size % 0x1000 != 0) {
        erase_pages++;
    }
    for (UINT8_T i = 0; i < erase_pages; i++) {
        flash_erase_sector(addr + (4096 * i));
    }
    return ret;
}

