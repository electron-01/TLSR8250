/**
 * @file tbs_storage_ib.h
 * @brief This is tbs_storage_ib file
 * @version 1.0
 * @date 2022-06-14
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TBS_STORAGE_IB_H__
#define __TBS_STORAGE_IB_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "tal_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#if 0
#define TBS_STORAGE_IB_DEBUG_LOG         TAL_PR_INFO
#define TBS_STORAGE_IB_DEBUG_LOG_HEXs(a, b)    TAL_PR_HEXDUMP_INFO("11111", a, b)
#define TBS_STORAGE_IB_DEBUG_LOG_RAW     PR_DEBUG_RAW
#else
#define TBS_STORAGE_IB_DEBUG_LOG(...)
#define TBS_STORAGE_IB_DEBUG_LOG_HEXs(...)
#define TBS_STORAGE_IB_DEBUG_LOG_RAW(...)
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
/*
 * 限制：
 * 1）增量双备份存储，最大存储块为 4K，采用双备份存储
 * 2）每个 4K 可以分成多个存储块，每个存储块内可以实施独立的增量式存储
 * 3）每个存储块可容纳的存储条目不要超过256个，每个存储条目的总长不要超过66字节
 * 4）每个存储条目支持定长：ID(1byte)+DATAS
 *                  不定长：ID(1byte)+DATAS_LEN(1byte)+DATAS
 *                  --> ID=1+7bits, 7bits表示实际ID, 1bit为0表示该ID数据有效, 1bit为1表示该ID数据已经被删除(通过增加条目的方式来删除，减小 flash 写操作)
 */
//存储块结构体
typedef struct {
    UINT16_T offset;             //增量式存储块存储的初始位置，相对于 4K 存储区的偏移
    UINT16_T size;               //增量式存储块的容量(多少字节)
    UINT8_T item_size;           //增量式存储块的条目的长度(如果为 0 表示不定长)
}TBS_STORAGE_IB_BLOCK_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
// A\B 区地址
// 块数组
// 块数组大小

/**
 * @brief tbs_storage_ib_init
 *
 * @param[in] flash_a_addr: flash_a_addr
 * @param[in] flash_b_addr: flash_b_addr
 * @param[in] *p_blocks: *p_blocks
 * @param[in] block_num: block_num
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tbs_storage_ib_init(UINT32_T flash_a_addr, UINT32_T flash_b_addr, TBS_STORAGE_IB_BLOCK_T *p_blocks, UINT8_T block_num);

/**
 * @brief tbs_storage_ib_clear
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tbs_storage_ib_clear(VOID_T);

/**
 * @brief tbs_storage_ib_delete_item
 *
 * @param[in] block_index: block_index
 * @param[in] item_id: item_id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tbs_storage_ib_delete_item(UINT8_T block_index, UINT8_T item_id);

/**
 * @brief tbs_storage_ib_insert_item
 *
 * @param[in] block_index: block_index
 * @param[in] item_id: item_id
 * @param[in] datas_len: datas_len
 * @param[in] *datas: *datas
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tbs_storage_ib_insert_item(UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas);

/**
 * @brief tbs_storage_ib_query_item
 *
 * @param[in] block_index: block_index
 * @param[in] item_id: item_id
 * @param[in] datas_len: datas_len
 * @param[in] *datas: *datas
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tbs_storage_ib_query_item(UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas);


#ifdef __cplusplus
}
#endif

#endif /* __TBS_STORAGE_IB_H__ */

