/**
 * @file tkl_gpio.c
 * @brief This is tkl_gpio file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "drivers.h"
#include "board.h"
#include "tkl_gpio.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT8_T valid:1;
    UINT8_T reserved:2;
    UINT8_T pin_id:5;
} TKL_IRQ_PIN_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
// PE0 - PE3 used as system MSPI bus and user can not operate those any time.
STATIC CONST UINT32_T tkl_gpio_list[] = {
    GPIO_PA0, GPIO_PA1, GPIO_PA2, GPIO_PA3, GPIO_PA4, GPIO_PA5, GPIO_PA6, GPIO_PA7, // 0 - 7 //
    GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB4, GPIO_PB5, GPIO_PB6, GPIO_PB7, // 8 - 15 //
    GPIO_PC0, GPIO_PC1, GPIO_PC2, GPIO_PC3, GPIO_PC4, GPIO_PC5, GPIO_PC6, GPIO_PC7, // 16 - 23 //
    GPIO_PD0, GPIO_PD1, GPIO_PD2, GPIO_PD3, GPIO_PD4, GPIO_PD5, GPIO_PD6, GPIO_PD7, // 24 - 31 //
    GPIO_PE0, GPIO_PE1, GPIO_PE2, GPIO_PE3, // 32 - 35//
};

TUYA_GPIO_NUM_E tkl_gpio_test_list[] = {
    TUYA_GPIO_NUM_9,  //GPIO_PB1
    TUYA_GPIO_NUM_15, //GPIO_PB7
    TUYA_GPIO_NUM_13, //GPIO_PB5
    TUYA_GPIO_NUM_12, //GPIO_PB4
    TUYA_GPIO_NUM_0,  //GPIO_PA0
    TUYA_GPIO_NUM_MAX, //NULL
    TUYA_GPIO_NUM_16, //GPIO_PC0
    TUYA_GPIO_NUM_18, //GPIO_PC2
    TUYA_GPIO_NUM_19, //GPIO_PC3
    TUYA_GPIO_NUM_26, //GPIO_PD2
    TUYA_GPIO_NUM_31, //GPIO_PD7
    TUYA_GPIO_NUM_20, //GPIO_PC4
    TUYA_GPIO_NUM_8,  //GPIO_PB0
    TUYA_GPIO_NUM_21, //GPIO_PC5
    TUYA_GPIO_NUM_17, //GPIO_PC1
    TUYA_GPIO_NUM_28, //GPIO_PD4
    TUYA_GPIO_NUM_27, //GPIO_PD3
    TUYA_GPIO_NUM_1,  //GPIO_PA1
    TUYA_GPIO_NUM_14, //GPIO_PB6
};

_attribute_no_ret_bss_ STATIC TUYA_GPIO_IRQ_T tkl_gpio_irq[BOARD_GPIO_IRQ_NUM] = {
    {0, NULL, NULL},
    {0, NULL, NULL}
};
_attribute_no_ret_bss_ STATIC TKL_IRQ_PIN_T irq_pin[BOARD_GPIO_IRQ_NUM] = {
    {0, 0, 0},
    {0, 0, 0}
};
_attribute_no_ret_bss_ STATIC UINT8_T irq_idx = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
UINT32_T tkl_gpio_to_tlsr_gpio(TUYA_GPIO_NUM_E pin_id)
{
    if (pin_id >= (sizeof(tkl_gpio_list)/sizeof(tkl_gpio_list[0]))) {
        return 0xFFFFFFFF;
    }
    UINT32_T gpio_tlsr = tkl_gpio_list[pin_id];
    return gpio_tlsr;
}

OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, CONST TUYA_GPIO_BASE_CFG_T *cfg)
{
   OPERATE_RET ret = OPRT_OK;
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    switch (cfg->mode) {
        case TUYA_GPIO_PULLUP:
            gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_PULLUP_1M);
        break;
        case TUYA_GPIO_PUSH_PULL:
            gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_PULLUP_10K);
        break;
        case TUYA_GPIO_PULLDOWN:
            gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_PULLDOWN_100K);
        break;
        case TUYA_GPIO_FLOATING:
            gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_UP_DOWN_FLOAT);
        break;
        default:
            gpio_setup_up_down_resistor(gpio_tlsr, PM_PIN_UP_DOWN_FLOAT);
        break;
    }

    if (TUYA_GPIO_INPUT == cfg->direct) {
        gpio_set_output_en(gpio_tlsr, 0);
        gpio_set_input_en(gpio_tlsr, 1);
    }
    else if (TUYA_GPIO_OUTPUT == cfg->direct) {
        gpio_set_output_en(gpio_tlsr, 1);
        gpio_set_input_en(gpio_tlsr, 0);
        gpio_write(gpio_tlsr, (cfg->level == TUYA_GPIO_LEVEL_HIGH) ? 1 : 0);
    }
    else {
        return OPRT_NOT_SUPPORTED;
    }

    gpio_set_func(gpio_tlsr, AS_GPIO);

    return ret;
}

OPERATE_RET tkl_gpio_deinit(TUYA_GPIO_NUM_E pin_id)
{
//    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
//    gpio_shutdown(gpio_tlsr);
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_gpio_write(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E level)
{
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
    gpio_write(gpio_tlsr, level?1:0);
    return OPRT_OK;
}

OPERATE_RET tkl_gpio_read(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E *level)
{
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
    if (level == NULL) {
        return OPRT_INVALID_PARM;
    }
    *level = gpio_read(gpio_tlsr)?TUYA_GPIO_LEVEL_HIGH:TUYA_GPIO_LEVEL_LOW;
    return OPRT_OK;
}

OPERATE_RET tkl_gpio_irq_init(TUYA_GPIO_NUM_E pin_id, CONST TUYA_GPIO_IRQ_T *cfg)
{
    OPERATE_RET ret = OPRT_OK;
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
    GPIO_PullTypeDef pull_type;
    UINT32_T falling;

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    for (UINT8_T i = 0; i < BOARD_GPIO_IRQ_NUM; i++) {
        if ((irq_pin[i].valid == 1) && (irq_pin[i].pin_id == pin_id)) {
            return OPRT_INIT_MORE_THAN_ONCE;
        }
    }

    if (irq_idx >= BOARD_GPIO_IRQ_NUM) {
        return OPRT_EXCEED_UPPER_LIMIT;
    }

    memcpy(&tkl_gpio_irq[irq_idx], cfg, SIZEOF(TUYA_GPIO_IRQ_T));
    irq_pin[irq_idx].valid = 1;
    irq_pin[irq_idx].pin_id = pin_id;

    switch (tkl_gpio_irq[irq_idx].mode) {
        case TUYA_GPIO_IRQ_RISE:
            pull_type = PM_PIN_PULLDOWN_100K;
            falling = POL_RISING;
            break;
        case TUYA_GPIO_IRQ_FALL:
            pull_type = PM_PIN_PULLUP_1M;
            falling = POL_FALLING;
            break;
        case TUYA_GPIO_IRQ_RISE_FALL:
            pull_type = PM_PIN_PULLUP_1M;
            falling = POL_FALLING;
            break;
        default:
            return OPRT_NOT_SUPPORTED;
            break;
    }

    irq_idx ++;

    gpio_set_interrupt_init(gpio_tlsr, pull_type, falling, FLD_IRQ_GPIO_EN);
    return ret;
}

OPERATE_RET tkl_gpio_irq_enable(TUYA_GPIO_NUM_E pin_id)
{
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);

    gpio_en_interrupt(gpio_tlsr, 1);
    return OPRT_OK;
}

OPERATE_RET tkl_gpio_irq_disable(TUYA_GPIO_NUM_E pin_id)
{
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(pin_id);
    gpio_en_interrupt(gpio_tlsr, 0);
    return OPRT_OK;
}

_attribute_ram_code_ VOID_T hal_irq_gpio_handler(VOID_T)
{
    UINT32_T src = reg_irq_src;

    if (src & FLD_IRQ_GPIO_EN) {
        UINT32_T gpio_irq_pin = reg_gpio_irq_src;
        UINT32_T tkl_gpio_pin = 0xFF;
        for (int i = 0; i < 32; i++) {
            if (gpio_irq_pin & BIT(i)) {
                tkl_gpio_pin = i;
                break;
            }
        }

        if (0xFF != tkl_gpio_pin) {
            for (UINT8_T idx = 0; idx < BOARD_GPIO_IRQ_NUM; idx++) {
                if ((irq_pin[idx].valid == 1) && (irq_pin[idx].pin_id == tkl_gpio_pin)) {
                    tkl_gpio_irq[idx].cb(tkl_gpio_irq[idx].arg);
                }
            }
        }

        reg_irq_src = FLD_IRQ_GPIO_EN;        // clear irq_gpio irq flag
    }

    /************* gpio irq risc0 *************/
    if (src & FLD_IRQ_GPIO_RISC0_EN) {
        reg_irq_src = FLD_IRQ_GPIO_RISC0_EN;        // clear irq_gpio irq flag
    }

    /************* gpio irq risc1 *************/
    if (src & FLD_IRQ_GPIO_RISC1_EN) {
        reg_irq_src = FLD_IRQ_GPIO_RISC1_EN;        // clear irq_gpio irq flag
    }
}

