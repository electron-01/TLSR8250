/**
 * @file tkl_watchdog.c
 * @brief This is tkl_watchdog file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"

#include "tkl_watchdog.h"

/**
 * Telink 的 看门狗 功能只要打开, 在main程序中会主动喂狗, 无需额外再调用
 *
 */

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/





UINT32_T tkl_watchdog_init(TUYA_WDOG_BASE_CFG_T *cfg)
{
    OPERATE_RET ret = OPRT_OK;
    wd_stop();
    wd_set_interval_ms(cfg->interval_ms, CLOCK_SYS_CLOCK_1MS);
    wd_start();
    return ret;
}

OPERATE_RET tkl_watchdog_deinit(VOID_T)
{
    wd_stop();
    return OPRT_OK;
}

OPERATE_RET tkl_watchdog_refresh(VOID_T)
{
    wd_clear();
    return OPRT_OK;
}

