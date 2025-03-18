/**
 * @file tkl_board.c
 * @brief This is tkl_board file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "stack/ble/ble.h"
#include "board.h"
#include "drivers.h"
#include "tkl_system.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
VOID_T (*tkl_board_irq_func)(VOID_T) = NULL;
UINT32_T tkl_board_state_suspend = 0;
UINT32_T tkl_board_state_working = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
VOID_T tkl_board_irq_callback_register(VOID_T(*func)(VOID_T))
{
    if (func) {
        tkl_board_irq_func = func;
    }
}

_attribute_ram_code_ UINT32_T tkl_ble_trxfifo_not_empty(VOID_T)
{
    UINT32_T ret = 0;
    if (bls_ll_isConnectState()) {
        // ret = (blt_rxfifo.rptr != blt_rxfifo.wptr)||(blt_txfifo.rptr != blt_txfifo.wptr);
        ret = (blt_rxfifo.rptr != blt_rxfifo.wptr) || blc_ll_getTxFifoNumber();
    }
    return ret;
}

VOID_T tkl_board_pm_suspend(TKL_PM_STATE_E state, TKL_PM_EVENT_E event)
{
    if (state == TKL_PM_CLEAR) {
        tkl_board_state_suspend &= ~(1 << event);
    } else {
        tkl_board_state_suspend |= (1 << event);
    }
}

VOID_T tkl_board_pm_working(TKL_PM_STATE_E state, TKL_PM_EVENT_E event)
{
    if (state == TKL_PM_CLEAR) {
        tkl_board_state_working &= ~(1 << event);
    } else {
        tkl_board_state_working |= (1 << event);
    }
}

