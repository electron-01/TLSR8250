/**
 * @file tuya_ble_attach_ota.c
 * @brief This is tuya_ble_attach_ota file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"

#include "board.h"

#include "tal_memory.h"
#include "tal_log.h"
#include "tal_system.h"
#include "tal_ota.h"
#include "tal_bluetooth.h"
#include "tal_sw_timer.h"
#include "tal_util.h"

#include "tuya_ble_log.h"
#include "tuya_ble_api.h" //tuya_ble_ota_response
#include "tuya_ble_attach_ota.h"
#include "tuya_ble_attach_ota_port.h"
#include "tuya_ble_main.h"

#if defined(TUYA_BLE_FEATURE_ATTACH_OTA_ENABLE) && (TUYA_BLE_FEATURE_ATTACH_OTA_ENABLE == 1)

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TUYA_BLE_ATTACH_OTA_STATE_UNKNOWN   (-1)

#define TUYA_BLE_ATTACH_OTA_VERSION         (3)
#define TUYA_BLE_ATTACH_OTA_PKG_LEN         (512)

#define TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN    (16)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
#pragma pack(1)
typedef struct {
    UINT8_T  flag;
    UINT8_T  ota_version;
    UINT8_T  type;
    UINT32_T version;
    UINT16_T package_maxlen;
} tuya_ble_attach_ota_req_rsp_t;

typedef struct {
    UINT8_T  type;
    UINT8_T  pid[8];
    UINT32_T version;
    UINT8_T  md5[TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN];
    UINT32_T file_len;
    UINT32_T crc32;
} tuya_ble_attach_ota_file_info_t;

typedef struct {
    UINT8_T  type;
    UINT8_T  state;
    UINT32_T old_file_len;
    UINT32_T old_crc32;
    UINT8_T  old_md5[TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN];
} tuya_ble_attach_ota_file_info_rsp_t;

typedef struct {
    UINT8_T  type;
    UINT32_T offset;
} tuya_ble_attach_ota_file_offset_t;

typedef struct {
    UINT8_T  type;
    UINT32_T offset;
} tuya_ble_attach_ota_file_offset_rsp_t;

typedef struct {
    UINT8_T  type;
    UINT16_T pkg_id;
    UINT16_T len;
    UINT16_T crc16;
    UINT8_T  data[];
} tuya_ble_attach_app_ota_data_t;

typedef struct {
    UINT8_T type;
    UINT8_T state;
} tuya_ble_attach_ota_data_rsp_t;

typedef struct {
    UINT8_T  type;
    UINT8_T state;
} tuya_ble_attach_ota_end_rsp_t;

typedef struct {
    UINT32_T* p_attach_ota_idx;
    UINT32_T  attach_ota_version;
    UINT32_T* p_firmware_crc32;
    UINT8_T*  p_firmware_md5;
} tuya_ble_attach_ota_pri_param_t;
#pragma pack()

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC INT32_T     sg_attach_ota_state             = TUYA_BLE_ATTACH_OTA_STATE_UNKNOWN;
STATIC BOOL_T      sg_attach_ota_success           = FALSE;
STATIC UINT32_T    sg_attach_ota_type              = 0;
STATIC UINT32_T    sg_attach_ota_idx               = 0;

STATIC INT32_T     sg_attach_data_package_id       = 0;
STATIC UINT32_T    sg_attach_data_offset           = 0;

STATIC UINT32_T    sg_attach_firmware_len          = 0;
STATIC UINT32_T    sg_attach_firmware_crc32        = 0;
STATIC UINT8_T     sg_attach_firmware_md5[TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN]   = {0};
STATIC TUYA_OTA_FIRMWARE_INFO_T* sg_attach_incomplete_firmware_info           = NULL;

STATIC tuya_ble_attach_ota_pri_param_t tuya_ble_attach_ota_pri_param          = {0};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
extern UINT16_T tuya_app_get_conn_handle(VOID_T);




STATIC UINT32_T tuya_ble_attach_ota_enter(VOID_T)
{
    //Enter the OTA process, disconnect Related
    sg_attach_ota_state = TUYA_BLE_OTA_REQ;
    sg_attach_ota_success = FALSE;

    sg_attach_data_package_id = -1;
    sg_attach_data_offset = 0;

    sg_attach_firmware_len = 0;
    sg_attach_firmware_crc32 = 0;
    memset(sg_attach_firmware_md5, 0, TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN);


    TAL_BLE_PEER_INFO_T peer_info = {0};
    peer_info.conn_handle = tuya_app_get_conn_handle();
    TAL_BLE_CONN_PARAMS_T conn_param = {0};
    conn_param.min_conn_interval = 15*4/5;
    conn_param.max_conn_interval = 30*4/5;
    conn_param.latency = 0;
    conn_param.conn_sup_timeout = 6000/10;
    conn_param.connection_timeout = 0;
    tal_ble_conn_param_update(peer_info, &conn_param);

    tuya_ble_attach_ota_port_start_notify(0, TUYA_OTA_FULL, TUYA_OTA_PATH_BLE);

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_exit(VOID_T)
{
    if (!sg_attach_ota_success) {
        tuya_ble_attach_ota_port_end_notify(FALSE);
    }

    sg_attach_ota_state = TUYA_BLE_ATTACH_OTA_STATE_UNKNOWN;

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_rsp(tuya_ble_ota_response_t* rsp, VOID_T* rsp_data, UINT16_T data_size)
{
    if (rsp->type != TUYA_BLE_OTA_DATA) {
        TAL_PR_HEXDUMP_INFO("ota_rsp_data", rsp_data, data_size);
    }

    rsp->p_data = rsp_data;
    rsp->data_len = data_size;
    return tuya_ble_ota_response(rsp);
}

STATIC UINT32_T tuya_ble_attach_ota_req_handler(UINT8_T* cmd, UINT16_T cmd_size, tuya_ble_ota_response_t* rsp)
{
    if (sg_attach_ota_state != TUYA_BLE_ATTACH_OTA_STATE_UNKNOWN) {
        TAL_PR_ERR("%s ota_state error: %d", __FUNCTION__, sg_attach_ota_state);

        tuya_ble_attach_ota_req_rsp_t req_rsp = {0};
        req_rsp.flag = 1; //refuse ota
        tuya_ble_attach_ota_rsp(rsp, &req_rsp, SIZEOF(tuya_ble_attach_ota_req_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    sg_attach_ota_type = *cmd;
    if ((cmd_size != 1) || !(sg_attach_ota_type == 1 || (sg_attach_ota_type >= 10 && sg_attach_ota_type <= 19))) {
        TAL_PR_ERR("%s param error", __FUNCTION__);

        tuya_ble_attach_ota_req_rsp_t req_rsp = {0};
        req_rsp.flag = 1; //refuse ota
        tuya_ble_attach_ota_rsp(rsp, &req_rsp, SIZEOF(tuya_ble_attach_ota_req_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    if (sg_attach_ota_type == 1) {
        sg_attach_ota_idx = 0;
    } else {
        sg_attach_ota_idx = sg_attach_ota_type - 9;
    }

    tuya_ble_attach_ota_pri_param.p_attach_ota_idx = &sg_attach_ota_idx;

    tuya_ble_attach_ota_enter();

    tuya_ble_attach_ota_req_rsp_t req_rsp = {0};
    req_rsp.flag = 0; //accept ota
    req_rsp.ota_version = TUYA_BLE_ATTACH_OTA_VERSION;
    req_rsp.type = sg_attach_ota_type; //0-ble, 1-mcu
    req_rsp.version = tuya_ble_attach_ota_info[sg_attach_ota_idx].version;
    tal_util_reverse_byte((VOID_T*)&req_rsp.version, SIZEOF(UINT32_T));
    req_rsp.package_maxlen = TUYA_BLE_ATTACH_OTA_PKG_LEN;
    tal_util_reverse_byte((VOID_T*)&req_rsp.package_maxlen, SIZEOF(UINT16_T));
    tuya_ble_attach_ota_rsp(rsp, &req_rsp, SIZEOF(tuya_ble_attach_ota_req_rsp_t));

    sg_attach_ota_state = TUYA_BLE_OTA_FILE_INFO;

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_file_info_handler(UINT8_T* cmd, UINT16_T cmd_size, tuya_ble_ota_response_t* rsp)
{
    if (sg_attach_ota_state != TUYA_BLE_OTA_FILE_INFO) {
        TAL_PR_ERR("%s ota_state error: %d", __FUNCTION__, sg_attach_ota_state);
        //rsp none
        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tuya_ble_attach_ota_file_info_t* file_info = (VOID_T*)cmd;
    if (file_info->type != sg_attach_ota_type) {
        TAL_PR_ERR("%s file_info->type error", __FUNCTION__);
        //rsp none
        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tal_util_reverse_byte((VOID_T*)&file_info->version, SIZEOF(UINT32_T));
    tal_util_reverse_byte((VOID_T*)&file_info->file_len, SIZEOF(UINT32_T));
    tal_util_reverse_byte((VOID_T*)&file_info->crc32, SIZEOF(UINT32_T));
    sg_attach_firmware_len = file_info->file_len;
    sg_attach_firmware_crc32 = file_info->crc32;
    memcpy(sg_attach_firmware_md5, file_info->md5, TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN);
    tuya_ble_attach_ota_pri_param.p_firmware_crc32 = &sg_attach_firmware_crc32;
    tuya_ble_attach_ota_pri_param.p_firmware_md5 = sg_attach_firmware_md5;

    tuya_ble_attach_ota_file_info_rsp_t file_info_rsp = {0};
    file_info_rsp.type = sg_attach_ota_type; //firmware info
    if (file_info->version <= tuya_ble_attach_ota_info[sg_attach_ota_idx].version) {
        file_info_rsp.state = 2; //version error
    }
    else if (file_info->file_len > BOARD_FLASH_OTA_SIZE) {
        file_info_rsp.state = 3; //size error
    } else {
        file_info_rsp.state = 0;

        sg_attach_ota_state = TUYA_BLE_OTA_FILE_OFFSET_REQ;

        tuya_ble_attach_ota_pri_param.attach_ota_version = file_info->version;

        file_info_rsp.old_file_len = sg_attach_incomplete_firmware_info->len;
        tal_util_reverse_byte((VOID_T*)&file_info_rsp.old_file_len, SIZEOF(UINT32_T));
        file_info_rsp.old_crc32 = sg_attach_incomplete_firmware_info->crc32;
        tal_util_reverse_byte((VOID_T*)&file_info_rsp.old_crc32, SIZEOF(UINT32_T));
        memset(file_info_rsp.old_md5, 0, TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN);
    }

    tuya_ble_attach_ota_rsp(rsp, &file_info_rsp, SIZEOF(tuya_ble_attach_ota_file_info_rsp_t));

    if (file_info_rsp.state != 0) {
        TAL_PR_ERR("%s errorid: %d", __FUNCTION__, file_info_rsp.state);
        tuya_ble_attach_ota_exit();
    }

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_file_offset_handler(UINT8_T* cmd, UINT16_T cmd_size, tuya_ble_ota_response_t* rsp)
{
    if (sg_attach_ota_state != TUYA_BLE_OTA_FILE_OFFSET_REQ) {
        TAL_PR_ERR("%s ota_state error: %d", __FUNCTION__, sg_attach_ota_state);
        //rsp none
        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tuya_ble_attach_ota_file_offset_t* file_offset = (VOID_T*)cmd;
    if (file_offset->type != sg_attach_ota_type) {
        TAL_PR_ERR("%s file_offset->type error, __FUNCTION__");
        //rsp none
        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tal_util_reverse_byte((VOID_T*)&file_offset->offset, SIZEOF(UINT32_T));

    tuya_ble_attach_ota_file_offset_rsp_t file_offset_rsp = {0};
    file_offset_rsp.type = sg_attach_ota_type;

    if (file_offset->offset > 0) {
        if ((memcmp(sg_attach_incomplete_firmware_info->md5, sg_attach_firmware_md5, TUYA_BLE_ATTACH_OTA_FILE_MD5_LEN) == 0)
            && (file_offset->offset >= sg_attach_incomplete_firmware_info->len)) { //The ota data from phone may not have been written to Flash
            file_offset_rsp.offset = sg_attach_incomplete_firmware_info->len;
            sg_attach_data_offset = sg_attach_incomplete_firmware_info->len;
        } else {
            file_offset_rsp.offset = 0;
            sg_attach_data_offset = 0;
        }
    }

    tal_util_reverse_byte((VOID_T*)&file_offset_rsp.offset, SIZEOF(UINT32_T));
    tuya_ble_attach_ota_rsp(rsp, &file_offset_rsp, SIZEOF(tuya_ble_attach_ota_file_offset_rsp_t));

    sg_attach_ota_state = TUYA_BLE_OTA_DATA;

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_data_handler(UINT8_T* cmd, UINT16_T cmd_size, tuya_ble_ota_response_t* rsp)
{
    if (sg_attach_ota_state != TUYA_BLE_OTA_DATA) {
        TAL_PR_ERR("%s ota_state error: %d", __FUNCTION__, sg_attach_ota_state);

        tuya_ble_attach_ota_data_rsp_t ota_data_rsp = {0};
        ota_data_rsp.state = 4; //unknow error
        tuya_ble_attach_ota_rsp(rsp, &ota_data_rsp, SIZEOF(tuya_ble_attach_ota_data_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tuya_ble_attach_app_ota_data_t* ota_data = (VOID_T*)cmd;
    if (ota_data->type != sg_attach_ota_type) {
        TAL_PR_ERR("%s ota_data->type error", __FUNCTION__);

        tuya_ble_attach_ota_data_rsp_t ota_data_rsp = {0};
        ota_data_rsp.state = 4; //unknow error
        tuya_ble_attach_ota_rsp(rsp, &ota_data_rsp, SIZEOF(tuya_ble_attach_ota_data_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tal_util_reverse_byte((VOID_T*)&ota_data->pkg_id, SIZEOF(UINT16_T));
    tal_util_reverse_byte((VOID_T*)&ota_data->len, SIZEOF(UINT16_T));
    tal_util_reverse_byte((VOID_T*)&ota_data->crc16, SIZEOF(UINT16_T));

    tuya_ble_attach_ota_data_rsp_t ota_data_rsp = {0};
    ota_data_rsp.type = sg_attach_ota_type;
    if (sg_attach_data_package_id+1 != ota_data->pkg_id) {
        ota_data_rsp.state = 1; //package id error
    }
    else if (cmd_size-7 != ota_data->len) {
        ota_data_rsp.state = 2; //size error
    }
    else if (tal_util_crc16(ota_data->data, ota_data->len, NULL) != ota_data->crc16) {
        ota_data_rsp.state = 3; //crc error
    } else {
        ota_data_rsp.state = 0;

        TUYA_OTA_DATA_T tal_ota_data = {0};
        tal_ota_data.total_len = sg_attach_firmware_len;
        tal_ota_data.offset = sg_attach_data_offset;

        tal_ota_data.data = tal_malloc(ota_data->len);
        if (tal_ota_data.data) {
            tal_ota_data.len = ota_data->len;
            memcpy(tal_ota_data.data, ota_data->data, ota_data->len);

            tal_ota_data.pri_data = &tuya_ble_attach_ota_pri_param;

            if (tuya_ble_attach_ota_port_data_process(&tal_ota_data, NULL) != OPRT_OK) {
                ota_data_rsp.state = 4; //unknow error
            }

            tal_free(tal_ota_data.data);
        } else {
            TAL_PR_ERR("tal_malloc failed");
            ota_data_rsp.state = 4; //unknow error
        }

        sg_attach_data_offset += ota_data->len;

        if (sg_attach_data_offset < sg_attach_firmware_len) {
            sg_attach_ota_state = TUYA_BLE_OTA_DATA;
        } else if (sg_attach_data_offset == sg_attach_firmware_len) {
            sg_attach_ota_state = TUYA_BLE_OTA_END;
        }

        sg_attach_data_package_id++;
        TAL_PR_INFO("sg_attach_data_package_id: %d", sg_attach_data_package_id);
    }

    tuya_ble_attach_ota_rsp(rsp, &ota_data_rsp, SIZEOF(tuya_ble_attach_ota_data_rsp_t));

    if (ota_data_rsp.state != 0) {
        TAL_PR_ERR("%s errorid: %d", __FUNCTION__, ota_data_rsp.state);
        tuya_ble_attach_ota_exit();
    }

    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_attach_ota_end_handler(UINT8_T* cmd, UINT16_T cmd_size, tuya_ble_ota_response_t* rsp)
{
    if (sg_attach_ota_state != TUYA_BLE_OTA_END) {
        TAL_PR_ERR("%s ota_state error: %d", __FUNCTION__, sg_attach_ota_state);

        tuya_ble_attach_ota_end_rsp_t end_rsp = {0};
        end_rsp.state = 3; //unknow error
        tuya_ble_attach_ota_rsp(rsp, &end_rsp, SIZEOF(tuya_ble_attach_ota_end_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    if ((cmd_size != 1) || (*cmd != sg_attach_ota_type)) {
        TAL_PR_ERR("%s type error", __FUNCTION__);

        tuya_ble_attach_ota_end_rsp_t end_rsp = {0};
        end_rsp.state = 3; //unknow error
        tuya_ble_attach_ota_rsp(rsp, &end_rsp, SIZEOF(tuya_ble_attach_ota_end_rsp_t));

        tuya_ble_attach_ota_exit();
        return OPRT_COM_ERROR;
    }

    tuya_ble_attach_ota_end_rsp_t end_rsp = {0};
    end_rsp.type = sg_attach_ota_type;

    UINT32_T ret = tuya_ble_attach_ota_port_end_notify(TRUE);
    if (ret == 1) {
        end_rsp.state = 1; //total size error
    } else if (ret == 2) {
        end_rsp.state = 2; //crc error
    } else {
        end_rsp.state = 0;

        sg_attach_ota_success = TRUE;

        tuya_ble_attach_ota_exit();

        tuya_ble_attach_ota_port_get_old_firmware_info(&sg_attach_incomplete_firmware_info);

        TAL_PR_INFO("ota success");
    }

    tuya_ble_attach_ota_rsp(rsp, &end_rsp, SIZEOF(tuya_ble_attach_ota_end_rsp_t));

    if (end_rsp.state != 0) {
        TAL_PR_ERR("%s errorid: %d", __FUNCTION__, end_rsp.state);
        tuya_ble_attach_ota_exit();
    }

    return OPRT_OK;
}

UINT32_T tuya_ble_attach_ota_init(VOID_T)
{
    tuya_ble_attach_ota_port_get_old_firmware_info(&sg_attach_incomplete_firmware_info);

//    TAL_PR_INFO("sg_attach_incomplete_firmware_info->len: %d", sg_attach_incomplete_firmware_info->len);
//    TAL_PR_INFO("sg_attach_incomplete_firmware_info->crc32: %x", sg_attach_incomplete_firmware_info->crc32);

    return OPRT_OK;
}

UINT32_T tuya_ble_attach_ota_handler(tuya_ble_ota_data_t* ota)
{
    tuya_ble_ota_response_t rsp;
    rsp.type = ota->type;

    if (ota->type != TUYA_BLE_OTA_DATA) {
        TAL_PR_INFO("ota_cmd_type: %d", ota->type);
        TAL_PR_HEXDUMP_INFO("ota_cmd_data", ota->p_data, ota->data_len);
    }

    switch (ota->type) {
        case TUYA_BLE_OTA_REQ: {
            tuya_ble_attach_ota_req_handler(ota->p_data, ota->data_len, &rsp);
        } break;

        case TUYA_BLE_OTA_FILE_INFO: {
            tuya_ble_attach_ota_file_info_handler(ota->p_data, ota->data_len, &rsp);
        } break;

        case TUYA_BLE_OTA_FILE_OFFSET_REQ: {
            tuya_ble_attach_ota_file_offset_handler(ota->p_data, ota->data_len, &rsp);
        } break;

        case TUYA_BLE_OTA_DATA: {
            tuya_ble_attach_ota_data_handler(ota->p_data, ota->data_len, &rsp);
        } break;

        case TUYA_BLE_OTA_END: {
            tuya_ble_attach_ota_end_handler(ota->p_data, ota->data_len, &rsp);
        } break;

        default: {
        } break;
    }
    return OPRT_OK;
}

UINT32_T tuya_ble_attach_ota_disconn_handler(VOID_T)
{
    if (sg_attach_ota_state >= TUYA_BLE_OTA_REQ) {
        return tuya_ble_attach_ota_exit();
    }
    return 1;
}

VOID_T tuya_ble_attach_ota_set_status(INT32_T status)
{
    sg_attach_ota_state = status;
}

INT32_T tuya_ble_attach_ota_get_status(VOID_T)
{
    return sg_attach_ota_state;
}

#endif // TUYA_BLE_FEATURE_ATTACH_OTA_ENABLE

