/********************************************************************************************************
 * @file     telink_app_config.h 
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
#pragma once
#include "board.h"
#include "types.h"
#include "custom_tuya_ble_config.h"
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define   CODE_PAGE_SIZE                    4096

/////////////////// MODULE /////////////////////////////////
#define BLE_MODULE_PM_ENABLE				 1
#define PM_DEEPSLEEP_RETENTION_ENABLE		 1

#define TELIK_SPP_SERVICE_ENABLE			 1

//tuya warning:batt check must be enabled
#define BATT_CHECK_ENABLE       			 1   //enable or disable battery voltage detection

#define BLT_APP_LED_ENABLE					 0

//firmware check
#define FIRMWARES_SIGNATURE_ENABLE           0

//////////////// SMP SETTING  //////////////////////////////
#define BLE_SECURITY_ENABLE 			   	 0


/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   DEEP_ANA_REG0 //u8,can save 8 bit info when deep
#define	LOW_BATT_FLG					     BIT(0)
#define	MCU_REBOOT_FLG					     BIT(1)
#define	FLD_SOFT_REBOOT_FLAG				 BIT(2)



#if (BATT_CHECK_ENABLE)
//telink device: you must choose one gpio with adc function to output high level(voltage will equal to vbat), then use adc to measure high level voltage
	//use PC5 output high level, then adc measure this high level voltage
	#define GPIO_VBAT_DETECT				GPIO_PC5
	#define PC5_FUNC						AS_GPIO
	#define PC5_INPUT_ENABLE				0
	#define ADC_INPUT_PCHN					C5P    //corresponding  ADC_InputPchTypeDef in adc.h
#endif

//////////////////// LED CONFIG (EVK board) ///////////////////////////
#if (BLT_APP_LED_ENABLE)
	#define LED_ON_LEVAL 					1 			//gpio output high voltage to turn on led
	#define	GPIO_LED						GPIO_PD5    //red
	#define PD5_FUNC						AS_GPIO
#endif


/////////////////// Clock  /////////////////////////////////
#ifndef CLOCK_SYS_CLOCK_HZ
#define CLOCK_SYS_CLOCK_HZ  				48000000
#endif
#if ((CLOCK_SYS_CLOCK_HZ != 16000000) && (CLOCK_SYS_CLOCK_HZ != 24000000) && (CLOCK_SYS_CLOCK_HZ != 48000000))
#error "CLOCK_SYS_CLOCK_HZ value defined error"
#endif

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};



/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE				1
#define WATCHDOG_INIT_TIMEOUT				4000  //ms

/////////////////// PM  //////////////////////////////
#ifndef PM_RETENTION_THRESHOLD_ADV
#define PM_RETENTION_THRESHOLD_ADV				105  // ms
#endif

#ifndef PM_RETENTION_THRESHOLD_CONN
#define PM_RETENTION_THRESHOLD_CONN				95  // ms
#endif

#ifndef PM_EARLYWAKEUP
#define PM_EARLYWAKEUP							1000 // us
#endif

///////////////////////////////////// ATT  HANDLER define ///////////////////////////////////////
typedef enum
{
	ATT_START_H = 0,

	//// Gap ////
	/**********************************************************************************************/
	GenericAccess_PS_H, 					//UUID: 2800, 	VALUE: uuid 1800
	GenericAccess_DeviceName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	GenericAccess_DeviceName_DP_H,			//UUID: 2A00,   VALUE: device name
	GenericAccess_Appearance_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	GenericAccess_Appearance_DP_H,			//UUID: 2A01,	VALUE: appearance
	CONN_PARAM_CD_H,						//UUID: 2803, 	VALUE:  			Prop: Read
	CONN_PARAM_DP_H,						//UUID: 2A04,   VALUE: connParameter

	//// gatt ////
	/**********************************************************************************************/
	GenericAttribute_PS_H,					//UUID: 2800, 	VALUE: uuid 1801
	GenericAttribute_ServiceChanged_CD_H,	//UUID: 2803, 	VALUE:  			Prop: Indicate
	GenericAttribute_ServiceChanged_DP_H,   //UUID:	2A05,	VALUE: service change
	GenericAttribute_ServiceChanged_CCB_H,	//UUID: 2902,	VALUE: serviceChangeCCC

	//// SPP ////
	/**********************************************************************************************/
	TKL_SERVICE_H,
	TKL_WRITE_CHAR_H,
	TKL_WRITE_CHAR_VALUE_H,
	TKL_NOTIFY_CHAR_H,
	TKL_NOTIFY_CHAR_VALUE_H,
	TKL_NOTIFY_CHAR_CCC_H,
	TKL_READ_CHAR_H,
	TKL_READ_CHAR_VALUE_H,

	ATT_END_H,

}ATT_HANDLE;

#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

