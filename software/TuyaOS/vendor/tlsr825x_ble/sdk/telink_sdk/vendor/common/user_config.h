/********************************************************************************************************
 * @file     user_config.h 
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

#ifndef __PROJECT_8258_MODULE__
#define __PROJECT_8258_MODULE__ 1
#endif

#if (__PROJECT_8258_DRIVER_TEST__)
	#include "../8258_driver_test/telink_app_config.h"
#elif (__PROJECT_8258_BLE_REMOTE__)
	#include "../8258_ble_remote/telink_app_config.h"
#elif (__PROJECT_8258_BLE_SAMPLE__)
	#include "../8258_ble_sample/telink_app_config.h"
#elif (__PROJECT_8258_MODULE__)
	#include "../8258_module/telink_app_config.h"
#elif (__PROJECT_8258_HCI__)
	#include "../8258_hci/telink_app_config.h"
#elif (__PROJECT_8258_FEATURE_TEST__)
	#include "../8258_feature_test/telink_app_config.h"
#elif(__PROJECT_8258_MASTER_KMA_DONGLE__ )
	#include "../8258_master_kma_dongle/telink_app_config.h"
#else
	#include "../common/default_config.h"
#endif

