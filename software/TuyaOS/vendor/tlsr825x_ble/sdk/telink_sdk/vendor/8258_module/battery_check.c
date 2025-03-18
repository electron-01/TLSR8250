/********************************************************************************************************
 * @file     battery_check.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     Sep. 18, 2015
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *			 The information contained herein is confidential and proprietary property of Telink 
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in. 
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this 
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
/*
 * battery_check.c
 *
 *  Created on: 2018-8-3
 *      Author: Administrator
 */

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "board.h"
#include "battery_check.h"

#include "tuya_cloud_types.h"


#if (BATT_CHECK_ENABLE)
#define ADC_SAMPLE_NUM		4

volatile UINT8_T adc_hw_initialized = 0;
STATIC UINT8_T lowbat_adc_en = 0;
STATIC UINT8_T lowBattDet_enable = 1;
STATIC UINT16_T batt_vol_mv;

STATIC INT32_T app_suspend_enter_low_battery (VOID)
{
	analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG)|LOW_BATT_FLG);  //mark
	return 1;//allow enter cpu_sleep_wakeup
}

STATIC VOID adc_vbat_detect_init(VOID)
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
    adc_set_chn_enable_and_max_state_cnt(ADC_MISC_CHN, 2);  	//set total length for sampling state machine and channel

    //set "capture state" length for misc channel: 240
    //set "set state" length for misc channel: 10
    //adc state machine  period  = 24M/250 = 96K, T = 10.4 uS
    adc_set_state_length(240, 0, 10);  	//set R_max_mc,R_max_c,R_max_s


    //set misc channel use differential_mode (telink advice: only differential mode is available)
    //single mode adc source, PB4 for example: PB4 positive channel, GND negative channel
    gpio_set_func(GPIO_VBAT_DETECT, AS_GPIO);
    gpio_set_input_en(GPIO_VBAT_DETECT, 0);
	gpio_set_output_en(GPIO_VBAT_DETECT, 1);
	gpio_write(GPIO_VBAT_DETECT, 1);
    adc_set_ain_channel_differential_mode(ADC_MISC_CHN, ADC_INPUT_PCHN, GND);

    //set misc channel resolution 14 bit
    //notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
    adc_set_resolution(ADC_MISC_CHN, RES14);  //set resolution

    //set misc channel vref 1.2V
    adc_set_ref_voltage(ADC_MISC_CHN, ADC_VREF_1P2V);  					//set channel Vref

    //set misc t_sample 6 cycle of adc clock:  6 * 1/4M
    adc_set_tsample_cycle(ADC_MISC_CHN, SAMPLING_CYCLES_6);  	//Number of ADC clock cycles in sampling phase

    //set Analog input pre-scaling 1/8
    adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);
    adc_power_on_sar_adc(1); 
}

VOID battery_set_detect_enable (INT32_T en)
{
	lowBattDet_enable = en;
	
	//need initialized again
	adc_hw_initialized = 0;
}

INT32_T battery_get_detect_enable (VOID)
{
	return lowBattDet_enable;
}

int app_battery_power_check(u16 alram_vol_mv)
{
    if(lowBattDet_enable == 0){
        return 0;
    }

	//when MCU powered up or wakeup from deep/deep with retention, adc need be initialized
	if(!adc_hw_initialized){
		adc_hw_initialized = 1;
		adc_vbat_detect_init();
	}

    batt_vol_mv = adc_sample_and_get_result();
	if(batt_vol_mv < alram_vol_mv){
		bls_pm_registerFuncBeforeSuspend( &app_suspend_enter_low_battery );
		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, clock_time() + 1000*CLOCK_SYS_CLOCK_1MS);  //deepsleep
		return 1;
	}else{
		analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG)&(~LOW_BATT_FLG));  //clr
		return 0;
	}
}

#endif
