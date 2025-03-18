/**
 * @file tbs_storage_ib.c
 * @brief This is tbs_storage_ib file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "tbs_storage_ib.h"
#include "tkl_flash.h"
#include "tal_memory.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define _OP_INSERT_ 0
#define _OP_DELETE_ 1
#define _OP_QUERY_  2
#define WRITE_ALIGN(len)        ((len + (4 - 1)) & ~(4 - 1))

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT32_T sg_flash_a_addr = 0;
STATIC UINT32_T sg_flash_b_addr = 0;
STATIC TBS_STORAGE_IB_BLOCK_T *sg_blocks = NULL;
STATIC UINT8_T sg_block_num = 0;

STATIC UINT32_T sg_storage_addr;

STATIC UINT8_T sg_blacklist_block_index;
STATIC UINT8_T sg_blacklist_id;
STATIC UINT8_T sg_blacklist_enable = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC UINT8_T __item_op(UINT8_T op, UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas);

STATIC VOID_T __transfer_blacklist_set(UINT8_T block_index, UINT8_T id)
{
    /**
     * 设置迁移时的黑名单，用于迁移时删除某 block 中的某 ID
     */
    TBS_STORAGE_IB_DEBUG_LOG("block_index = %d, id = %d\n",block_index,id);

    sg_blacklist_block_index = block_index;
    sg_blacklist_id = id;
    sg_blacklist_enable = 1;
}

STATIC UINT8_T __transfer_storage_item(UINT8_T block_index, UINT32_T addr_from, UINT32_T addr_to, UINT16_T storage_size, UINT8_T item_size)
{
    /**
     * 用于将增量式存储的内容进行去重和迁移
     * |--addr_from、addr_to 分别是迁移起始地址
     * |--storage_size 是一个增量式存储空间的总大小
     * |--item_size 是一个存储单元的大小
     *    |--对于单元定长的，item_size 为固定非0值 [ID+内容]
     *    |--对于单元非定长的，item_size 为 0      [ID+内容长度+内容]
     */
    TBS_STORAGE_IB_DEBUG_LOG("block_index = %d, addr_from = %x, addr_to = %x, storage_size = %d, item_size = %d\n",
            block_index, addr_from, addr_to, storage_size, item_size);

//    UINT8_T can_transfer = 0;
    UINT8_T item_num = 0;
    UINT8_T ids[256];  // 256 >= item_num
    UINT8_T lens[256]; // 256 >= item_num
    UINT8_T head[2];

    UINT16_T r_offset = 0, w_offset = 0;
    while (r_offset+item_size <= storage_size) {                  //读取全部序号
        tkl_flash_read(addr_from+r_offset, head, 2);             //读取头两个字节，分别是ID+[LEN] (对于固定单元长度的没有LEN)
        if (head[0] == 0xFF)break;                                   //如果ID为0xFF，认为非法，直接停止

        ids[item_num] = head[0];                                    //记录id
        lens[item_num] = (item_size == 0?head[1]+2:item_size);      //记录len
        r_offset += lens[item_num];                                 //更新读偏移
        item_num++;                                                 //总ITEM数++
    }

    TBS_STORAGE_IB_DEBUG_LOG_HEXs(ids, item_num);

    for (UINT8_T i=item_num-1; i>0; i--) {                               //倒序去重（前面的ID号和后面的一样，则置前面的为0xFF，如果本身就是需要删除的ID，则本身也置 0xFF）
        if (sg_blacklist_enable == 1 &&
                sg_blacklist_block_index == block_index &&
                    sg_blacklist_id == ids[i]) {
            ids[i] = 0xFF;                                      //根据迁移黑名单，在迁移时不将其迁移，实现删除的效果
//            can_transfer = 1;
        }

        if (ids[i] != 0xFF) {
            for (INT8_T j=i-1; j>=0; j--) {
                if ((ids[i] & 0x7F) == (ids[j] & 0x7F)) {
                    ids[j] = 0xFF;
//                    can_transfer = 1;
                }
            }
            if ((ids[i] & 0x80) == 0x80) {
                ids[i] = 0xFF;
//                can_transfer = 1;
            }
        }
    }
    TBS_STORAGE_IB_DEBUG_LOG_HEXs(ids, item_num);
    //if (can_transfer == 0)                                       //没有任何条目能够去重/删除的，说明没有迁移价值
    //    return 0;

    UINT8_T index = 0;
    UINT8_T tmp[66]; //66 >= item_size
    r_offset = 0;
    for (UINT8_T i=0; i<item_num; i++) {                              //逐条迁移
        if (ids[i] != 0xFF) {                                         //检查是否需要迁移（老数据不进行迁移）
            tkl_flash_read(addr_from+r_offset, tmp, lens[i]);      //读取一个ITEM
            if (w_offset+lens[i] <= storage_size)
                tkl_flash_write(addr_to+w_offset, tmp, lens[i]);   //写入一个ITEM
            else                                                        //不够迁移
                return 0;
            w_offset += lens[i];                                        //更新写偏移
            index++;                                                    //更新迁移后的ITEM总数
        }
        r_offset += lens[i];                                        //更新读偏移
    }

    return index;
}

STATIC UINT8_T __transfer_storage_msg(UINT32_T addr_from, UINT32_T addr_to)
{
    TBS_STORAGE_IB_DEBUG_LOG("addr_from = %x, addr_to = %x\n", addr_from, addr_to);

    tkl_flash_erase(addr_to,4096);

    for (UINT8_T i = 0; i < sg_block_num; i++) {                        //逐块迁移
        TBS_STORAGE_IB_BLOCK_T *p = &sg_blocks[i];
        __transfer_storage_item(i,
                                addr_from + p->offset,
                                addr_to + p->offset,
                                p->size,
                                p->item_size);
    }

    tkl_flash_erase(addr_from,4096);

    sg_blacklist_enable = 0;

    return 0;
}

STATIC UINT8_T __check_block_is_empty(UINT32_T addr, UINT8_T is_erase)
{
    // 检查一个 sector = 4K 区域是否干净，如果不干净，实施擦除
    TBS_STORAGE_IB_DEBUG_LOG("addr = %x, is_erase = %d\n",addr,is_erase);

    UINT8_T tmp[256];
    for (UINT8_T i=0; i<16; i++) {
        memset(tmp, 0, 256);
        tkl_flash_read(addr+256*i, tmp, 256);
        for (UINT16_T j=0; j<256; j++) {
            if (tmp[j] != 0xFF) {
                if (is_erase)
                    tkl_flash_erase(addr,4096);
                return 0;
            }
        }
    }
    return 1;
}

STATIC UINT32_T __cal_block_not_ff_num(UINT32_T addr)
{
    // 统计一个 sector = 4K 区域非 0xFF 数据的数量
    TBS_STORAGE_IB_DEBUG_LOG("addr = %x\n", addr);

    UINT8_T tmp[256];
    UINT32_T num = 0;
    for (UINT8_T i=0; i<16; i++) {
        memset(tmp, 0, 256);
        tkl_flash_read(addr+256*i, tmp, 256);
        for (UINT16_T j=0; j<256; j++) {
            if (tmp[j] != 0xFF) {
                num++;
            }
        }
    }
    return num;
}

STATIC UINT8_T __storage_power_on_check(VOID_T)
{
    // 上电检查存储情况：https://wiki.tuya-inc.com:7799/page/1469284538566844478 [上电检测]
    TBS_STORAGE_IB_DEBUG_LOG("...\n");

    UINT8_T no_data[2] = {0, 0};

    no_data[0] = __check_block_is_empty(sg_flash_a_addr, 0);
    no_data[1] = __check_block_is_empty(sg_flash_b_addr, 0);

    TBS_STORAGE_IB_DEBUG_LOG("no_data = [%d, %d]\n", no_data[0], no_data[1]);

    if (no_data[0] == 1 && no_data[1] == 1) {//表示此时没有任何联动信息存储
        sg_storage_addr = sg_flash_a_addr;
    } else if (no_data[0] == 0 && no_data[1] == 1) {
        sg_storage_addr = sg_flash_a_addr;
    } else if (no_data[0] == 1 && no_data[1] == 0) {
        sg_storage_addr = sg_flash_b_addr;
    } else {//两个都有数据，表示可能是上次整理/迁移时断电了
        UINT32_T a_num = __cal_block_not_ff_num(sg_flash_a_addr);
        UINT32_T b_num = __cal_block_not_ff_num(sg_flash_b_addr);

        TBS_STORAGE_IB_DEBUG_LOG("a/b num = [%d, %d]\n", a_num, b_num);

        if (a_num >= b_num) {//A->B 迁移
            __transfer_storage_msg(sg_flash_a_addr, sg_flash_b_addr);
            sg_storage_addr = sg_flash_b_addr;
        } else {//B->A 迁移
            __transfer_storage_msg(sg_flash_b_addr, sg_flash_a_addr);
            sg_storage_addr = sg_flash_a_addr;
        }
    }

    return 0;
}

STATIC UINT8_T __insert(UINT8_T block_index, UINT16_T r_offset, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas, UINT8_T is_delete)
{
    TBS_STORAGE_IB_DEBUG_LOG("block_index = %d, r_offset = %x, item_id = %d, datas_len = %d, is_delete = %d\n",block_index,r_offset,item_id,datas_len,is_delete);

    UINT8_T tmp[66]; //66 >= item_size
    UINT8_T tmp2[66];

    TBS_STORAGE_IB_BLOCK_T *p = &sg_blocks[block_index];
    UINT8_T item_size = p->item_size;                                        //根据 p->item_size 类型，计算 item_size

    if (is_delete) {
        if (p->item_size == 0) {
            item_size = 4;
            tmp[0] = item_id;
            tmp[1] = 2;
        } else {
            tmp[0] = item_id | 0x80;
            memset(&tmp[1], 0, datas_len);
        }
    } else {
        if (p->item_size == 0) {
            item_size = datas_len+2;
            tmp[0] = item_id;
            tmp[1] = datas_len;
            memcpy(&tmp[2], datas, datas_len);
        } else if (datas_len+1 == p->item_size) {
            tmp[0] = item_id;
            memcpy(&tmp[1], datas, datas_len);
        } else {
            return 0;
        }
    }


    if (item_size + r_offset <= p->size) {                                //空间足够--插入
        tkl_flash_write(sg_storage_addr+p->offset+r_offset, tmp, item_size);

        memset(tmp2, 0, item_size);                                           //读出来比较下
        tkl_flash_read(sg_storage_addr+p->offset+r_offset, tmp2, item_size);
        if (memcmp(tmp, tmp2, item_size) == 0)
            return 1;
        else
            return 0;
    } else {                                                              //空间不够--迁移
        STATIC UINT8_T try_times = 0;                                            //统计空间整理次数，因为有递归调用，因此这里要有次数限制
        if (try_times < 1) {
            if (is_delete == 1)                                          //删除时，由于没有足够空间进行以增加的方式删除，就要直接进行删除
                __transfer_blacklist_set(block_index, item_id);

            if (sg_storage_addr == sg_flash_a_addr) {//A->B 迁移
                __transfer_storage_msg(sg_flash_a_addr, sg_flash_b_addr);
                sg_storage_addr = sg_flash_b_addr;
            } else {//B->A 迁移
                __transfer_storage_msg(sg_flash_b_addr, sg_flash_a_addr);
                sg_storage_addr = sg_flash_a_addr;
            }
            try_times++;
            UINT8_T ret = __item_op(is_delete == 0?_OP_INSERT_:_OP_DELETE_, block_index, item_id, datas_len, datas);
            try_times--;
            return ret;
        } else {
            try_times = 0;
            return 0;
        }
    }
}

STATIC UINT8_T __query(UINT8_T block_index, UINT8_T *last_target_head, UINT16_T last_target_offset, UINT8_T datas_len, UINT8_T *datas)
{
    TBS_STORAGE_IB_DEBUG_LOG("block_index = %d, last_target_head = [%d,%d], last_target_offset = %x, datas_len = %d\n",
            block_index, last_target_head[0], last_target_head[1], last_target_offset, datas_len);

    if ((last_target_head[0] & 0x80) == 0x80)                          //该ID数据已经被删除或不存在
        return 0;

    TBS_STORAGE_IB_BLOCK_T *p = &sg_blocks[block_index];

    UINT8_T tmp[66]; //66 >= item_size
    UINT8_T item_datas_len;
    UINT8_T item_size;
    UINT8_T *p_datas;
    if (p->item_size == 0) {//不定长item
        item_datas_len = last_target_head[1];
        item_size = item_datas_len+2;
        p_datas = &tmp[2];
    } else {//定长
        item_datas_len = p->item_size-1;
        item_size = p->item_size;
        p_datas = &tmp[1];
    }

    if (datas_len < item_datas_len)
        return 0;

    tkl_flash_read(sg_storage_addr+p->offset+last_target_offset, tmp, item_size);
    memcpy(datas, p_datas, item_datas_len);
    return item_datas_len;
}

//STATIC UINT8_T __delete(UINT8_T *last_target_head) {
//    TBS_STORAGE_IB_DEBUG_LOG("last_target_head = [%d, %d]\n", last_target_head[0], last_target_head[1]);

//    if (last_target_head[0] == 0xFF)                                 //该ID数据已经不存在
//        return 1;
//    if ((last_target_head[0] & 0x80) == 0x80)                          //数据已经标记删除
//        return 1;

//    //新增一条失能记录(定长的数据长度为定长，非定长的数据长度为0)
//    //为了防止因为使用新增条目的方法删除导致没有多余空间，因此在新增的时候要至少留足一次删除的空间
//
//    return 0;
//}

STATIC UINT8_T __item_op(UINT8_T op, UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas)
{
    // 元素插入/删除/读取
    TBS_STORAGE_IB_DEBUG_LOG("op = %d, block_index = %d, item_id = %d, datas_len = %d\n",op,block_index,item_id,datas_len);

    if (block_index >= sg_block_num)return 0;
    if ((item_id & 0x80) == 0x80)return 0;

    TBS_STORAGE_IB_BLOCK_T *p = &sg_blocks[block_index];

    UINT8_T head[2];
    UINT8_T last_target_head[2] = {0xFF, 0xFF};
    UINT16_T last_target_offset;
    UINT16_T r_offset = 0;
    UINT16_T item_num = 0;
    while (r_offset+p->item_size <= p->size) {                            //读取全部序号
        tkl_flash_read(sg_storage_addr+p->offset+r_offset, head, 2);    //读取头两个字节，分别是ID+[LEN] (对于固定单元长度的没有LEN)
        if (head[0] == 0xFF) break;                                          //如果ID为0xFF，认为非法，直接停止
        if ((head[0] & 0x7F) == item_id) {                                      //记录目标最后一个head (方便做读取/删除操作:如果本来就不存在，直接返回删除成功；
            last_target_head[0] = head[0];                                      //如果本来存在，采用置失能标志位方法资源不够，则直接实施数据精简迁移)
            last_target_head[1] = head[1];
            last_target_offset = r_offset;
        }
        r_offset += (p->item_size == 0?head[1]+2:p->item_size);             //更新 read offset
        item_num++;
    }

    if (op == _OP_QUERY_) {//query
        TBS_STORAGE_IB_DEBUG_LOG("query: block_index = %d, last_target_head = [%d,%d], last_target_offset = %x, datas_len = %x",block_index,last_target_head[0],last_target_head[1],last_target_offset,datas_len);
        return __query(block_index, last_target_head, last_target_offset, datas_len, datas);
    } else if (op == _OP_DELETE_) {//删除
        if ((last_target_head[0] == 0xFF) || ((last_target_head[0] & 0x80) == 0x80))
            return 1;
        return __insert(block_index, r_offset, item_id, 0, NULL, 1);
    } else if (op == _OP_INSERT_) {//新增
        return __insert(block_index, r_offset, item_id, datas_len, datas, 0);
    }

    return 0;
}

UINT8_T tbs_storage_ib_init(UINT32_T flash_a_addr, UINT32_T flash_b_addr, TBS_STORAGE_IB_BLOCK_T *p_blocks, UINT8_T block_num)
{
    TBS_STORAGE_IB_DEBUG_LOG("-->\n");
    //read from flash
        //|-- local_autos(need read to ram)   9*32 bytes
        //|-- local_auto_events(need read to ram)  11*48 bytes
        //|-- local_auto_actions(no need read to ram)  单条最大限制:(max 3 dp(max dp = 14bytes)+3 delay(3bytes)+3 auto(4bytes) + 1 = 3*(14+3+4)+1=64bytes)，总共占用资源 64*32 = 2048K
    //ty_mesh_local_auto_read_form_flash();
    //ty_mesh_local_auto_event_read_form_flash();

    sg_flash_a_addr = flash_a_addr;
    sg_flash_b_addr = flash_b_addr;
    sg_blocks = p_blocks;
    sg_block_num = block_num;

    for (uint32_t idx=0; idx<sg_block_num; idx++) {
        sg_blocks[idx].item_size = WRITE_ALIGN(sg_blocks[idx].item_size);
    }

    __storage_power_on_check();

    return 1;
}

UINT8_T tbs_storage_ib_clear(VOID_T)
{
    tkl_flash_erase(sg_flash_a_addr,4096);
    tkl_flash_erase(sg_flash_b_addr,4096);
    return 1;
}

UINT8_T tbs_storage_ib_delete_item(UINT8_T block_index, UINT8_T item_id)
{
    TBS_STORAGE_IB_DEBUG_LOG("-->\n");
    return __item_op(_OP_DELETE_, block_index, item_id, 0, NULL);
}

UINT8_T tbs_storage_ib_insert_item(UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas)
{
    TBS_STORAGE_IB_DEBUG_LOG("-->\n");

    uint32_t len = 0;
    if (sg_blocks[block_index].item_size == 0) {
        len= WRITE_ALIGN(datas_len+2)-2;
    } else {
        len= WRITE_ALIGN(datas_len+1)-1;
    }

    return __item_op(_OP_INSERT_, block_index, item_id, len, datas);
}

UINT8_T tbs_storage_ib_query_item(UINT8_T block_index, UINT8_T item_id, UINT8_T datas_len, UINT8_T *datas)
{
    TBS_STORAGE_IB_DEBUG_LOG("-->\n");

    uint32_t len = 0;
    if (sg_blocks[block_index].item_size == 0) {
        len= WRITE_ALIGN(datas_len+2)-2;
    } else {
        len= WRITE_ALIGN(datas_len+1)-1;
    }

    uint8_t* tmp_buf = tal_malloc(len);
    if (tmp_buf) {
        memset(tmp_buf, 0, len);
        memcpy(datas, tmp_buf, datas_len);
    }
    return __item_op(_OP_QUERY_, block_index, item_id, len, datas);
}

