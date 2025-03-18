/***********************************************************************
 ** INCLUDE                                                           **
 **********************************************************************/
#include "board.h"
#include "string.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_api.h"
#include "tuya_ble_ota.h"
#include "tuya_ble_bulkdata_demo.h"
#include "tuya_ble_protocol_callback.h"
#include "tuya_ble_main.h"
#include "tuya_ble_mem.h"

#include "tal_flash.h"
#include "tal_memory.h"
#include "tal_log.h"
#include "tal_rtc.h"
#include "tal_utc.h"
#include "tal_bluetooth.h"
#include "tal_util.h"

#if defined(TUYA_BLE_FILE_ENABLE) && (TUYA_BLE_FILE_ENABLE==1)
#include "app_ble_file_demo.h"
#include "tuya_ble_protocol_callback.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/
#define STORAGE_IN_MUC 0
#define STORAGE_IN_EXT 1
#ifndef FILE_STORAGE_AREA
#define FILE_STORAGE_AREA STORAGE_IN_EXT
#endif
#define TUYA_BLE_FILE_INFO_ADDR             (BOARD_FLASH_FILE_INFO_ADDR)                            //存储当前下发文件信息
#define TUYA_BLE_FILE_MD5_INFO_ADDR         (BOARD_FLASH_FILE_MD5_INFO_ADDR)                        //存储当前下发文件MD5信息及当前使用文件ID
#define TUYA_BLE_FILE_INFO_BACKUP_ADDR      (BOARD_FLASH_FILE_INFO_BACKUP_ADDR)                     //存储已下发文件所有条目信息，采用队列进行管理

#define TUYA_BLE_FILE_START_ADDR            (BOARD_FLASH_FILE_START_ADDR)                           //文件存储起始地址
#define TUYA_BLE_FILE_END_ADDR              (BOARD_FLASH_FILE_END_ADDR)                             //文件存储结束地址
#define TUYA_BLE_FILE_RECORD_SIZE           (TUYA_BLE_FILE_END_ADDR - TUYA_BLE_FILE_START_ADDR)     //存储文件区域

#define TUYA_BLE_FILE_FILE_ADDR             (TUYA_BLE_FILE_START_ADDR)                                               //记录存储当前文件地址
#define TUYA_BLE_FILE_MAX_LEN               (2 * 1024 * 1024)                                           //单个文件最大长度，设置为2M字节.

/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/

/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/
STATIC TAL_BLE_FILE_DATA_STEP_E file_step = FILE_STEP_NONE;
STATIC TAL_BLE_FILE_VERSION_FLAG_E file_id_ver_flag = FILE_VERSION_INIT; //file version flag
STATIC TAL_BLE_FILE_INFO_T file_info;
STATIC TAL_BLE_FILE_INFO_DATA_T file_stored_info;
STATIC TUYA_BLE_HISTORY_FILE_T history_file_data;      // file history entry data
STATIC UINT8_T file_continue_flag = 0;                 //file version flag

STATIC TAL_BLE_FILE_MD5_INFO_T file_stored_md5_info;
STATIC TAL_BLE_FILE_MD5_INFO_T file_md5_info_storage;
STATIC mbedtls_md5_context ctx_remoffset;

/***********************************************************************
 ** FUNCTON                                                           **
 **********************************************************************/
STATIC VOID_T report_dp_voice_file_stu(UINT16_T file_id, UINT32_T file_ver, UINT8_T status) {
    extern UINT32_T g_sn;
    #define TUYA_DP_STATUS_VOICE_FILE 30

    UINT8_T dp_data[30];
    UINT8_T index = 0;
    dp_data[index++] = TUYA_DP_STATUS_VOICE_FILE;
    dp_data[index++] = DT_RAW;
    dp_data[index++] = 0;
    dp_data[index++] = 7;
    dp_data[index++] = (UINT8_T)((file_id>>8) & 0xff);
    dp_data[index++] = (UINT8_T)(file_id & 0xff);
    dp_data[index++] = (UINT8_T)((file_ver>>24) & 0xff);
    dp_data[index++] = (UINT8_T)((file_ver>>16) & 0xff);
    dp_data[index++] = (UINT8_T)((file_ver>>8) & 0xff);
    dp_data[index++] = (UINT8_T)(file_ver & 0xff);
    dp_data[index++] = (UINT8_T)(status);
    
    tuya_ble_dp_data_send(g_sn++, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITH_RESPONSE, dp_data, index);
}

STATIC OPERATE_RET app_ble_file_write(UINT32_T addr, CONST UCHAR_T *src, UINT32_T size)
{
#if FILE_STORAGE_AREA == STORAGE_IN_MUC
    return tal_flash_write(addr, src, size);
#else
    return OPRT_OK;
#endif
}

STATIC OPERATE_RET app_ble_file_read(UINT32_T addr, UCHAR_T *dst, UINT32_T size)
{
#if FILE_STORAGE_AREA == STORAGE_IN_MUC
    return tal_flash_read(addr, dst, size);
#else
    return OPRT_OK;
#endif
}

STATIC OPERATE_RET app_ble_file_erase(UINT32_T addr, UINT32_T size)
{
#if FILE_STORAGE_AREA == STORAGE_IN_MUC
    return tal_flash_erase(addr, size);
#else
    return OPRT_OK;
#endif
}

BOOL_T tuya_ble_md5_crypt_loop(mbedtls_md5_context *ctx, ENUM_MD5_CRYPE_LOOP_STEP step, UINT8_T *input, UINT16_T input_len, UINT8_T *output)
{
    INT32_T ret;

    if (MD5_CRYPT_LOOP_STEP_INIT == step) {
        mbedtls_md5_init( ctx );
        if (( ret = mbedtls_md5_starts_ret( ctx ) ) != 0) {
            mbedtls_md5_free( ctx );
        }
    } else if (MD5_CRYPT_LOOP_STEP_UPDATE == step) {
        if(( ret = mbedtls_md5_update_ret( ctx, input, input_len ) ) != 0 ) {
            mbedtls_md5_free( ctx );
        }
    } else if (MD5_CRYPT_LOOP_STEP_FINISH == step) {
        if( ( ret = mbedtls_md5_finish_ret( ctx, output ) ) != 0 ) {
            mbedtls_md5_free( ctx );
        }
    } else {
        mbedtls_md5_free( ctx );
    }

    return ret;
}

STATIC OPERATE_RET tuya_ble_history_file_get_data(UINT32_T addr, TUYA_BLE_HISTORY_FILE_T *pdata )
{
    if (pdata == NULL) {
        return OPRT_INVALID_PARM;
    }

    app_ble_file_read(addr, (UINT8_T *)pdata, SIZEOF(TUYA_BLE_HISTORY_FILE_T));
    
    return OPRT_OK;
}

STATIC UINT32_T tuya_ble_history_file_get_ver_by_fid(UINT16_T rfitem)
{
    UINT32_T file_version;

    file_version = history_file_data.file_info_data[rfitem].file_ver;
    
    TAL_PR_DEBUG("file_version: %d", file_version);
    return file_version;
}

STATIC OPERATE_RET tuya_ble_history_file_set_data( TUYA_BLE_HISTORY_FILE_T *pdata )
{
    if (pdata == NULL) {
        return OPRT_INVALID_PARM;
    }
        
    if (app_ble_file_erase(TUYA_BLE_FILE_INFO_BACKUP_ADDR, TUYA_NV_ERASE_MIN_SIZE) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }
     
    if (app_ble_file_write(TUYA_BLE_FILE_INFO_BACKUP_ADDR, (UINT8_T*)pdata, SIZEOF(TUYA_BLE_HISTORY_FILE_T)) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

STATIC VOID_T tuya_ble_history_file_data_init(VOID_T)
{
    memset(&history_file_data, 0, SIZEOF(TUYA_BLE_HISTORY_FILE_T));

    tuya_ble_history_file_get_data(TUYA_BLE_FILE_INFO_BACKUP_ADDR, &history_file_data);
    
    if(history_file_data.storage_flag != TUYA_BLE_FILE_STORAGE_HEAD_FLAG) {
        memset(&history_file_data, 0, SIZEOF(TUYA_BLE_HISTORY_FILE_T));
        history_file_data.storage_flag = TUYA_BLE_FILE_STORAGE_HEAD_FLAG ;
        tuya_ble_history_file_set_data(&history_file_data);
    }
}

STATIC OPERATE_RET tuya_ble_history_file_del_record(UINT32_T index)
{
    INT32_T i;

    if(index > history_file_data.write_index) {
        return OPRT_INVALID_PARM;
    }

    for(i = index; i < TUYA_BLE_HISTOR_MAX_NUM - 1; i++) {
        memcpy((UINT8_T*)&(history_file_data.file_info_data[i]), (UINT8_T*)&(history_file_data.file_info_data[i+1]), SIZEOF(TAL_BLE_FILE_INFO_DATA_T));
    }
    
    history_file_data.write_index--;
    history_file_data.data_nums--;
    TAL_PR_DEBUG("history_file_data.write_index:0x%x,history_file_data.data_nums:0x%x", history_file_data.write_index,history_file_data.data_nums);
    
    tuya_ble_history_file_set_data(&history_file_data);
    
    return OPRT_OK;
}

STATIC VOID_T tuya_ble_history_file_add_record(TAL_BLE_FILE_INFO_DATA_T *pfile_data)
{
    if(history_file_data.write_index == TUYA_BLE_HISTOR_MAX_NUM) {
        tuya_ble_history_file_del_record(0);
    }

    history_file_data.file_info_data[history_file_data.write_index].id_buf = pfile_data->id_buf;

    memcpy((UINT8_T*)&(history_file_data.file_info_data[history_file_data.write_index]), (UINT8_T*)pfile_data, SIZEOF(TAL_BLE_FILE_INFO_DATA_T));

    if(history_file_data.data_nums < TUYA_BLE_HISTOR_MAX_NUM) {
        history_file_data.data_nums++;
        history_file_data.write_index++;
    }
    
    tuya_ble_history_file_set_data(&history_file_data);
}

STATIC OPERATE_RET tuya_ble_history_file_search_by_fid(UINT16_T file_fid, UINT16_T *rfitem)
{
    UINT16_T i,temp_fid;
    
    i = history_file_data.data_nums;
    
    for( ; i > 0; i--) {
        temp_fid = history_file_data.file_info_data[i-1].file_id;

        if(temp_fid == file_fid) {
            *rfitem = i - 1;//item_num.
            return OPRT_NOT_SUPPORTED;         
        }
    }
    
    return OPRT_OK;
}

STATIC OPERATE_RET tuya_ble_file_settings_save(TAL_BLE_FILE_INFO_DATA_T *info)
{
    TAL_BLE_FILE_INFO_DATA_T stored_info;
    TAL_BLE_FILE_MD5_INFO_T file_md5_info_storage;
    
    app_ble_file_read(TUYA_BLE_FILE_MD5_INFO_ADDR, (UINT8_T*)&file_md5_info_storage, SIZEOF(TAL_BLE_FILE_MD5_INFO_T));
    file_stored_md5_info.cur_file_id = file_md5_info_storage.cur_file_id;

    if(app_ble_file_erase(TUYA_BLE_FILE_INFO_ADDR, TUYA_NV_ERASE_MIN_SIZE) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }

    stored_info.file_addr = TUYA_BLE_FILE_START_ADDR;
    stored_info.type = info->type;
    stored_info.file_id = info->file_id;
    stored_info.file_ver = info->file_ver;
    stored_info.file_len = info->file_len;
    memcpy(stored_info.file_md5, info->file_md5, SIZEOF(info->file_md5));
    stored_info.id_len = info->id_len;
    
    if(app_ble_file_write(TUYA_BLE_FILE_INFO_ADDR, (UINT8_T*)&stored_info, SIZEOF(TAL_BLE_FILE_INFO_DATA_T)) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }
    
    TAL_PR_DEBUG("Asizeof(TAL_BLE_FILE_MD5_INFO_T):%d",SIZEOF(TAL_BLE_FILE_MD5_INFO_T));
    if(app_ble_file_write(TUYA_BLE_FILE_MD5_INFO_ADDR, (UINT8_T*)&file_stored_md5_info, SIZEOF(TAL_BLE_FILE_MD5_INFO_T) + TUYA_BLE_ALIGN_NUM) != TUYA_BLE_SUCCESS) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

STATIC UINT8_T tuya_ble_file_settings_load(TAL_BLE_FILE_INFO_DATA_T *info)
{
    OPERATE_RET ret = OPRT_OK;
    TAL_BLE_FILE_INFO_DATA_T stored_info;

    app_ble_file_read(TUYA_BLE_FILE_INFO_ADDR, (UINT8_T*)&stored_info.file_addr, SIZEOF(stored_info.file_addr));
    if(stored_info.file_addr == TUYA_BLE_FILE_FILE_ADDR) {
        ret = OPRT_OK;
    } else{
        ret = OPRT_NOT_FOUND;
    }

    if(ret == OPRT_OK) {
        app_ble_file_read(TUYA_BLE_FILE_INFO_ADDR, (UINT8_T*)&stored_info, SIZEOF(TAL_BLE_FILE_INFO_DATA_T));
        info->file_addr = stored_info.file_addr; 
        info->type = stored_info.type;
        info->file_id = stored_info.file_id;
        info->file_ver = stored_info.file_ver;
        info->file_len = stored_info.file_len;
        memcpy(info->file_md5, stored_info.file_md5, SIZEOF(stored_info.file_md5));
        info->id_len = stored_info.id_len;
    }

    app_ble_file_read(TUYA_BLE_FILE_MD5_INFO_ADDR, (UINT8_T*)&file_stored_md5_info.ctx_storage, SIZEOF(file_stored_md5_info.ctx_storage));

    return ret;
}

STATIC VOID_T tuya_ble_file_enter(VOID_T)
{
    tuya_ble_update_conn_param_timer_stop();
    if(tuya_ble_connect_status_get() == BONDING_CONN) {
        TAL_BLE_PEER_INFO_T peer_info = {0};
        peer_info.conn_handle = tuya_app_get_conn_handle();
        TAL_BLE_CONN_PARAMS_T conn_param = {0};
        conn_param.min_conn_interval = 15 * 4 / 5;
        conn_param.max_conn_interval = 30 * 4 / 5;
        conn_param.latency = 0;
        conn_param.conn_sup_timeout = 6000 / 10;
        conn_param.connection_timeout = 0;
        tal_ble_conn_param_update(peer_info, &conn_param);
        TAL_PR_DEBUG("conn_param");
    }
}

STATIC VOID_T tuya_ble_file_exit(VOID_T)
{
    file_step = FILE_STEP_NONE;
    tuya_ble_update_conn_param_timer_start();
}

STATIC UINT32_T tuya_ble_file_rsp(tuya_ble_file_response_t *rsp, VOID_T *rsp_data, UINT16_T data_size)
{
    rsp->p_data = rsp_data;
    rsp->data_len = data_size;
    return tuya_ble_file_response(rsp);
}

STATIC UINT16_T tuya_ble_file_rsp_pack_info(TAL_BLE_FILE_INFO_RSP_T rsp_s, UINT8_T *nrsp_buf)
{
    UINT16_T i = 0,j = 0;

    nrsp_buf[i++] = rsp_s.type;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id&0x00ff);
    nrsp_buf[i++] = rsp_s.status;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.pkt_maxlen>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.pkt_maxlen&0xff);
    nrsp_buf[i++] = (UINT8_T)(rsp_s.stored_len>>24)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.stored_len>>16)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.stored_len>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.stored_len)&0xff;
    for(j=0; j<TUYA_BLE_FILE_MD5_LEN; j++) {
        nrsp_buf[i++] = rsp_s.stored_md5[j];
    }
    
    return i;
}

STATIC UINT16_T tuya_ble_file_rsp_pack_offset(TAL_BLE_FILE_OFFSET_REP_T rsp_s, UINT8_T *nrsp_buf)
{
    UINT16_T i = 0;

    nrsp_buf[i++] = rsp_s.type;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id&0x00ff);
    nrsp_buf[i++] = (UINT8_T)(rsp_s.offset>>24)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.offset>>16)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.offset>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.offset)&0xff;

    return i;
}

STATIC UINT16_T tuya_ble_file_rsp_pack_data(TAL_BLE_FILE_DATA_RSP_T rsp_s, UINT8_T *nrsp_buf)
{
    UINT16_T i = 0;

    nrsp_buf[i++] = rsp_s.type;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id&0x00ff);
    nrsp_buf[i++] = rsp_s.status;

    return i;
}

STATIC UINT16_T tuya_ble_file_rsp_pack_end(TAL_BLE_FILE_END_RSP_T rsp_s, UINT8_T *nrsp_buf)
{
    UINT16_T i = 0;

    nrsp_buf[i++] = rsp_s.type;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id>>8)&0xff;
    nrsp_buf[i++] = (UINT8_T)(rsp_s.file_id&0x00ff);
    nrsp_buf[i++] = rsp_s.status;

    return i;
}

STATIC OPERATE_RET tuya_ble_file_info_handler(UINT8_T *buf, UINT16_T len, tuya_ble_file_response_t *rsp)
{
    UINT16_T readfileitem = 0;
    TAL_BLE_FILE_INFO_T file_info_req;
    TAL_BLE_FILE_INFO_RSP_T file_info_rsp;
    UINT8_T nrsp_buf[TUYA_BLE_FILE_RSP_MAX_LEN] = {0};
    UINT16_T i = 0,j = 0;
    
    memset(&file_info_rsp, 0x00, SIZEOF(TAL_BLE_FILE_INFO_RSP_T));
    file_info_rsp.status = 0x00;
    
    if(file_step != FILE_STEP_NONE) {
        file_info_rsp.status = FILE_RSP_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_INFO - error: %d", file_info_rsp.status);
        i = tuya_ble_file_rsp_pack_info(file_info_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_RESOURCE_NOT_READY;
    }

    file_step = FILE_STEP_INFO;
    
    // receive data parse
    i = 0;
    file_info_req.type = buf[i++];
    file_info_req.file_id = (buf[i]<<8) + buf[i+1];
    i += 2;
    TAL_PR_DEBUG("file_info_req.file_id:0x%02x,i=%d", file_info_req.file_id,i);
    file_info_req.id_len = buf[i++];
    
    file_info_req.id_buf = tuya_ble_malloc(file_info_req.id_len);
    if(file_info_req.id_buf == NULL) {
        TAL_PR_ERR("Malloc False");
        file_info_rsp.status = FILE_RSP_ERR_UNKNOWN;
        i = tuya_ble_file_rsp_pack_info(file_info_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_MALLOC_FAILED;
    }

    for(j = 0; j < file_info_req.id_len; j++) {
        file_info_req.id_buf[j] = buf[i++];
    }
    // TAL_PR_HEXDUMP_DEBUG(" file_info_req.id_buf",(UINT8_T*) file_info_req.id_buf,file_info_req.id_len);
    file_info_req.file_ver = (buf[i]<<24) + (buf[i+1]<<16) + (buf[i+2]<<8) + buf[i+3];
    i += 4;
    file_info_req.file_len = (buf[i]<<24) + (buf[i+1]<<16) + (buf[i+2]<<8) + buf[i+3];
    i += 4;
    TAL_PR_DEBUG("file_ver: 0x%02x - file_len: 0x%02x, i=%d", file_info_req.file_ver, file_info_req.file_len,i);
    
    for(j = 0; j < TUYA_BLE_FILE_MD5_LEN; j++) {
        file_info_req.file_md5[j] = buf[i++];
    }
    // TAL_PR_HEXDUMP_DEBUG(" file_info_req.file_md5",(UINT8_T*) file_info_req.file_md5,TUYA_BLE_FILE_MD5_LEN);

    // push file infomation data
    memset(&file_info, 0x00, SIZEOF(TAL_BLE_FILE_INFO_T));
    file_info.type     = file_info_req.type;
    file_info.file_id  = file_info_req.file_id;
    file_info.id_len   = file_info_req.id_len;
    file_info.file_ver = file_info_req.file_ver;
    file_info.file_len = file_info_req.file_len;
    memcpy(file_info.file_md5, file_info_req.file_md5, SIZEOF(file_info_req.file_md5));

    // response data prepare
    file_info_rsp.type       = file_info.type;
    file_info_rsp.file_id    = file_info.file_id;
    file_info_rsp.pkt_maxlen = TUYA_BLE_FILE_PKT_MAX_LEN;
    if(tuya_ble_history_file_search_by_fid(file_info.file_id,&readfileitem) == OPRT_OK) {
        file_id_ver_flag = FILE_VERSION_INIT;
    } else {
        // The file exists, but the version is lower than or equal to the stored version.
        TAL_PR_ERR("file_info.file_ver: %d", file_info.file_ver);
        TAL_PR_ERR("readfileitem: %d", readfileitem);
    
        if(file_info.file_ver <= tuya_ble_history_file_get_ver_by_fid(readfileitem)) {
            file_id_ver_flag     = FILE_VERSION_INIT;
            file_info_rsp.status = FILE_RSP_ERR_VERSION;
            TAL_PR_ERR("FILE_STEP_INFO - error: %d", file_info_rsp.status);
            i = tuya_ble_file_rsp_pack_info(file_info_rsp, nrsp_buf);
            tuya_ble_file_rsp(rsp, &nrsp_buf, i);
            tuya_ble_file_exit();
            tuya_ble_free(file_info_req.id_buf);
            return OPRT_NOT_SUPPORTED;
        } else {
            // high version mark
            file_id_ver_flag = FILE_VERSION_HIGH;
        }
    }  

    //file size exceeds limit - the debugging file size is temporarily set to 2M.
    if(file_info_req.file_len >= TUYA_BLE_FILE_MAX_LEN) {
        file_info_rsp.status = FILE_RSP_ERR_SIZE;
        TAL_PR_ERR("FILE_STEP_INFO - error: %d", file_info_rsp.status);
        i = tuya_ble_file_rsp_pack_info(file_info_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        tuya_ble_free(file_info_req.id_buf);
        return OPRT_EXCEED_UPPER_LIMIT;
    }

    memset(&file_stored_info, 0x00, SIZEOF(TAL_BLE_FILE_INFO_DATA_T));
    if(tuya_ble_file_settings_load(&file_stored_info) != OPRT_OK) {
        file_stored_info.file_len  = 0;
        file_stored_info.file_addr = TUYA_BLE_FILE_START_ADDR;
    }

    file_stored_info.type     = file_info.type;
    file_stored_info.file_ver = file_info.file_ver;
    file_stored_info.id_len   = file_info.id_len;
    file_stored_info.id_buf   = file_info_req.id_buf;
    // TAL_PR_HEXDUMP_DEBUG(" file_stored_info.id_buf",(UINT8_T*) file_stored_info.id_buf,file_stored_info.id_len);
    TAL_PR_DEBUG("file_stored_info.file_id:0x%02x - file_info.file_id:0x%02x", file_stored_info.file_id, file_info.file_id);
    if(file_stored_info.file_id == file_info.file_id) {
        file_info_rsp.stored_len = file_stored_info.file_len;
        file_info.pkt_id         = file_info_rsp.stored_len/TUYA_BLE_FILE_PKT_MAX_LEN;
        file_continue_flag       = 1;
        memcpy(&ctx_remoffset,&file_stored_md5_info.ctx_storage,  SIZEOF(ctx_remoffset));//power off read  the latest stored value.
        // TAL_PR_HEXDUMP_DEBUG("ctx_remoffset data4",(UINT8_T*)&ctx_remoffset,SIZEOF(mbedtls_md5_context));
        // TAL_PR_HEXDUMP_DEBUG("file_info.file_md5 data",(UINT8_T*)file_stored_info.file_md5,16);

    } else {
        file_info_rsp.stored_len = 0;
    }
    TAL_PR_DEBUG("file_info_rsp.stored_len: 0x%02x", file_info_rsp.stored_len);

    memcpy(file_info_rsp.stored_md5, file_stored_info.file_md5, SIZEOF(file_stored_info.file_md5));

    // TAL_PR_HEXDUMP_DEBUG("file_info_rsp",(UINT8_T*)&file_info_rsp, SIZEOF(TAL_BLE_FILE_INFO_RSP_T));
    i = tuya_ble_file_rsp_pack_info(file_info_rsp, nrsp_buf);
    tuya_ble_file_rsp(rsp, &nrsp_buf, i);
    
    tuya_ble_free(file_info_req.id_buf);
    tuya_ble_file_enter();
    file_step = FILE_STEP_OFFSET;

    return OPRT_OK;
}

STATIC OPERATE_RET tuya_ble_file_offset_handler(UINT8_T *buf, UINT16_T len, tuya_ble_file_response_t *rsp)
{
    UINT8_T nrsp_buf[TUYA_BLE_FILE_RSP_MAX_LEN]={0};
    UINT16_T i = 0;
    
    if(file_step != FILE_STEP_OFFSET) {
        TAL_PR_ERR("FILE_STEP_OFFSET - file_step error");
        tuya_ble_file_exit();
        return OPRT_RESOURCE_NOT_READY;
    }

    TAL_BLE_FILE_OFFSET_REQ_T file_offset_req;
    i = 0;
    file_offset_req.type = buf[i++];
    file_offset_req.file_id = (buf[i]<<8) + buf[i+1];
    i += 2;
    file_offset_req.offset = (buf[i]<<24) + (buf[i+1]<<16) + (buf[i+2]<<8) + buf[i+3];
    
    if(file_offset_req.type != file_info.type) {
        TAL_PR_ERR("FILE_STEP_OFFSET - file_offset_req.type error");
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }

    if(file_offset_req.file_id != file_info.file_id) {
        TAL_PR_ERR("FILE_STEP_OFFSET - file_offset_req.file_id error");
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }
    
    /* response */
    TAL_BLE_FILE_OFFSET_REP_T file_offset_rsp;
    memset(&file_offset_rsp, 0x00, SIZEOF(TAL_BLE_FILE_OFFSET_REP_T));

    file_offset_rsp.type = file_info.type;
    file_offset_rsp.file_id = file_info.file_id;

    TAL_PR_DEBUG("file_offset_req.file_id:0x%02x - file_stored_info.file_id:0x%02x", file_offset_req.file_id,file_stored_info.file_id);  
    TAL_PR_DEBUG("file_offset_req.offset:0x%02x - file_stored_info.file_len:0x%02x", file_offset_req.offset,file_stored_info.file_len);  
    
     if((file_offset_req.file_id == file_stored_info.file_id)) {
        file_offset_rsp.offset = file_stored_info.file_len;
        file_info.data_len = file_stored_info.file_len;
    } else {
        TAL_PR_ERR("file_stored_info.file_len error. id.len.md5");
        file_offset_rsp.offset = 0;
        file_info.data_len = 0;
    }
    
    file_stored_info.file_id = file_offset_rsp.file_id;

    i = tuya_ble_file_rsp_pack_offset(file_offset_rsp, nrsp_buf);
    tuya_ble_file_rsp(rsp, &nrsp_buf, i);

    file_step = FILE_STEP_DATA;

    return OPRT_OK;
}

STATIC OPERATE_RET tuya_ble_file_data_handler(UINT8_T *buf, UINT16_T len, tuya_ble_file_response_t *rsp)
{
    UINT8_T nrsp_buf[TUYA_BLE_FILE_RSP_MAX_LEN]={0};
    TAL_BLE_FILE_DATA_RSP_T file_data_rsp;
    UINT16_T i = 0;

    memset(&file_data_rsp, 0x00, SIZEOF(TAL_BLE_FILE_DATA_RSP_T));
    file_data_rsp.status = 0x00;

    if(file_step != FILE_STEP_DATA) {
        file_data_rsp.status = FILE_RSP_DATA_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);

        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        
        tuya_ble_file_exit();
        return OPRT_RESOURCE_NOT_READY;
    }
    
    TAL_BLE_FILE_DATA_REQ_T file_data_req;
    
    i = 0;
    file_data_req.type = buf[i++];
    file_data_req.file_id = (buf[i]<<8) + buf[i+1];
    i += 2;
    file_data_req.pkt_id = (buf[i]<<8) + buf[i+1];
    i += 2;
    file_data_req.pkt_len = (buf[i]<<8) + buf[i+1];
    i += 2;
    file_data_req.pkt_crc16 = (buf[i]<<8) + buf[i+1];
    i += 2;
    UINT8_T* pfile_data_req_data = (VOID_T*)(buf+i);
    
    TAL_PR_DEBUG("file_data_req.pkt_len: :0x%02x", file_data_req.pkt_len);

    if(file_data_req.type != file_info.type) {
        file_data_rsp.status = FILE_RSP_DATA_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_TYPE- error: %d", file_data_rsp.status);
        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }
    if(file_data_req.file_id != file_info.file_id) {
        file_data_rsp.status = FILE_RSP_DATA_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_ID- error: %d", file_data_rsp.status);
        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }

    // response
    file_data_rsp.type = file_info.type;
    file_data_rsp.file_id = file_info.file_id;
 
    if(file_data_req.pkt_id != file_info.pkt_id) {
        file_data_rsp.status = FILE_RSP_DATA_ERR_PKT; //packet id error
        TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);
        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_COM_ERROR;
    }

    if(file_data_req.pkt_len != (len - 9)) {
        //packet len error
        file_data_rsp.status = FILE_RSP_DATA_ERR_LENRTH;
        TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);
        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_COM_ERROR;
    }

    if(tal_util_crc16(pfile_data_req_data, file_data_req.pkt_len, NULL) != file_data_req.pkt_crc16) {
        //packet crc16 error
        file_data_rsp.status = FILE_RSP_DATA_ERR_CRC;
        TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);
        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_CRC32_FAILED;
    }

    //erase
    UINT8_T flag_4k = 0;
    UINT8_T flag_4byte = 0;// flash writes the last pack of four bytes aligned.
    UINT8_T md5[TUYA_BLE_FILE_MD5_LEN];

    if((file_info.data_len < file_info.file_len)&&((file_info.data_len == 0) ||(file_continue_flag == 1)|| ((file_info.data_len + file_data_req.pkt_len) >= (((file_info.data_len / TUYA_NV_ERASE_MIN_SIZE) + 1) * TUYA_NV_ERASE_MIN_SIZE))))
    {
        UINT32_T erase_addr = TUYA_BLE_FILE_START_ADDR;
        
        if(file_info.data_len != 0) {
            
            erase_addr += (((file_info.data_len / TUYA_NV_ERASE_MIN_SIZE) + 1) * TUYA_NV_ERASE_MIN_SIZE) % TUYA_BLE_FILE_RECORD_SIZE;

            if(file_continue_flag == 1) {
                file_continue_flag = 0;
                erase_addr += (((file_info.data_len / TUYA_NV_ERASE_MIN_SIZE)) * TUYA_NV_ERASE_MIN_SIZE) % TUYA_BLE_FILE_RECORD_SIZE;
            } else {
                flag_4k = 1;
            }
        }
        
        if(file_info.data_len == 0) {
            tuya_ble_md5_crypt_loop(&ctx_remoffset,MD5_CRYPT_LOOP_STEP_INIT,pfile_data_req_data, file_info.data_len, md5);
        }

        app_ble_file_erase(erase_addr, TUYA_NV_ERASE_MIN_SIZE);
    }

    //four bytes aligned.
    if(file_data_req.pkt_len % 4) {
        flag_4byte = 4 - (file_data_req.pkt_len % 4);
        file_data_req.pkt_len += flag_4byte;
    }
    
    if (file_info.data_len == file_info.file_len) {
        file_step = FILE_STEP_END;
    } else if(app_ble_file_write(TUYA_BLE_FILE_START_ADDR + (file_info.data_len%TUYA_BLE_FILE_RECORD_SIZE), pfile_data_req_data, file_data_req.pkt_len) == OPRT_OK) {
        //four bytes aligned.
        if(flag_4byte) {
            file_data_req.pkt_len -= flag_4byte;
            flag_4byte = 0;
        }

        file_info.data_len += file_data_req.pkt_len;
        
        if(file_info.data_len < file_info.file_len) {
            file_step = FILE_STEP_DATA;
        } else if(file_info.data_len == file_info.file_len) {
            file_step = FILE_STEP_END;
        } else {
            file_data_rsp.status = FILE_RSP_DATA_ERR_UNKNOWN;
            TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);
            i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
            tuya_ble_file_rsp(rsp, &nrsp_buf, i);
            tuya_ble_file_exit();
            return OPRT_COM_ERROR;
        }
        
        file_info.pkt_id++;
        TAL_PR_INFO("file_info.pkt_id: %d", file_info.pkt_id);

        //computes MD5 for sequential files
        mbedtls_md5_context dctx_remoffset;
        // TAL_PR_HEXDUMP_DEBUG("md5_md5 data",(UINT8_T*)md5,TUYA_BLE_FILE_MD5_LEN);
        tuya_ble_md5_crypt_loop(&ctx_remoffset,MD5_CRYPT_LOOP_STEP_UPDATE,pfile_data_req_data, file_data_req.pkt_len, md5);
        memcpy(&dctx_remoffset, &ctx_remoffset, SIZEOF(ctx_remoffset));
        memcpy(&file_stored_md5_info.ctx_storage, &ctx_remoffset, SIZEOF(ctx_remoffset));//can be read poweroff.
        tuya_ble_md5_crypt_loop(&dctx_remoffset,MD5_CRYPT_LOOP_STEP_FINISH,pfile_data_req_data, file_data_req.pkt_len, md5);
        
        // TAL_PR_HEXDUMP_DEBUG("md5_md5 data",(UINT8_T*)md5,16);
        // TAL_PR_HEXDUMP_DEBUG("file_info.file_md5 data",(UINT8_T*)file_info.file_md5,16);
        memcpy(file_stored_info.file_md5, md5, SIZEOF(md5));

        if(flag_4k) {
            file_stored_info.file_len = file_info.data_len;

            TAL_PR_DEBUG("file_stored_info.file_len :0x%x - file_info.data_len:0x%x", file_stored_info.file_len,file_info.data_len);
            tuya_ble_file_settings_save(&file_stored_info);

            // TAL_PR_HEXDUMP_DEBUG("ctx_remoffset data---S",(UINT8_T*)&file_stored_md5_info.ctx_storage,SIZEOF(mbedtls_md5_context));
            // TAL_PR_HEXDUMP_DEBUG("file_info.file_md5 data---S",(UINT8_T*)file_stored_info.file_md5,16);
        }
    } else {
        //file write error
        file_data_rsp.status = FILE_RSP_DATA_ERR_WFLASH;
        TAL_PR_ERR("FILE_STEP_DATA- error: %d", file_data_rsp.status);
        TAL_PR_DEBUG("pfile_data_req_data:0x%02x - file_data_req.pkt_len:0x%02x,file_stored_info.file_addr:0x%02x, file_info.data_len:0x%02x", pfile_data_req_data,file_data_req.pkt_len,file_stored_info.file_addr,file_info.data_len);

        i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_COM_ERROR;
    }

    i = tuya_ble_file_rsp_pack_data(file_data_rsp, nrsp_buf);
    tuya_ble_file_rsp(rsp, &nrsp_buf, i);
    
    return OPRT_OK;
}

STATIC OPERATE_RET tuya_ble_file_end_handler(UINT8_T *buf, UINT16_T len, tuya_ble_file_response_t *rsp)
{
    UINT8_T nrsp_buf[TUYA_BLE_FILE_RSP_MAX_LEN]={0};
    UINT16_T i = 0;
    TAL_BLE_FILE_INFO_DATA_T file_info_data_fresh;
    TAL_BLE_FILE_END_RSP_T file_end_rsp;

    memset(&file_end_rsp, 0x00, SIZEOF(TAL_BLE_FILE_END_RSP_T));

    if(file_step != FILE_STEP_END) {
        file_end_rsp.status = FILE_RSP_END_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_END- error: %d", file_end_rsp.status);
        i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_RESOURCE_NOT_READY;
    }

    TAL_BLE_FILE_END_REQ_T file_end_req;
    i = 0;
    file_end_req.type = buf[i++];
    file_end_req.file_id = (buf[i]<<8) + buf[i+1];
  
    if(file_end_req.type != file_info.type) {
        file_end_rsp.status = FILE_RSP_END_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_END- error: %d", file_end_rsp.status);
        i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }

    if(file_end_req.file_id != file_info.file_id) {
        file_end_rsp.status = FILE_RSP_END_ERR_UNKNOWN;
        TAL_PR_ERR("FILE_STEP_END- error: %d", file_end_rsp.status);
        i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_NOT_SUPPORTED;
    }

    // response
    file_end_rsp.type = file_info.type;
    file_end_rsp.file_id = file_info.file_id;
    
    if(file_info.data_len != file_info.file_len) {
        //file total length error
        file_end_rsp.status = FILE_RSP_END_ERR_TOTAL_LENGTH;
        TAL_PR_ERR("FILE_STEP_END- error: %d", file_end_rsp.status);
        i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
        tuya_ble_file_rsp(rsp, &nrsp_buf, i);
        tuya_ble_file_exit();
        return OPRT_INDEX_OUT_OF_BOUND;
    }

    //  TAL_PR_HEXDUMP_DEBUG("file_stored_info.file_md5 data",(UINT8_T*)file_stored_info.file_md5,16);
    //  TAL_PR_HEXDUMP_DEBUG("file_info.file_md5 data",(UINT8_T*)file_info.file_md5,16);

     if(memcmp(file_stored_info.file_md5, file_info.file_md5, SIZEOF(file_info.file_md5))) {
        //file md5 error
         file_end_rsp.status = FILE_RSP_END_ERR_MD5;
         TAL_PR_ERR("FILE_STEP_END- error: %d", file_end_rsp.status);
         i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
         tuya_ble_file_rsp(rsp, &nrsp_buf, i);
         tuya_ble_file_exit();
         return OPRT_COM_ERROR;
     }

    TAL_PR_INFO("file download success");

    file_stored_info.file_len = file_info.file_len;
    memcpy(file_stored_info.file_md5, file_info.file_md5, SIZEOF(file_info.file_md5));
    //the current transfer completes file storage.
    tuya_ble_file_settings_save(&file_stored_info);

    memcpy(&file_info_data_fresh, &file_stored_info, SIZEOF(file_stored_info));
    if(file_id_ver_flag == FILE_VERSION_HIGH) {
        file_id_ver_flag = FILE_VERSION_INIT;
        tuya_ble_history_file_del_by_fid(file_info.file_id);
    }
    //flash update record sheet item.
    tuya_ble_history_file_add_record(&file_info_data_fresh);
    
    i = tuya_ble_file_rsp_pack_end(file_end_rsp, nrsp_buf);
    tuya_ble_file_rsp(rsp, &nrsp_buf, i);
    tuya_ble_file_exit();

    // report success info to app
    report_dp_voice_file_stu(file_info.file_id, file_info.file_ver, 1);
    return OPRT_OK;
}

VOID_T tuya_ble_file_handler(tuya_ble_file_data_t *file)
{
    tuya_ble_file_response_t rsp;
    rsp.type = file->type;
    
    if(file->type != TUYA_BLE_FILE_DATA) {
        TAL_PR_INFO("file_type: %d", file->type);
        TAL_PR_HEXDUMP_INFO("file_data", file->p_data, file->data_len);
    }
    
    switch (file->type) {       
        case TUYA_BLE_FILE_INFO: { 
            tuya_ble_file_info_handler(file->p_data, file->data_len, &rsp);
        } break;
        case TUYA_BLE_FILE_OFFSET_REQ: {
            tuya_ble_file_offset_handler(file->p_data, file->data_len, &rsp);
        } break;
        case TUYA_BLE_FILE_DATA: {
            tuya_ble_file_data_handler(file->p_data, file->data_len, &rsp);
        } break;
        case TUYA_BLE_FILE_END: {
            tuya_ble_file_end_handler(file->p_data, file->data_len, &rsp);
        } break;
        case TUYA_BLE_FILE_UNKONWN: {
        } break;

        default: {
        } break;
    }
}

VOID_T tuya_ble_file_disconn_handler(VOID_T)
{
    if(file_step > FILE_STEP_NONE) {
        tuya_ble_file_exit();
    }
}

VOID_T tuya_ble_file_init(VOID_T)
{
    tuya_ble_history_file_data_init();
}

TAL_BLE_FILE_INFO_DATA_T tuya_ble_file_get_stored_info(VOID_T)
{
    app_ble_file_read(TUYA_BLE_FILE_INFO_ADDR, (UINT8_T*)&file_stored_info, SIZEOF(TAL_BLE_FILE_INFO_DATA_T));
    
    return file_stored_info;
}

TUYA_BLE_HISTORY_FILE_T* tuya_ble_history_file_get_record(VOID_T)
{
    return &history_file_data;
}

OPERATE_RET tuya_ble_history_file_del_by_fid(UINT16_T file_fid)
{
    UINT16_T i,temp_fid;
    
    i = history_file_data.data_nums;
        
    for( ; i > 0; i--) {
        temp_fid = history_file_data.file_info_data[i-1].file_id;
        
        if(temp_fid == file_fid) {
            tuya_ble_history_file_del_record(i-1);
            return OPRT_OK;
        } 
    }
    
    return OPRT_NOT_FOUND;
}

TAL_BLE_FILE_INFO_DATA_T* tuya_ble_history_file_get_by_fid(UINT16_T file_fid)
{
    UINT16_T i,temp_fid;

    i = history_file_data.data_nums;
    
    for( ; i > 0; i--) {
        temp_fid = history_file_data.file_info_data[i-1].file_id;
        
        if(temp_fid == file_fid) {
            return &history_file_data.file_info_data[i-1] ;
        }
    }
    
    return NULL;
}
#endif
