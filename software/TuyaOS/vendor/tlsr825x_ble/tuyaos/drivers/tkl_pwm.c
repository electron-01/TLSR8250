/**
 * @file tkl_pwm.c
 * @brief This is tkl_pwm file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "drivers.h"
#include "board.h"
#include "tkl_pwm.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define PWM_CLOCK  (CLOCK_SYS_CLOCK_HZ/2)
#define PWM_ID_MAX TUYA_PWM_NUM_MAX

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
TUYA_PWM_BASE_CFG_T tkl_pwm_cfg[PWM_ID_MAX];

STATIC UINT32_T pwm_tlsr_io_map[PWM_ID_MAX] = {
    GPIO_PC2, //C2
    GPIO_PC3, //C3
    GPIO_PD4, //D4
    GPIO_PD2, //D2
    GPIO_PB4, //B4
    GPIO_PB5, //B5
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
OPERATE_RET tkl_pwm_init(TUYA_PWM_NUM_E ch_id, CONST TUYA_PWM_BASE_CFG_T *cfg)
{
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }
    
    if(ch_id >= PWM_ID_MAX){
        return OPRT_INVALID_PARM;
    }

    memcpy(&tkl_pwm_cfg[ch_id], cfg, sizeof(TUYA_PWM_BASE_CFG_T));

    pwm_set_clk(CLOCK_SYS_CLOCK_HZ, PWM_CLOCK);

    UINT32_T pwm_max_tick  = PWM_CLOCK / cfg->frequency;
    UINT_T pwm_duty = (UINT_T)(pwm_max_tick * ((float)cfg->duty / 100));
    pwm_set(ch_id, pwm_max_tick, pwm_duty);
    if (TUYA_PWM_POSITIVE == cfg->polarity) {
        pwm_n_revert(ch_id);
    }
    else {
        pwm_revert(ch_id);
    }
    gpio_set_func(pwm_tlsr_io_map[ch_id], AS_PWM);

    return tkl_pwm_start(ch_id);
}

OPERATE_RET tkl_pwm_mapping_to_gpio(TUYA_PWM_NUM_E ch_id, TUYA_GPIO_NUM_E gpio)
{
    if(ch_id >= PWM_ID_MAX){
        return OPRT_INVALID_PARM;
    }

    extern UINT32_T tkl_gpio_to_tlsr_gpio(TUYA_GPIO_NUM_E pin_id);
    UINT32_T gpio_tlsr = tkl_gpio_to_tlsr_gpio(gpio);
    if (GET_PWMID(gpio_tlsr, AS_PWM) == ch_id) {
        gpio_set_func(gpio_tlsr, AS_PWM);
        pwm_tlsr_io_map[ch_id] = gpio_tlsr;
        return OPRT_OK;
    }

    return OPRT_COM_ERROR;
}

OPERATE_RET tkl_pwm_deinit(TUYA_PWM_NUM_E ch_id)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_start(TUYA_PWM_NUM_E ch_id)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    pwm_start(ch_id);
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_stop(TUYA_PWM_NUM_E ch_id)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    pwm_stop(ch_id);
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_duty_set(TUYA_PWM_NUM_E ch_id, UINT32_T duty)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    tkl_pwm_cfg[ch_id].duty = duty;
    UINT32_T pwm_max_tick  = PWM_CLOCK / tkl_pwm_cfg[ch_id].frequency;
    UINT_T pwm_duty = (UINT_T)(pwm_max_tick * ((float)duty / 1000000));
    pwm_set_cmp(ch_id, pwm_duty);
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_frequency_set(TUYA_PWM_NUM_E ch_id, UINT32_T frequency)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    tkl_pwm_cfg[ch_id].frequency = frequency;
    UINT32_T pwm_max_tick  = PWM_CLOCK / frequency;
    UINT_T pwm_duty = (UINT_T)(pwm_max_tick * ((float)tkl_pwm_cfg[ch_id].duty / 1000000));
    pwm_set(ch_id, pwm_max_tick, pwm_duty);
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_polarity_set(TUYA_PWM_NUM_E ch_id, TUYA_PWM_POLARITY_E polarity)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    tkl_pwm_cfg[ch_id].polarity = polarity;

    if (TUYA_PWM_POSITIVE == polarity) {
        pwm_n_revert(ch_id);
    }
    else {
        pwm_revert(ch_id);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_pwm_info_set(TUYA_PWM_NUM_E ch_id, CONST TUYA_PWM_BASE_CFG_T *info)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    if (info == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (0 == memcmp(info, &tkl_pwm_cfg[ch_id], sizeof(TUYA_PWM_BASE_CFG_T))) {
        return OPRT_OK;
    }

    return tkl_pwm_init(ch_id, info);
}

OPERATE_RET tkl_pwm_info_get(TUYA_PWM_NUM_E ch_id, TUYA_PWM_BASE_CFG_T *info)
{
    if (ch_id >= PWM_ID_MAX) {
        return OPRT_NOT_SUPPORTED;
    }

    if (info == NULL) {
        return OPRT_INVALID_PARM;
    }

    memcpy(info, &tkl_pwm_cfg[ch_id], sizeof(TUYA_PWM_BASE_CFG_T));

    return OPRT_OK;
}

