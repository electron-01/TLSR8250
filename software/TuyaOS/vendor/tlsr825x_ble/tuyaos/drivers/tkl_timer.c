/**
 * @file tkl_timer.c
 * @brief This is tkl_timer file
 * @version 1.0
 * @date 2021-08-06
 *
 * @copyright Copyright 2018-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"
#include "tkl_timer.h"
#include "tkl_memory.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TUYA_TIMER_BASE_CFG_T tkl_hw_timer_cfg[2];
STATIC UINT_T tkl_hw_timer_us[2] = {0};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
_attribute_ram_code_ OPERATE_RET hal_hw_timer_handler(VOID_T)
{
    if (reg_irq_src & FLD_IRQ_TMR0_EN) {
        reg_tmr_sta |= FLD_TMR_STA_TMR0;
        if (tkl_hw_timer_cfg[0].cb != NULL) {
            tkl_hw_timer_cfg[0].cb(tkl_hw_timer_cfg[0].args);
        }
    }

    if (reg_irq_src & FLD_IRQ_TMR1_EN) {
        reg_tmr_sta |= FLD_TMR_STA_TMR1;
        if (tkl_hw_timer_cfg[1].cb != NULL) {
            tkl_hw_timer_cfg[1].cb(tkl_hw_timer_cfg[1].args);
        }
    }
    return OPRT_OK;
}

OPERATE_RET tkl_timer_init(TUYA_TIMER_NUM_E port, TUYA_TIMER_BASE_CFG_T *cfg)
{
    if (port > TUYA_TIMER_NUM_1) {
        return OPRT_NOT_SUPPORTED;
    }

    if (TUYA_TIMER_MODE_PERIOD != cfg->mode) {
        return OPRT_NOT_SUPPORTED;
    }

    switch (port) {
        case TUYA_TIMER_NUM_0: {
            //enable timer0 interrupt
            reg_irq_mask |= FLD_IRQ_TMR0_EN;
            reg_tmr0_tick = 0;
            break;
        }
        case TUYA_TIMER_NUM_1: {
            //enable timer1 interrupt      trigger slow---donnt no why
            reg_irq_mask |= FLD_IRQ_TMR1_EN;
            reg_tmr1_tick = 0;
            break;
        }
        default: {
            break;
        }
    }

    memcpy(&tkl_hw_timer_cfg[port], cfg, sizeof(TUYA_TIMER_BASE_CFG_T));
    return OPRT_OK;
}

OPERATE_RET tkl_timer_start(TUYA_TIMER_NUM_E port, UINT_T us)
{
    if (port > TUYA_TIMER_NUM_1) {
        return OPRT_NOT_SUPPORTED;
    }

    switch (port) {
        case TUYA_TIMER_NUM_0: {
            //enable timer0 interrupt
            reg_tmr0_capt = CLOCK_SYS_CLOCK_1US * us;
            reg_tmr_ctrl |= FLD_TMR0_EN;
            break;
        }
        case TUYA_TIMER_NUM_1: {
            //enable timer1 interrupt
            reg_tmr1_capt = CLOCK_SYS_CLOCK_1US * us;
            reg_tmr_ctrl |= FLD_TMR1_EN;
            break;
        }
        default: {
            break;
        }
    }

    tkl_hw_timer_us[port] = us;
    return OPRT_OK;
}

OPERATE_RET tkl_timer_stop(TUYA_TIMER_NUM_E port)
{
    if (port > TUYA_TIMER_NUM_1) {
        return OPRT_NOT_SUPPORTED;
    }

    switch (port) {
        case TUYA_TIMER_NUM_0: {
            reg_tmr_ctrl &= (~FLD_TMR0_EN);
        }
        break;
        case TUYA_TIMER_NUM_1: {
            reg_tmr_ctrl &= (~FLD_TMR1_EN);
        }
        break;
        default: {
            break;
        }
    }
    return OPRT_OK;
}

OPERATE_RET tkl_timer_deinit(TUYA_TIMER_NUM_E port)
{
    if (port > TUYA_TIMER_NUM_1) {
        return OPRT_NOT_SUPPORTED;
    }

    switch (port) {
        case TUYA_TIMER_NUM_0: {
            //enable timer0 interrupt
            reg_irq_mask &= (~FLD_IRQ_TMR0_EN);
            break;
        }
        case TUYA_TIMER_NUM_1: {
            //enable timer0 interrupt      trigger slow---donnt no why
            reg_irq_mask &= (~FLD_IRQ_TMR1_EN);
            break;
        }
        default: {
            break;
        }
    }

    memset(&tkl_hw_timer_cfg[port], 0, sizeof(TUYA_TIMER_BASE_CFG_T));
    return OPRT_OK;
}

OPERATE_RET tkl_timer_get(TUYA_TIMER_NUM_E port, UINT_T *us)
{
    if (us == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (port > TUYA_TIMER_NUM_1) {
        return OPRT_NOT_SUPPORTED;
    }

    *us = tkl_hw_timer_us[port];
    return OPRT_OK;
}


