/**
 * @file tkl_ota.c
 * @brief This is tkl_ota file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "stack/ble/ble.h"
#include "stack/ble/service/ble_ll_ota.h"
#include "board.h"

#include "tkl_flash.h"
#include "tkl_memory.h"
#include "tkl_ota.h"

/***********************************************************************
 ********************* extern ******************************************
 **********************************************************************/
extern OPERATE_RET tkl_flash_write_ota(UINT32_T addr, CONST UCHAR_T *src, UINT32_T size);
extern OPERATE_RET tkl_flash_erase_ota(UINT32_T addr, UINT32_T size);
STATIC UINT32_T calculate_crc32(UINT8_T* buf, UINT32_T size, UINT32_T* p_crc);
STATIC UINT32_T tkl_ota_get_crc32_in_flash(UINT32_T len);
STATIC VOID_T  tuya_ble_ota_set_boot_flag(VOID_T);
STATIC UINT32_T tuya_ble_ota_info_save(VOID_T);
STATIC UINT32_T tuya_ble_ota_info_load(VOID_T);

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TKL_OTA_PKG_LEN             (512)
#define TKL_OTA_ERASE_MIN_SIZE      (0x1000)
#define TKL_OTA_FILE_MD5_LEN        (16)

#define TELINK_OTA_BOOT_FLAG        (0x4B)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT32_T flag;
    UINT32_T len;
    UINT32_T crc32;
    UINT8_T  md5[TKL_OTA_FILE_MD5_LEN];
} TKL_OTA_FIRMWARE_INFO_T;

typedef struct {
    UINT32_T *p_firmware_crc32;
    UINT8_T  *p_firmware_md5;
} tkl_ota_pri_param_t;

typedef enum {
    EM_OTA_SETTING_VALID = 0x18181888,
    EM_OTA_SETTING_INVALID = 0,
    EM_OTA_SETTING_NULL =0xFFFFFFFF
}EM_SETTING_VALID_E;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT32_T sg_firmware_len = 0;
STATIC UINT32_T sg_data_crc32 = 0;
STATIC TKL_OTA_FIRMWARE_INFO_T sg_incomplete_firmware_info_buffer = {0};
STATIC TKL_OTA_FIRMWARE_INFO_T* sg_incomplete_firmware_info = NULL;
STATIC tkl_ota_pri_param_t *tkl_ota_pri_param = NULL;
STATIC UINT32_T ota_info_addr = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
OPERATE_RET tkl_ota_get_ability(UINT32_T *image_size, TUYA_OTA_TYPE_E *type)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ota_start_notify(UINT_T image_size, TUYA_OTA_TYPE_E type, TUYA_OTA_PATH_E path)
{
#if TKL_BLUETOOTH_SUPPORT_SCAN
    tkl_ble_gap_scan_stop();
#endif
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ota_data_process(TUYA_OTA_DATA_T *pack, UINT32_T* remain_len)
{
    OPERATE_RET ret = OPRT_OK;

    tkl_ota_pri_param = pack->pri_data;

    BOOL_T flag_4k = FALSE;
    if ((pack->offset == 0) || ((pack->offset + pack->len) >= (((pack->offset/TKL_OTA_ERASE_MIN_SIZE) + 1)*TKL_OTA_ERASE_MIN_SIZE))) {
        // if (pack->offset == 0) {
        //     tkl_flash_erase_ota(BOARD_FLASH_OTA_START_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        // } else {
        //     UINT32_T erase_addr = BOARD_FLASH_OTA_START_ADDR + (((pack->offset/TKL_OTA_ERASE_MIN_SIZE) + 1)*TKL_OTA_ERASE_MIN_SIZE);
        //     tkl_flash_erase_ota(erase_addr, TKL_OTA_ERASE_MIN_SIZE);
        // }

        flag_4k = TRUE;
    }

    sg_data_crc32 = calculate_crc32(pack->data, pack->len, &sg_data_crc32);

    if (pack->offset == 0) {
        pack->data[8] = 0xFF;
    }

    tkl_flash_write_ota(BOARD_FLASH_OTA_START_ADDR + pack->offset, pack->data, pack->len);

    sg_firmware_len = pack->total_len;

    if (flag_4k) {
        memcpy(sg_incomplete_firmware_info->md5, tkl_ota_pri_param->p_firmware_md5, TKL_OTA_FILE_MD5_LEN);
        sg_incomplete_firmware_info->len = pack->offset + pack->len;
        sg_incomplete_firmware_info->crc32 = sg_data_crc32;
        tuya_ble_ota_info_save();
    }

    return ret;
}

OPERATE_RET tkl_ota_end_notify(BOOL_T reset)
{
    OPERATE_RET ret = OPRT_OK;

    if ((sg_data_crc32 != tkl_ota_get_crc32_in_flash(sg_firmware_len))
        || (sg_data_crc32 != *(tkl_ota_pri_param->p_firmware_crc32))) {
        return 2;
    }

    if (reset == TRUE) {
        tuya_ble_ota_set_boot_flag();
        memset(&sg_incomplete_firmware_info_buffer, 0, SIZEOF(TKL_OTA_FIRMWARE_INFO_T));
        tuya_ble_ota_info_save();

        analog_write(DEEP_ANA_REG0, analog_read(DEEP_ANA_REG0) | FLD_SOFT_REBOOT_FLAG);

        VOID_T tuya_ble_disconnect_and_reset_timer_start(VOID_T);
        tuya_ble_disconnect_and_reset_timer_start();

    } else {
        memset(&sg_incomplete_firmware_info_buffer, 0, SIZEOF(TKL_OTA_FIRMWARE_INFO_T));
        tuya_ble_ota_info_save();
    }

    return ret;
}

OPERATE_RET tkl_ota_get_old_firmware_info(TUYA_OTA_FIRMWARE_INFO_T **info)
{
    OPERATE_RET ret = OPRT_OK;

    TY_PRINTF("Telink OTA Addr: 0x%08x", BOARD_FLASH_OTA_START_ADDR);
    ret = tuya_ble_ota_info_load();
    *info = (TUYA_OTA_FIRMWARE_INFO_T *)&(sg_incomplete_firmware_info->len);

    return ret;
}

STATIC UINT32_T calculate_crc32(UINT8_T* buf, UINT32_T size, UINT32_T* p_crc)
{
    UINT32_T crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
    for (UINT32_T i = 0; i < size; i++) {
        crc = crc ^ buf[i];
        for (UINT32_T j = 8; j > 0; j--) {
            crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
        }
    }
    return ~crc;
}

STATIC UINT32_T tkl_ota_get_crc32_in_flash(UINT32_T len)
{
    if (len == 0) {
        return 0;
    }

    UINT8_T *buf = NULL;
    UINT32_T firmware_crc32 = 0;
    UINT32_T start_addr = BOARD_FLASH_OTA_START_ADDR;
    UINT32_T package_count = len/TKL_OTA_PKG_LEN;
    UINT32_T package_remainder = len%TKL_OTA_PKG_LEN;

    buf = tkl_system_malloc(TKL_OTA_PKG_LEN);
    if (buf) {
        for (UINT32_T idx=0; idx<package_count; idx++) {
            tkl_flash_read(start_addr, buf, TKL_OTA_PKG_LEN);
            if (idx == 0) {
                buf[8] = TELINK_OTA_BOOT_FLAG;
            }
            firmware_crc32 = calculate_crc32(buf, TKL_OTA_PKG_LEN, &firmware_crc32);
            start_addr += TKL_OTA_PKG_LEN;
        }

        if (package_remainder > 0) {
            tkl_flash_read(start_addr, buf, TKL_OTA_PKG_LEN);
            firmware_crc32 = calculate_crc32(buf, package_remainder, &firmware_crc32);
            start_addr += package_remainder;
        }
        tkl_system_free(buf);
    }

    return firmware_crc32;
}

STATIC VOID_T tuya_ble_ota_set_boot_flag(VOID_T)
{
    UINT8_T flag = TELINK_OTA_BOOT_FLAG;
    flash_write_page(BOARD_FLASH_OTA_START_ADDR + 8u, 1, &flag);
    flag = 0;
    flash_read_page(BOARD_FLASH_OTA_START_ADDR + 8u, 1, &flag);

    if (flag == TELINK_OTA_BOOT_FLAG) {
        flag = 0;
        flash_write_page((BOARD_FLASH_OTA_START_ADDR ? 0 : 0x40000) + 8u, 1, &flag);
    }
}

STATIC UINT32_T tuya_ble_ota_info_save(VOID_T)
{
    UINT32_T ota_info_len = sizeof(TKL_OTA_FIRMWARE_INFO_T);
    UINT32_T ota_info_clear_flag = 0;

    if ((ota_info_addr < (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR) || (ota_info_addr > ((UINT32_T)BOARD_FLASH_OTA_INFO_ADDR + TKL_OTA_ERASE_MIN_SIZE))) {
        TY_PRINTF("TUYA_BLE_OTA_SAVE- addr error");
        tkl_flash_erase_ota(BOARD_FLASH_OTA_INFO_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR;
    }

    if (ota_info_addr >= ((UINT32_T)BOARD_FLASH_OTA_INFO_ADDR + TKL_OTA_ERASE_MIN_SIZE - ota_info_len)) {
        TY_PRINTF("TUYA_BLE_OTA_SAVE- addr error2");
        tkl_flash_erase_ota(BOARD_FLASH_OTA_INFO_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR;
    }

    ota_info_clear_flag = EM_OTA_SETTING_INVALID;
    tkl_flash_write_ota(ota_info_addr, (UINT8_T *)&ota_info_clear_flag, sizeof(ota_info_clear_flag));
    if (0 != sg_incomplete_firmware_info->len) {
        ota_info_addr += ota_info_len;
        sg_incomplete_firmware_info->flag = EM_OTA_SETTING_VALID;
        tkl_flash_write_ota(ota_info_addr, (UINT8_T *)sg_incomplete_firmware_info, ota_info_len);
    }
    return 0;
}

STATIC UINT32_T tuya_ble_ota_info_load(VOID_T)
{
    UINT32_T ota_info_len = sizeof(TKL_OTA_FIRMWARE_INFO_T);
    UINT32_T do_effect_flag = 0;
    UINT32_T i=0;

    sg_incomplete_firmware_info = (TKL_OTA_FIRMWARE_INFO_T*)(&sg_incomplete_firmware_info_buffer);
    for (i = 0; i < TKL_OTA_ERASE_MIN_SIZE; i += ota_info_len) {
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR + i;
        tkl_flash_read(ota_info_addr, (UINT8_T *)sg_incomplete_firmware_info, ota_info_len);
        if (EM_OTA_SETTING_INVALID == sg_incomplete_firmware_info->flag) {
            if (0 != i) {
                do_effect_flag = 1;
            }
            continue;
        } else if (EM_OTA_SETTING_VALID == sg_incomplete_firmware_info->flag) {
            do_effect_flag = 2;
            break;
        } else if (EM_OTA_SETTING_NULL == sg_incomplete_firmware_info->flag) {
            if (0 == i) {
                do_effect_flag = 1;
            }
            break;
        }
    }

    if (1 == do_effect_flag) {
        if (i > ota_info_len) {
            tkl_flash_erase_ota(BOARD_FLASH_OTA_START_ADDR, BOARD_FLASH_OTA_SIZE);
        }
        tkl_flash_erase_ota(BOARD_FLASH_OTA_INFO_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR;
        memset((void*)sg_incomplete_firmware_info, 0, ota_info_len);
        tuya_ble_ota_info_save();
        // TY_PRINTF("OTA INFO LOAD - 1");
    } else if (2 == do_effect_flag) {
        sg_data_crc32 = tkl_ota_get_crc32_in_flash(sg_incomplete_firmware_info->len);
        if (sg_data_crc32 == sg_incomplete_firmware_info->crc32) {
            // TY_PRINTF("OTA INFO LOAD - 2: 0x%08x, 0x%08x", sg_data_crc32);
        } else {
        tkl_flash_erase_ota(BOARD_FLASH_OTA_START_ADDR, BOARD_FLASH_OTA_SIZE);
        tkl_flash_erase_ota(BOARD_FLASH_OTA_INFO_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR;
        memset((void*)sg_incomplete_firmware_info, 0, ota_info_len);
        tuya_ble_ota_info_save();
            // TY_PRINTF("OTA INFO LOAD - 3: 0x%08x, 0x%08x", sg_data_crc32, sg_incomplete_firmware_info->crc32);
        }
    } else if (0 == do_effect_flag) {
        memset((void*)sg_incomplete_firmware_info, 0, ota_info_len);
        // TY_PRINTF("OTA INFO LOAD - 4");
    } else {
        tkl_flash_erase_ota(BOARD_FLASH_OTA_INFO_ADDR, TKL_OTA_ERASE_MIN_SIZE);
        ota_info_addr = (UINT32_T)BOARD_FLASH_OTA_INFO_ADDR;
        memset((void*)sg_incomplete_firmware_info, 0, ota_info_len);
        tuya_ble_ota_info_save();
        // TY_PRINTF("OTA INFO LOAD - 5");
    }
    return 0;
}

