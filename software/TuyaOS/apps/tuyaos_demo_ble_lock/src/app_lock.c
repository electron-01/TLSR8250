/**
 * @file app_lock.c
 * @brief This is app_lock file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */


#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_gpio.h"
#include "app_dp_parser.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TIMER_ID autolock_timer_id = NULL;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




STATIC VOID_T autolock_timeout_handler(TIMER_ID timer_id, VOID_T *arg)
{
    UINT8_T data[5] = {0};
    data[0] = AUTO_CLOSE_LOCK;
    data[4] = 0xFF;
    app_dp_record_report(OR_DP_RECORD_CLOSE_LOCK_COMMON, data, 5);

    UINT8_T lock_status = 0x00;     //closed
    app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);

    TAL_PR_INFO("auto close lock");
}

VOID_T app_lock_timer_init(VOID_T)
{
    tal_sw_timer_create(autolock_timeout_handler, NULL, &autolock_timer_id);
}

VOID_T app_lock_autolock_timer_start(UINT32_T time_ms)
{
    if (autolock_timer_id == NULL) {
        app_lock_timer_init();
    }

    tal_sw_timer_start(autolock_timer_id, time_ms, TAL_TIMER_ONCE);
}

VOID_T app_lock_autolock_timer_stop(VOID_T)
{
    if (autolock_timer_id == NULL) {
        app_lock_timer_init();
    }

    tal_sw_timer_stop(autolock_timer_id);
}

