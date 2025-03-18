/**
 * @file tuya_ble_accessory_demo.c
 * @brief This is tuya_ble_accessory_demo file
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright 2023-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "board.h"
#include "tuya_ble_type.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_product_test.h"
#include "tuya_ble_ota.h"
#include "tuya_ble_event.h"
#include "tuya_ble_log.h"
#include "tuya_ble_internal_config.h"
#include "tuya_ble_accessory_demo.h"
#include "tuya_ble_feature_accessory.h"
#include "tuya_ble_accessory_uart_protocol_handler_demo.h"

#include "tal_memory.h"
#include "tal_util.h"
#include "tal_flash.h"
#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"
#include "tal_uart.h"

#if (TUYA_BLE_ACCESSORY_MOUNT_SUPPORTED != 0)

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR
#define FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR (0x64000)
#endif

#ifndef FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR
#define FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR (0x64000+TUYA_NV_ERASE_MIN_SIZE*1)
#endif

#define FLASH_ACCESSORY_ACTIVE_INFO_MAX_STORAGE_NUM (128)

#define FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT (32)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
tuya_ble_accessory_connection_info_t accessory_info[ACCESSORY_MOUNT_MAX];

STATIC volatile UINT32_T current_accessory_info_storage_addr = 0;
STATIC TIMER_ID tuya_ble_accessory_timer_id = NULL;
STATIC UINT8_T accessory_status_last = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T tuya_ble_accessory_timer_handler(TIMER_ID timer_id, VOID_T *arg)
{
    extern TAL_UART_CFG_T tal_uart_cfg;
    TUYA_GPIO_LEVEL_E level;
    TUYA_GPIO_BASE_CFG_T gpio_cfg = {
        .mode = TUYA_GPIO_PULLDOWN,
        .direct = TUYA_GPIO_INPUT,
        .level = TUYA_GPIO_LEVEL_LOW,
    };

    // is production test?
    // is device ota?
    // is accessory mcu ota?
    if (tuya_ble_internal_production_test_with_ble_flag_get()) {
        return;
    }
    if (tuya_ble_ota_get_status() == TUYA_BLE_OTA_DATA) {
        return;
    }
    if (tuya_ble_accessory_mcu_ota_status()) {
        return;
    }

    // some chip gpio unsupport pull down input so rely on hardware that pull down
    tal_uart_deinit(TUYA_UART_NUM_0);
    tal_gpio_init(TUYA_GPIO_NUM_2, &gpio_cfg);
    tal_gpio_read(TUYA_GPIO_NUM_2, &level);
    tal_uart_init(TUYA_UART_NUM_0, &tal_uart_cfg);

    if (level != accessory_status_last) {
        accessory_status_last = level;
        TAL_PR_DEBUG("RX->%d", level);
        if (level) {
            // accessory connected keep device awake
            // before setup session between device and aceesory, don't set accessory connect status to 1.
        } else {
            // accessory disconnected enable sleep
            accessory_info[0].connect_status = 0;
        }
        tuya_ble_accessory_connection_info_report(&accessory_info[0]); //BLE->APP
    }
}

VOID_T tuya_ble_custom_app_uart_common_process(UINT8_T *p_in_data, UINT16_T in_len)
{
    tuya_ble_accessory_uart_protocol_process(0x00, p_in_data, in_len);
}

STATIC OPERATE_RET tuya_ble_accessory_connection_info_reading_handle(tuya_ble_accessory_info_reading_data_t attach_info)
{
    OPERATE_RET ret       = OPRT_OK;
    UINT8_T *p_buf        = NULL;
    UINT16_T total_len    = 0;
    UINT16_T index        = 0;
    UINT8_T accessory_num = 0;
    UINT8_T i = 0;
    UINT8_T j = 0;
    UINT8_T accesory_index[ACCESSORY_MOUNT_MAX] = {0};
    UINT8_T key_in[16] = {0};
    UINT8_T output[16] = {0};

    if ((attach_info.attach_len > 0) && (attach_info.attach_data==NULL)) {
        TAL_PR_ERR("error,invalid param!");
        return OPRT_INVALID_PARM;
    }

    for (i = 0; i < ACCESSORY_MOUNT_MAX; i++) {
        if ((accessory_info[i].device_id_len != DEVICE_ID_LEN)
            ||((accessory_info[i].pid_len != TUYA_BLE_PRODUCT_ID_MAX_LEN) && (accessory_info[i].pid_len != TUYA_BLE_PRODUCT_ID_DEFAULT_LEN))
            ||((accessory_info[i].accessory_fw_info_len%7) != 0)
            ||(accessory_info[i].p_accessory_fw_info == NULL)
            ||(tal_util_buffer_value_is_all_x(accessory_info[i].device_id, DEVICE_ID_LEN, 0x00) == TRUE)
            ||(tal_util_buffer_value_is_all_x(accessory_info[i].common_pid, TUYA_BLE_PRODUCT_ID_MAX_LEN, 0x00) == TRUE)) {
            continue;
        }
        accessory_num++;
        accesory_index[j++] = i;
        total_len = 7 + accessory_info[i].device_id_len + accessory_info[i].pid_len + accessory_info[i].accessory_fw_info_len + 18;
    }
    total_len += 3;
    total_len += attach_info.attach_len;

    if ((total_len <= 4) || (total_len > TUYA_BLE_SEND_MAX_DATA_LEN)) {
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    p_buf = (UINT8_T *)tal_malloc(total_len);
    if (p_buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    total_len = 0;
    p_buf[total_len++] = 0; //version
    p_buf[total_len++] = attach_info.attach_len;
    if (attach_info.attach_len > 0) {
        memcpy(p_buf + total_len, attach_info.attach_data, attach_info.attach_len);
        total_len += attach_info.attach_len;
    }
    p_buf[total_len++] = accessory_num;
    for (i = 0; i < j; i++) {
        index = accesory_index[i];
        p_buf[total_len++] = accessory_info[index].device_id_len;

        memcpy(p_buf+total_len, accessory_info[index].device_id, accessory_info[index].device_id_len);
        total_len += accessory_info[index].device_id_len;

        p_buf[total_len++] = accessory_info[index].connect_status;
        p_buf[total_len++] = accessory_info[index].pid_type;
        p_buf[total_len++] = accessory_info[index].pid_len;

        memcpy(p_buf + total_len, accessory_info[index].common_pid, accessory_info[index].pid_len);
        total_len += accessory_info[index].pid_len;

        p_buf[total_len++] = accessory_info[index].short_id>>8;
        p_buf[total_len++] = accessory_info[index].short_id;

        p_buf[total_len++] = accessory_info[index].accessory_fw_info_len;
        memcpy(p_buf+total_len, (UINT8_T*)accessory_info[index].p_accessory_fw_info, accessory_info[index].accessory_fw_info_len);
        total_len += accessory_info[index].accessory_fw_info_len;

        p_buf[total_len++] = 0x00;
        p_buf[total_len++] = 16;
        memcpy(key_in, tuya_ble_current_para.auth_settings.auth_key, 16);
        tuya_ble_accessory_encrypt(key_in, 16, accessory_info[index].device_id,16, output);
        memcpy(p_buf+total_len, output, SIZEOF(output));
        total_len += SIZEOF(output);
    }

TAL_PR_HEXDUMP_DEBUG("Info", p_buf, total_len);
    ret = tuya_ble_accessory_info_report(p_buf, total_len);
    tal_free(p_buf);

    return ret;
}

STATIC OPERATE_RET tuya_ble_accessory_active_info_receive_handle(tuya_ble_accessory_active_info_data_t accessory_active_info_data)
{
    UINT8_T active_accessory_num = 0;
    UINT8_T i = 0;
    UINT8_T device_id_len = 0;
    UINT8_T active_status = 0;
    UINT8_T index = 0;
    UINT8_T info_index = 0;
    tuya_ble_accessory_active_info_t active_info;
    tuya_ble_accessory_active_info_t cmp_active_info;

    active_accessory_num = accessory_active_info_data.active_info_data[index++];
    if (((accessory_active_info_data.active_info_len - 1) % 20) != 0) {
        TAL_PR_ERR("Invalid info len.");
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    active_accessory_num = (active_accessory_num>ACCESSORY_MOUNT_MAX)?(ACCESSORY_MOUNT_MAX):(active_accessory_num);

    for (i = 0; i < active_accessory_num; i++) {
        active_info.short_id = ((UINT16_T)accessory_active_info_data.active_info_data[index++] << 8) & 0xFF00;
        active_info.short_id |= accessory_active_info_data.active_info_data[index++];
        device_id_len = accessory_active_info_data.active_info_data[index++];
        if (device_id_len != DEVICE_ID_LEN) {
            TAL_PR_ERR("Invalid device id.");
            return OPRT_NOT_FOUND;
        }
        memcpy(active_info.device_id, accessory_active_info_data.active_info_data + index, DEVICE_ID_LEN);
        index += DEVICE_ID_LEN;
        active_status = accessory_active_info_data.active_info_data[index++]; //0-active 1-failed

        if ((active_status == 0) && (tuya_ble_accessory_connection_info_find_by_device_id(active_info.device_id, &info_index) == TRUE)) {
            accessory_info[info_index].short_id = active_info.short_id;
            accessory_info[info_index].active_status = 1; //1-active 0-not active
            if (tuya_ble_accessory_storage_read_active_info_by_device_id(active_info.device_id, &cmp_active_info) == TRUE) {
                if (cmp_active_info.short_id != active_info.short_id) {
                    tuya_ble_accessory_storage_write_active_info(&active_info);
                }
            } else {
                tuya_ble_accessory_storage_write_active_info(&active_info);
            }
            tuya_ble_accessory_send_custom_event(TUYA_BLE_ACCESSORY_EVT_CONNECTE_STATUS, (UINT8_T*)&accessory_info[info_index]);
        }
    }
    return OPRT_OK;
}

OPERATE_RET tuya_ble_accessory_connection_info_report(tuya_ble_accessory_connection_info_t *p_accessory_info)
{
    OPERATE_RET ret    = OPRT_OK;
    UINT8_T *p_buf     = NULL;
    UINT16_T total_len = 0;
    UINT16_T index     = 0;
    UINT8_T key_in[16] = {0};
    UINT8_T output[16] = {0};

    if (tuya_ble_connect_status_get() != BONDING_CONN) {
        return OPRT_NETWORK_ERROR;
    }

    if (((p_accessory_info->device_id_len) != DEVICE_ID_LEN)
        ||((p_accessory_info->pid_len != TUYA_BLE_PRODUCT_ID_MAX_LEN) && (p_accessory_info->pid_len != TUYA_BLE_PRODUCT_ID_DEFAULT_LEN))
        ||((p_accessory_info->accessory_fw_info_len % 7) != 0)
        ||(p_accessory_info->p_accessory_fw_info == NULL)
        ||(tal_util_buffer_value_is_all_x(p_accessory_info->device_id, DEVICE_ID_LEN, 0x00) == TRUE)
        ||(tal_util_buffer_value_is_all_x(p_accessory_info->common_pid, TUYA_BLE_PRODUCT_ID_MAX_LEN, 0x00) == TRUE)) {
        TAL_PR_ERR("error,accessory info invalid!");
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    total_len = 7 + p_accessory_info->device_id_len + p_accessory_info->pid_len + p_accessory_info->accessory_fw_info_len + 18;
    total_len += 3;

    if (total_len > TUYA_BLE_SEND_MAX_DATA_LEN) {
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    p_buf = (UINT8_T *)tal_malloc(total_len);
    if (p_buf == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    p_buf[index++] = 0; //version
    p_buf[index++] = 0; //must be 0.
    p_buf[index++] = 1; //must be 1.

    p_buf[index++] = p_accessory_info->device_id_len;

    memcpy(p_buf + index, p_accessory_info->device_id, p_accessory_info->device_id_len);
    index += p_accessory_info->device_id_len;

    p_buf[index++] = p_accessory_info->connect_status;
    p_buf[index++] = p_accessory_info->pid_type;
    p_buf[index++] = p_accessory_info->pid_len;

    memcpy(p_buf+index, p_accessory_info->common_pid, p_accessory_info->pid_len);
    index += p_accessory_info->pid_len;

    p_buf[index++] = p_accessory_info->short_id >> 8;
    p_buf[index++] = p_accessory_info->short_id;

    p_buf[index++] = p_accessory_info->accessory_fw_info_len;
    memcpy(p_buf + index, (UINT8_T*)p_accessory_info->p_accessory_fw_info, p_accessory_info->accessory_fw_info_len);
    index += p_accessory_info->accessory_fw_info_len;

    p_buf[index++] = 0x00;
    p_buf[index++] = 16;
    memcpy(key_in, tuya_ble_current_para.auth_settings.auth_key, 16);
    tuya_ble_accessory_encrypt(key_in, 16, p_accessory_info->device_id, 16, output);
    memcpy(p_buf + index, output, SIZEOF(output));
    index += SIZEOF(output);

    ret = tuya_ble_accessory_info_report(p_buf, index);
    tal_free(p_buf);

    return ret;
}

STATIC OPERATE_RET tuya_ble_accessory_storage_auto_revocer_check(VOID_T)
{
    UINT32_T addr = 0;
    UINT32_T offset = 0;
    UINT32_T write_able_offset = 0;
    UINT32_T write_able_offset_in_backup_area = 0;
    tuya_ble_accessory_active_info_t temp_accessory_active_info;
    UINT8_T buf[256] = {0};

    for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR+offset;
        tal_flash_read(addr, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));

        if (tal_util_buffer_value_is_all_x((UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t), 0xff)==TRUE) {
            write_able_offset = offset;
            break;
        }
    }

    for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR+offset;
        tal_flash_read(addr, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));
        if (tal_util_buffer_value_is_all_x((UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t), 0xff)==TRUE) {
            write_able_offset_in_backup_area = offset;
            break;
        }
    }
    if (write_able_offset == write_able_offset_in_backup_area) {
        return OPRT_OK;
    } else if (write_able_offset > write_able_offset_in_backup_area) {
        tal_flash_erase(FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR, TUYA_NV_ERASE_MIN_SIZE);
        for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += 256) {
            addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR+offset;
            tal_flash_read(addr, buf, 256);
            addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR+offset;
            tal_flash_write(addr, buf, 256);
        }
        return OPRT_COM_ERROR;
    } else {
        tal_flash_erase(FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR, TUYA_NV_ERASE_MIN_SIZE);
        for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE;offset += 256) {
            addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR + offset;
            tal_flash_read(addr, buf, 256);
            addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + offset;
            tal_flash_write(addr, buf, 256);
        }
        return OPRT_NOT_FOUND;
    }
}

STATIC VOID_T tuya_ble_accessory_storage_slim(VOID_T)
{
    UINT32_T offset = 0;
    UINT32_T addr   = 0;
    UINT32_T write_able_offset = 0;
    tuya_ble_accessory_active_info_t temp_accessory_active_info;
    UINT8_T *buf = NULL;

    /* Migrate the data from the latest half sector to the backup sector and then copy the data from the backup sector to the storage sector.*/
    buf = (UINT8_T *)tal_malloc(256);
    if (buf == NULL) {
        TAL_PR_ERR("malloc failed");
        return;
    }

    tal_flash_erase(FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR, TUYA_NV_ERASE_MIN_SIZE);
    for (offset = TUYA_NV_ERASE_MIN_SIZE / 2; offset < TUYA_NV_ERASE_MIN_SIZE; offset += 256) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + offset;
        tal_flash_read(addr, buf, 256);
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR + (offset - TUYA_NV_ERASE_MIN_SIZE / 2);
        tal_flash_write(addr, buf, 256);
    }

    tal_flash_erase(FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR, TUYA_NV_ERASE_MIN_SIZE);
    for (offset = 0; offset < (TUYA_NV_ERASE_MIN_SIZE / 2); offset += 256) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_BACK_ADDR + offset;
        tal_flash_read(addr, buf, 256);
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + offset;
        tal_flash_write(addr, buf, 256);
    }

    for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + offset;
        tal_flash_read(addr, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));

        if (tal_util_buffer_value_is_all_x((UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t), 0xFF)) {
            write_able_offset = offset;
            break;
        }
    }

    current_accessory_info_storage_addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + write_able_offset - FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT;
}

STATIC VOID_T tuya_ble_accessory_storage_init(VOID_T)
{
    UINT32_T offset = 0;
    UINT32_T addr = 0;
    UINT32_T write_able_offset = 0;
    tuya_ble_accessory_active_info_t temp_accessory_active_info;

    tuya_ble_accessory_storage_auto_revocer_check();

    for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT) {
        addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + offset;
        tal_flash_read(addr, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));

        if (tal_util_buffer_value_is_all_x((UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t), 0xFF)) {
            write_able_offset = offset;
            break;
        }
    }

    current_accessory_info_storage_addr = FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + write_able_offset - FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT;
}

UINT32_T tuya_ble_accessory_storage_write_active_info(tuya_ble_accessory_active_info_t *p_accessory_active_info)
{
    tuya_ble_accessory_active_info_t verify_data;

    p_accessory_active_info->info_len = SIZEOF(tuya_ble_accessory_active_info_t)-6;
    p_accessory_active_info->info_crc = tal_util_crc32((UINT8_T*)p_accessory_active_info + 6, p_accessory_active_info->info_len, NULL);

    if (current_accessory_info_storage_addr < FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR || current_accessory_info_storage_addr >= (FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + TUYA_NV_ERASE_MIN_SIZE)) {
        tuya_ble_accessory_storage_init();
    }

    current_accessory_info_storage_addr += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT;

    if ((current_accessory_info_storage_addr >= ((UINT32_T)FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR + TUYA_NV_ERASE_MIN_SIZE))) {
        tuya_ble_accessory_storage_slim();
    }

    tal_flash_write(current_accessory_info_storage_addr, (UINT8_T *)p_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));

    tal_flash_read(current_accessory_info_storage_addr, (UINT8_T*)&verify_data, SIZEOF(tuya_ble_accessory_active_info_t));
    if (verify_data.info_crc == tal_util_crc32((UINT8_T*)&verify_data + 6, verify_data.info_len, NULL)) {
        return EM_STORAGE_SUCCESS;
    } else {
        return EM_STORAGE_FAILED;
    }
}

BOOL_T tuya_ble_accessory_storage_read_active_info_by_device_id(UINT8_T *device_id, tuya_ble_accessory_active_info_t *p_accessory_active_info)
{
    UINT32_T addr = 0;
    UINT32_T offset = 0;
    tuya_ble_accessory_active_info_t temp_accessory_active_info = {0};

    /* Find from the end to the front. */
    for (offset = 0; offset < TUYA_NV_ERASE_MIN_SIZE; offset += FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_UNIT) {
        addr = current_accessory_info_storage_addr - offset;
        if (addr < FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR) {
            break;
        }
        tal_flash_read(addr, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));

        if (temp_accessory_active_info.info_len > (SIZEOF(tuya_ble_accessory_active_info_t) - 6)) {
            continue;
        }

        if (temp_accessory_active_info.info_crc == tal_util_crc32((UINT8_T*)&temp_accessory_active_info + 6, temp_accessory_active_info.info_len, NULL)) {
            if (memcmp(temp_accessory_active_info.device_id, device_id, DEVICE_ID_LEN) == 0) {
                // if same then copy info to device list
                if (p_accessory_active_info != NULL) {
                    memcpy((UINT8_T*)p_accessory_active_info, (UINT8_T*)&temp_accessory_active_info, SIZEOF(tuya_ble_accessory_active_info_t));
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}

OPERATE_RET tuya_ble_accessory_connection_info_create(UINT8_T port_id, tuya_ble_accessory_connection_info_t app_accessory_info)
{
    tuya_ble_accessory_active_info_t accessory_active_info;

    if (port_id >= ACCESSORY_MOUNT_MAX) {
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    if (accessory_info[port_id].p_accessory_fw_info != NULL) {
        tal_free(accessory_info[port_id].p_accessory_fw_info);
    }

    memcpy((UINT8_T*)&accessory_info[port_id], (UINT8_T *)&app_accessory_info, SIZEOF(tuya_ble_accessory_connection_info_t));

    accessory_info[port_id].p_accessory_fw_info = (UINT8_T *)tal_malloc(app_accessory_info.accessory_fw_info_len);
    if (accessory_info[port_id].p_accessory_fw_info == NULL) {
        return OPRT_MALLOC_FAILED;
    } else {
        memcpy(accessory_info[port_id].p_accessory_fw_info, app_accessory_info.p_accessory_fw_info, app_accessory_info.accessory_fw_info_len);
    }
    accessory_info[port_id].port_id = port_id;
    accessory_info[port_id].connect_status = 1;

    if (tuya_ble_accessory_storage_read_active_info_by_device_id(accessory_info[port_id].device_id, &accessory_active_info) == TRUE) {
        accessory_info[port_id].short_id = accessory_active_info.short_id;
        accessory_info[port_id].active_status = 1;
    }

    return OPRT_OK;
}

VOID_T tuya_ble_accessory_connection_info_delete(UINT8_T port_id)
{
    memset((UINT8_T*)&accessory_info[port_id], 0, SIZEOF(tuya_ble_accessory_connection_info_t));
    tal_free((UINT8_T*)accessory_info[port_id].p_accessory_fw_info);
}

BOOL_T tuya_ble_accessory_connection_info_find_by_device_id(UINT8_T* device_id, UINT8_T *index)
{
    UINT8_T i;
    for (i = 0; i < ACCESSORY_MOUNT_MAX; i++) {
        if (memcmp(accessory_info[i].device_id, device_id, DEVICE_ID_LEN) == 0) {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL_T tuya_ble_accessory_connection_info_find_by_short_id(UINT16_T short_id, UINT8_T *index)
{
    UINT8_T i;

    /*short_id:big-end*/
    short_id = ((short_id & 0xFF00) >> 8) | ((short_id & 0x00FF) << 8); //to small-end

    for (i = 0; i < ACCESSORY_MOUNT_MAX; i++) {
        if (accessory_info[i].short_id == short_id) {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

OPERATE_RET tuya_ble_accessory_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;
    tuya_ble_accessory_storage_init();

    #if 0
    /* Reduce the possibility of prolonged flash manipulation at runtime.*/
    if (current_accessory_info_storage_addr>FLASH_ACCESSORY_ACTIVE_INFO_STORAGE_ADDR+TUYA_NV_ERASE_MIN_SIZE/2) {
        tuya_ble_accessory_storage_slim();
    }
    #endif

    memset((UINT8_T*)accessory_info, 0, SIZEOF(accessory_info));
    for (UINT8_T i = 0; i < ACCESSORY_MOUNT_MAX; i++) {
        accessory_info[i].port_id = i;
    }

    ret = tal_sw_timer_create(tuya_ble_accessory_timer_handler, NULL, &tuya_ble_accessory_timer_id);
    if (ret) {
        TAL_PR_ERR("timer create failed");
        return ret;
    }
    ret = tal_sw_timer_start(tuya_ble_accessory_timer_id, 1000, TAL_TIMER_CYCLE);
    if (ret) {
        TAL_PR_ERR("timer start failed");
        return ret;
    }

    return OPRT_OK;
}

VOID_T tuya_ble_accessory_sdk_cb_event_handler(tuya_ble_cb_evt_param_t* event)
{
    tuya_ble_accessory_id_info_t *p_id_info;
    UINT8_T info_index;

    switch (event->evt) {
        case TUYA_BLE_CB_EVT_ACCESSORY_INFO_READING: {
            //0x8020 APP->BLE
            TAL_PR_DEBUG("TUYA_BLE_CB_EVT_ACCESSORY_INFO_READING");
            tuya_ble_accessory_connection_info_reading_handle(event->accessory_info_reading_data);
        } break;
        case TUYA_BLE_CB_EVT_ACCESSORY_INFO_REPORT_RESPONSE: {
            //0x8021 APP->BLE
            TAL_PR_DEBUG("TUYA_BLE_CB_EVT_ACCESSORY_INFO_REPORT_RESPONSE");
        } break;
        case TUYA_BLE_CB_EVT_ACCESSORY_ACTIVE_INFO_RECEIVED: {
            //0x8022 APP->BLE
            TAL_PR_DEBUG("TUYA_BLE_CB_EVT_ACCESSORY_ACTIVE_INFO_RECEIVED");
            tuya_ble_accessory_active_info_receive_handle(event->accessory_active_info_data);
        } break;
        case TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_DATA_RECEIVED: {
            TAL_PR_HEXDUMP_DEBUG("DP RECEIVE - ID_INFO ", event->dp_data_with_src_type_received_data.p_add_info, event->dp_data_with_src_type_received_data.add_info_len);
            TAL_PR_HEXDUMP_DEBUG("DP RECEIVE - DP DATA ", event->dp_data_with_src_type_received_data.p_data, event->dp_data_with_src_type_received_data.data_len);

            if (event->dp_data_with_src_type_received_data.src_type != DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT) {
                break;
            }
            p_id_info = (tuya_ble_accessory_id_info_t*)event->dp_data_with_src_type_received_data.p_add_info;
            if ((p_id_info->id_type != 0) || (p_id_info->id_len != 2)) {
                TAL_PR_ERR("TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_DATA_RECEIVED id info error.");
                break;
            }

            if (tuya_ble_accessory_connection_info_find_by_short_id(p_id_info->short_id, &info_index) == TRUE) {
                tuya_ble_accessory_uart_cmd_dp_data_send_to_accessory(accessory_info[info_index].port_id, event->dp_data_with_src_type_received_data.sn, event->dp_data_with_src_type_received_data.p_data, event->dp_data_with_src_type_received_data.data_len);
                /*dp report test.*/
                tuya_ble_dp_data_with_src_type_send(event->dp_data_with_src_type_received_data.sn, DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, \
                    event->dp_data_with_src_type_received_data.add_info_len, event->dp_data_with_src_type_received_data.p_add_info, \
                    event->dp_data_with_src_type_received_data.p_data, event->dp_data_with_src_type_received_data.data_len);

                /*dp report test, with response.*/
                //tuya_ble_dp_data_with_src_type_send(event->dp_data_with_src_type_received_data.sn, DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITH_RESPONSE, \
                //    event->dp_data_with_src_type_received_data.add_info_len, event->dp_data_with_src_type_received_data.p_add_info, \
                //    event->dp_data_with_src_type_received_data.p_data, event->dp_data_with_src_type_received_data.data_len);

                /*dp report test, with time*/
                //UINT32_T timestamp = 0;
                //tuya_ble_dp_data_with_src_type_and_time_send(event->dp_data_with_src_type_received_data.sn, DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT, DP_SEND_FOR_CLOUD_PANEL, DP_TIME_TYPE_UNIX_TIMESTAMP, \
                //    &timestamp, event->dp_data_with_src_type_received_data.add_info_len, event->dp_data_with_src_type_received_data.p_add_info, \
                //    event->dp_data_with_src_type_received_data.p_data, event->dp_data_with_src_type_received_data.data_len);
            } else {
                TAL_PR_ERR("unknown device: %d", p_id_info->short_id);
            }
        } break;
        case TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_QUERY: {
            TAL_PR_HEXDUMP_DEBUG("DP QUERY - ID_INFO ", event->dp_query_data_with_src_type.p_add_info, 4);

            if (event->dp_query_data_with_src_type.src_type != DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT) {
                break;
            }

            p_id_info = (tuya_ble_accessory_id_info_t*)event->dp_query_data_with_src_type.p_add_info;
            if ((p_id_info->id_type != 0) || (p_id_info->id_len != 2)) {
                TAL_PR_ERR("TUYA_BLE_CB_EVT_WITH_SRC_TYPE_DP_QUERY id info error.");
                break;
            }

            if (tuya_ble_accessory_connection_info_find_by_short_id(p_id_info->short_id, &info_index) == TRUE) {
                TAL_PR_HEXDUMP_DEBUG("DP Query - accessory:", event->dp_query_data_with_src_type.p_data, event->dp_query_data_with_src_type.data_len);

                /*send work status*/
                tuya_ble_accessory_uart_cmd_send_work_mode(accessory_info[info_index].port_id);

                /*send dp query*/
                tuya_ble_accessory_uart_cmd_dp_query(accessory_info[info_index].port_id, event->dp_query_data_with_src_type.p_data, event->dp_query_data_with_src_type.data_len);
            } else {
                TAL_PR_DEBUG("unknown device: %d", p_id_info->short_id);
            }
        } break;
        case TUYA_BLE_CB_EVT_DP_DATA_WITH_SRC_TYPE_SEND_RESPONSE: {
            if (event->dp_with_src_type_send_response_data.src_type != DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT) {
                break;
            }

            p_id_info = (tuya_ble_accessory_id_info_t*)event->dp_with_src_type_send_response_data.p_add_info;
            if ((p_id_info->id_type != 0) || (p_id_info->id_len != 2)) {
                TAL_PR_ERR("TUYA_BLE_CB_EVT_DP_DATA_WITH_SRC_TYPE_SEND_RESPONSE id info error.");
                break;
            }

            if (tuya_ble_accessory_connection_info_find_by_short_id(p_id_info->short_id, &info_index) == TRUE) {
                TAL_PR_DEBUG("DP Send Respondse - accessory:sn:0x%08X,type:%d,mode:%d,ack:%d,status:%d", event->dp_with_src_type_send_response_data.sn,\
                event->dp_with_src_type_send_response_data.type, event->dp_with_src_type_send_response_data.mode, event->dp_with_src_type_send_response_data.ack, event->dp_with_src_type_send_response_data.status);
            } else {
                TAL_PR_ERR("unknown device: %d", p_id_info->short_id);
            }
        } break;
        case TUYA_BLE_CB_EVT_DP_DATA_WITH_SRC_TYPE_AND_TIME_SEND_RESPONSE: {
            if (event->dp_with_src_type_and_time_send_response_data.src_type != DATA_SOURCE_TYPE_ACCESSORY_EQUIPMENT) {
                break;
            }

            p_id_info = (tuya_ble_accessory_id_info_t*)event->dp_with_src_type_and_time_send_response_data.p_add_info;
            if ((p_id_info->id_type != 0) || (p_id_info->id_len != 2)) {
                TAL_PR_ERR("TUYA_BLE_CB_EVT_DP_DATA_WITH_SRC_TYPE_AND_TIME_SEND_RESPONSE id info error.");
                break;
            }

            if (tuya_ble_accessory_connection_info_find_by_short_id(p_id_info->short_id, &info_index) == TRUE) {
                TAL_PR_DEBUG("DP Send Respondse - accessory:sn:0x%08X,type:%d,mode:%d,ack:%d,status:%d", event->dp_with_src_type_and_time_send_response_data.sn,\
                event->dp_with_src_type_and_time_send_response_data.type, event->dp_with_src_type_and_time_send_response_data.mode, event->dp_with_src_type_and_time_send_response_data.ack, event->dp_with_src_type_and_time_send_response_data.status);
            } else {
                TAL_PR_ERR("unknown device,%d", p_id_info->short_id);
            }
        } break;
        case TUYA_BLE_CB_EVT_ACCESSORY_OTA_DATA: {
            p_id_info = (tuya_ble_accessory_id_info_t*)event->accessory_ota_data.p_data;
            if ((p_id_info->id_type != 0) || (p_id_info->id_len != 2)) {
                TAL_PR_ERR("TUYA_BLE_CB_EVT_ACCESSORY_OTA_DATA id info error.");
                break;
            }

            if (tuya_ble_accessory_connection_info_find_by_short_id(p_id_info->short_id, &info_index) == TRUE) {
                tuya_ble_accessory_ota_data_from_ble_handler(accessory_info[info_index].port_id, event->accessory_ota_data.type, event->accessory_ota_data.p_data + 4, event->accessory_ota_data.data_len - 4);
            } else {
                TAL_PR_ERR("unknown device.");
            }

        } break;
        case TUYA_BLE_CB_EVT_CONNECT_STATUS: {
            UINT8_T n_index = 0;
            if (event->connect_status == BONDING_UNCONN) {
                /*send work status*/
                for (n_index = 0; n_index < ACCESSORY_MOUNT_MAX; n_index++) {
                tuya_ble_accessory_uart_cmd_send_work_mode(accessory_info[n_index].port_id);
                }
            }
        } break;
        default: {

        } break;
    }
}

#endif

