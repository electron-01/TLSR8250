/**
 * @file tkl_adc.c
 * @brief This is tkl_adc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"
#include "battery_check.h"
#include "tkl_adc.h"
/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TUYA_ADC_BASE_CFG_T tkl_adc_cfg = {
    .ch_nums = 1,
    .type = TUYA_ADC_EXTERNAL_SAMPLE_VOL,
    .width = 14,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC UINT_T tkl_adc_resolution_transform(UINT_T width)
{
    switch (width) {
        case 8:     {return RES8;}
        case 10:    {return RES10;}
        case 12:    {return RES12;}
        case 14:    {return RES14;}
        default:      {return -1;}
    }
}

STATIC UINT8_T tkl_adc_adc_channel_transform(UINT32_T bit)
{
    UINT8_T i = 0;
    for (i = 0; i < 32; i++) {
        if (bit & (1 << i)) {
            break;
        }
    }
    return i;
}

STATIC VOID_T adc_gpio_ain_init(TUYA_ADC_TYPE_E type, UINT32_T pin, UINT8_T channle)
{
    ////Step 1: power off sar adc/////////////////////////////////////////////////////////
    /******power off sar adc********/
    adc_power_on_sar_adc(0);
    //////////////////////////////////////////////////////////////////////////////////////

    ////Step 2: Config some common adc settings(user can not change these)/////////////////
    /******enable signal of 24M clock to sar adc********/
    adc_enable_clk_24m_to_sar_adc(1);

    /******set adc sample clk as 4MHz******/
    adc_set_sample_clk(5); //adc sample clk= 24M/(1+5)=4M

    /******set adc L R channel Gain Stage bias current trimming******/
    adc_set_left_gain_bias(GAIN_STAGE_BIAS_PER100);
    adc_set_right_gain_bias(GAIN_STAGE_BIAS_PER100);
    ////////////////////////////////////////////////////////////////////////////////////////

    ////Step 3: Config adc settings  as needed /////////////////////////////////////////////
    //set misc channel en,  and adc state machine state cnt 2( "set" stage and "capture" state for misc channel)
    adc_set_chn_enable_and_max_state_cnt(ADC_MISC_CHN, 2);      //set total length for sampling state machine and channel

    //set "capture state" length for misc channel: 240
    //set "set state" length for misc channel: 10
    //adc state machine  period  = 24M/250 = 96K, T = 10.4 uS
    adc_set_state_length(240, 0, 10);      //set R_max_mc,R_max_c,R_max_s


    //set misc channel use differential_mode (telink advice: only differential mode is available)
    //single mode adc source, PB4 for example: PB4 positive channel, GND negative channel
    gpio_set_func(pin, AS_GPIO);
    gpio_set_input_en(pin, 0);
    if (type == TUYA_ADC_INNER_SAMPLE_VOL) {
        //interval detect
        gpio_set_output_en(pin, 1);
        gpio_write(pin, 1);
    } else {
        //external detect
        gpio_set_output_en(pin, 0);
        gpio_write(pin, 0);
    }
    adc_set_ain_channel_differential_mode(ADC_MISC_CHN, channle, GND);

    //set misc channel resolution 14 bit
    //notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
    adc_set_resolution(ADC_MISC_CHN, RES14);  //set resolution

    //set misc channel vref 1.2V
    adc_set_ref_voltage(ADC_MISC_CHN, ADC_VREF_1P2V);                      //set channel Vref

    //set misc t_sample 6 cycle of adc clock:  6 * 1/4M
    adc_set_tsample_cycle(ADC_MISC_CHN, SAMPLING_CYCLES_6);      //Number of ADC clock cycles in sampling phase

    //set Analog input pre-scaling 1/8
    adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);
    adc_power_on_sar_adc(1);
}

OPERATE_RET tkl_adc_init(TUYA_ADC_NUM_E port_num, TUYA_ADC_BASE_CFG_T *cfg)
{
    /*
        ADC CHANNLE PIN:
        GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB4,// 0 - 4
        GPIO_PB5, GPIO_PB6, GPIO_PB7, GPIO_PC4, GPIO_PC5 // 5 - 9
    */
    UINT8_T adc_channel = tkl_adc_adc_channel_transform(cfg->ch_list.data);
    UINT32_T pin = ADC_GPIO_tab[adc_channel];

    // tlsr825x PB1/PB7 used as UART TX/RX
    if (adc_channel == 7 || adc_channel == 1) {
        return OPRT_NOT_SUPPORTED;
    }

    if (adc_channel >= 10) {
        return OPRT_NOT_SUPPORTED;
    }

    if (tkl_adc_resolution_transform(cfg->width) == (UINT_T)-1) {
        return OPRT_NOT_SUPPORTED;
    }

    memcpy(&tkl_adc_cfg, cfg, sizeof(tkl_adc_cfg));

    adc_gpio_ain_init(tkl_adc_cfg.type, pin, adc_channel+1);
    // adc_gpio_ain_init(TUYA_ADC_EXTERNAL_SAMPLE_VOL, pin, adc_channel+1);

#if (BATT_CHECK_ENABLE)
    battery_set_detect_enable(0);
#endif

    return OPRT_OK;
}

OPERATE_RET tkl_adc_deinit(TUYA_ADC_NUM_E port_num)
{
    UINT8_T adc_channel = port_num;
    if (adc_channel > 10) {
        return OPRT_NOT_SUPPORTED;
    }

    /******power off sar adc********/
    adc_power_on_sar_adc(0);
    /****** sar adc Reset ********/
    //reset whole digital adc module
    adc_reset_adc_module();

#if (BATT_CHECK_ENABLE)
    battery_set_detect_enable(1);
#endif

    return OPRT_OK;
}

UINT8_T tkl_adc_width_get(TUYA_ADC_NUM_E port_num)
{
    UINT8_T adc_channel = port_num;
    if (adc_channel > 10) {
        return OPRT_NOT_SUPPORTED;
    }

    return tkl_adc_cfg.width;
}

UINT32_T tkl_adc_ref_voltage_get(TUYA_ADC_NUM_E port_num)
{
    return 1200;
}

INT32_T tkl_adc_temperature_get(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_adc_read_data(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    /*
        ADC CHANNLE PIN:
        GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB4,// 0 - 4
        GPIO_PB5, GPIO_PB6, GPIO_PB7, GPIO_PC4, GPIO_PC5 // 5 - 9
    */
    UINT8_T adc_channel = tkl_adc_adc_channel_transform(tkl_adc_cfg.ch_list.data);

    if (adc_channel > 10) {
        return OPRT_NOT_SUPPORTED;
    }

    if (adc_channel == 7 || adc_channel == 1) {
        return OPRT_NOT_SUPPORTED;
    }

    if (buff == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (len == 0) {
        return OPRT_INVALID_PARM;
    }

    buff[0] = adc_sample_and_get_result();
    // adc_power_on_sar_adc(0);

    return OPRT_OK;
}

OPERATE_RET tkl_adc_read_single_channel(TUYA_ADC_NUM_E port_num, UINT8_T ch_id, INT32_T *data)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_adc_read_voltage(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    return tkl_adc_read_data(port_num, buff, len);
}

