/**
 * @file tkl_wakeup.c
 * @brief This is tkl_wakeup file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"

#include "tkl_wakeup.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define WAKEUP_SOURCE_COUNT 5

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
OPERATE_RET tkl_wakeup_source_set(CONST TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    if (param == NULL) {
        return OPRT_INVALID_PARM;
    }

    switch (param->source) {
        case TUYA_WAKEUP_SOURCE_TIMER: {
            return OPRT_NOT_SUPPORTED;
        }
        break;

        case TUYA_WAKEUP_SOURCE_GPIO: {
            UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(param->wakeup_para.gpio_param.gpio_num);
            gpio_set_func(gpio_tlsr, AS_GPIO);
            gpio_set_input_en(gpio_tlsr, 1);
            gpio_set_output_en(gpio_tlsr, 0);
            if (TUYA_GPIO_LEVEL_LOW == param->wakeup_para.gpio_param.level) {
                gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_PULLUP_1M);
                cpu_set_gpio_wakeup(gpio_tlsr, Level_Low, 1);
            }
            else {
                gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_PULLDOWN_100K);
                cpu_set_gpio_wakeup(gpio_tlsr, Level_High, 1);
            }
        }
        break;

        default:
            return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_wakeup_source_clear(CONST TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    if (param == NULL) {
        return OPRT_INVALID_PARM;
    }

    switch (param->source) {
        case TUYA_WAKEUP_SOURCE_TIMER: {
            return OPRT_NOT_SUPPORTED;
        }
        break;

        case TUYA_WAKEUP_SOURCE_GPIO: {
            UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(param->wakeup_para.gpio_param.gpio_num);
            cpu_set_gpio_wakeup(gpio_tlsr, Level_Low, 0);
        }
        break;

        default:
            return OPRT_NOT_SUPPORTED;
    }


    return OPRT_OK;
}

