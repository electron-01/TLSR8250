/********************************************************************************************************
 * @file     u_printf.h 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 30, 2010
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

#include "telink_app_config.h"

#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)//print info by a gpio or usb printer
	int  u_printf(const char *fmt, ...);
	int  u_sprintf(char* s, const char *fmt, ...);
	void u_array_printf(unsigned char*data, unsigned int len);

	#define printf	 		u_printf
	#define sprintf	 		u_sprintf
    #define array_printf	u_array_printf
#else
	int  u_sprintf(char* s, const char *fmt, ...);
	#define printf
	#define sprintf         u_sprintf
	#define array_printf
#endif

