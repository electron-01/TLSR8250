/**
 * @file app_mdev_demo.c
 * @brief This is app_mdev_demo file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */


/***********************************************************************
 ** INCLUDE                                                           **
 **********************************************************************/
#include "board.h"

#include "tuya_ble_api.h"

#include "tal_bluetooth.h"
#include "tal_log.h"
#include "tal_sw_timer.h"

#include "app_mdev_demo.h"

/***********************************************************************
 ** CONSTANT ( MACRO AND ENUM )                                       **
 **********************************************************************/


/***********************************************************************
 ** STRUCT                                                            **
 **********************************************************************/


/***********************************************************************
 ** VARIABLE                                                          **
 **********************************************************************/
STATIC TY_RSSI_BASE_TEST_T ty_rssi_base_test = {
    .test_en = 0,
    .mdev_index = 0,
    .is_start = 1,
    .mdev_num = 0,
};
STATIC TIMER_ID app_mdev_timer = NULL;

/***********************************************************************
 ** FUNCTON                                                           **
 **********************************************************************/
STATIC VOID_T sort(INT8_T *num_list, UINT8_T cnt)
{
    for(UINT8_T i = 0; i < cnt - 1; i++){
        UINT8_T is_change = 0;
        for(UINT8_T j = 0; j < cnt - 1 - i; j++){
            if(num_list[j] < num_list[j + 1]){
                is_change = 1;
                INT8_T temp = num_list[j];
                num_list[j] = num_list[j + 1];
                num_list[j + 1] = temp;
            }
        }
        if(0 == is_change){
            break;
        }
    }
}

STATIC VOID_T ty_rssi_base_test_update_rssi(INT8_T rssi){
    if(ty_rssi_base_test.is_start == 0){
        return;
    }

    if(ty_rssi_base_test.mdev_index >= MAX_RSSI_NUM){
        ty_rssi_base_test.mdev_index = 0;
    }
    ty_rssi_base_test.mdev_rssi[ty_rssi_base_test.mdev_index] = rssi;
    ty_rssi_base_test.mdev_index++;
    ty_rssi_base_test.mdev_num++;
    if(ty_rssi_base_test.mdev_num > MAX_RSSI_NUM){
        ty_rssi_base_test.mdev_num = MAX_RSSI_NUM;
    }
}

STATIC OPERATE_RET ty_rssi_base_test_get_rssi_avg(INT32_T *rssi)
{
    *rssi = 0;
    if(ty_rssi_base_test.mdev_num == 0){
        return OPRT_NOT_FOUND;
    }

    UINT8_T sum_cnt;
    sort(ty_rssi_base_test.mdev_rssi, ty_rssi_base_test.mdev_num);
    if(ty_rssi_base_test.mdev_num >= (MAX_RSSI_NUM/2)){
        sum_cnt = ty_rssi_base_test.mdev_num/2;
    } else if (ty_rssi_base_test.mdev_num >= (MAX_RSSI_NUM/4)){
        sum_cnt = (MAX_RSSI_NUM/4);
    } else {
        sum_cnt = ty_rssi_base_test.mdev_num;
    }

    for(UINT8_T i=0;i<sum_cnt;i++){
        (*rssi)+=ty_rssi_base_test.mdev_rssi[i];
    }

    (*rssi) /= sum_cnt;

    return OPRT_OK;
}

OPERATE_RET app_mdev_rssi_recv(UINT8_T *adv, UINT8_T adv_len, UINT8_T *mac, int rssi)
{
    if(0 == ty_rssi_base_test.test_en){
        return OPRT_NOT_SUPPORTED;
    }

    UINT8_T *name_data;
    UINT8_T name_data_len = 0;

    if(OPRT_OK == tal_util_adv_report_parse(0x09, adv, adv_len, &name_data, &name_data_len)){
        if ((7 == name_data_len) && (0 == memcmp(name_data, "ty_mdev", 7))) {
            ty_rssi_base_test_update_rssi(rssi);
        }
    }

    return OPRT_OK;
}

STATIC VOID_T app_mdev_timer_cb(TIMER_ID timer_id, VOID_T *arg)
{
    INT32_T rssi = 0;

    tal_sw_timer_delete(app_mdev_timer);

    ty_rssi_base_test_get_rssi_avg(&rssi);
    rssi = -rssi;
    TAL_PR_DEBUG("mdev rssi: %d", rssi);

    if(rssi > -40 && rssi != 0){
        // enter mdev test mode
        TAL_PR_DEBUG("enter mdev test mode");
    }
}

OPERATE_RET app_mdev_check_init(VOID_T)
{
    // do not enter mdev test when device has bounding
    if(tuya_ble_connect_status_get() != UNBONDING_UNCONN){
        ty_rssi_base_test.test_en = 0;
        TAL_PR_INFO("device bounding refuse enter mdev test");
        return OPRT_NOT_SUPPORTED;
    }else{
        ty_rssi_base_test.test_en = 1;
    }

    // note: advice user set a flag that refuse device enter mdev test mode when device bounding or continue running over 30 minute.

    tal_sw_timer_create(app_mdev_timer_cb, NULL, &app_mdev_timer);
    tal_sw_timer_start(app_mdev_timer, 2000, TAL_TIMER_ONCE);

    return OPRT_OK;
}
