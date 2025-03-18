/**
 * @file tkl_rtc.c
 * @brief This is tkl_rtc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"
#include "tkl_rtc.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TIME_T s_local_timestamp = 0;
STATIC BOOL_T s_rtc_is_init     = FALSE;
STATIC SYS_TIME_T tkl_system_timer_clock = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
_attribute_ram_code_ VOID_T tuya_rtc_loop_handler(VOID_T)
{
    SYS_TIME_T now_tick  = 0;
    SYS_TIME_T last_tick = 0;
    SYS_TIME_T seconds   = 0;

    last_tick = tkl_system_timer_clock;
    now_tick = clock_time();
    seconds = ((now_tick >= last_tick) ? (now_tick - last_tick) : (0xFFFFFFFF - last_tick + now_tick)) / CLOCK_16M_SYS_TIMER_CLK_1S;
    if (seconds > 0) {
        s_local_timestamp += seconds; //unix time
        tkl_system_timer_clock = tkl_system_timer_clock + (seconds * CLOCK_16M_SYS_TIMER_CLK_1S);
    }
}

OPERATE_RET tkl_rtc_init(VOID_T)
{
    s_rtc_is_init = TRUE;
    return OPRT_OK;
}

OPERATE_RET tkl_rtc_deinit(VOID_T)
{
    s_rtc_is_init = FALSE;
    return OPRT_OK;
}

OPERATE_RET tkl_rtc_time_set(TIME_T time_sec)
{
    s_local_timestamp = time_sec;
    return OPRT_OK;
}

OPERATE_RET tkl_rtc_time_get(TIME_T *time_sec)
{
    if (time_sec == NULL) {
        return OPRT_INVALID_PARM;
    }

    *time_sec = s_local_timestamp;

    return OPRT_OK;
}

