/**
 * @file tkl_sleep.c
 * @brief This is tkl_sleep file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "pm.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "battery_check.h"
#include "vendor/common/blt_common.h"
#include "../common/blt_soft_timer.h"
#include "tkl_sleep.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern SYS_TIME_T tkl_sw_timer_next_tick;
BOOL_T g_system_sleep = FALSE;
TUYA_SLEEP_CB_T g_tkl_cpu_sleep_callback;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
OPERATE_RET tkl_cpu_sleep_callback_register(TUYA_SLEEP_CB_T *sleep_cb)
{
    if (sleep_cb == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (sleep_cb->pre_sleep_cb) {
        g_tkl_cpu_sleep_callback.pre_sleep_cb   = sleep_cb->pre_sleep_cb;
    }

    if (sleep_cb->post_wakeup_cb) {
        g_tkl_cpu_sleep_callback.post_wakeup_cb = sleep_cb->post_wakeup_cb;
    }

    return OPRT_OK;
}

VOID_T tkl_cpu_allow_sleep(VOID_T)
{
    g_system_sleep = TRUE;
}

VOID_T tkl_cpu_force_wakeup(VOID_T)
{
    g_system_sleep = FALSE;

    bls_pm_setSuspendMask (SUSPEND_DISABLE);
}

OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
    if(mode == TUYA_CPU_SLEEP){
        if(enable) {
            tkl_board_state_suspend |= (1 << 31);
        } else {
            tkl_board_state_suspend &= ~(1 << 31);
        }
    } else if(mode == TUYA_CPU_DEEP_SLEEP){
        tkl_board_state_suspend &= ~(1 << 31);
        if(enable) {
        } else {
            tkl_board_state_suspend |= (1 << 31);
        }
    } else {
        return OPRT_INVALID_PARM;
    }

    return OPRT_OK;
}

_attribute_ram_code_ BOOL_T tkl_cpu_is_sleep(VOID_T)
{
    return g_system_sleep;
}

