/********************************************************************************************************
 * @file     blt_soft_timer.h 
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
 * blt_soft_timer.h
 *
 *  Created on: 2016-10-28
 *      Author: Administrator
 */

#ifndef BLT_SOFT_TIMER_H_
#define BLT_SOFT_TIMER_H_
#include "tuya_ble_type.h"

//user define
#ifndef BLT_SOFTWARE_TIMER_ENABLE
#define BLT_SOFTWARE_TIMER_ENABLE					0   //enable or disable
#endif


#define 	MAX_TIMER_NUM							20   //timer max number

#define     TLSR_TIMER0_EVT                         0
#define     TLSR_TIMER1_EVT                         1
#define     TLSR_TIMER2_EVT                         2
#define     TLSR_TIMER3_EVT                         3
#define     TLSR_TIMER4_EVT                         4
#define     TLSR_TIMER5_EVT                         5
#define     TLSR_TIMER6_EVT                         6
#define     TLSR_TIMER7_EVT                         7
#define     TLSR_TIMER8_EVT                         8
#define     TLSR_TIMER9_EVT                         9
#define     TLSR_TIMER10_EVT                        10
#define     TLSR_TIMER11_EVT                        11
#define     TLSR_TIMER12_EVT                        12
#define     TLSR_TIMER13_EVT                        13
#define     TLSR_TIMER14_EVT                        14
#define     TLSR_TIMER15_EVT                        15
#define     TLSR_TIMER16_EVT                        16
#define     TLSR_TIMER17_EVT                        17
#define     TLSR_TIMER18_EVT                        18
#define     TLSR_TIMER19_EVT                        19

#define		MAINLOOP_ENTRY							0
#define 	CALLBACK_ENTRY							1



//if t1 < t2  return 1
#define		TIME_COMPARE_SMALL(t1,t2)   ( (u32)((t2) - (t1)) < BIT(30)  )

// if t1 > t2 return 1
#define		TIME_COMPARE_BIG(t1,t2)   ( (u32)((t1) - (t2)) < BIT(30)  )


#define		BLT_TIMER_SAFE_MARGIN_PRE	  (CLOCK_16M_SYS_TIMER_CLK_1US<<7)  //128 us
#define		BLT_TIMER_SAFE_MARGIN_POST	  (CLOCK_16M_SYS_TIMER_CLK_1S<<2)   // 4S
static int inline blt_is_timer_expired(u32 t, u32 now) {
	return ((u32)(now + BLT_TIMER_SAFE_MARGIN_PRE - t) < BLT_TIMER_SAFE_MARGIN_POST);
}

typedef int (*blt_timer_callback_t)(void*);

typedef struct blt_time_event_t {
	blt_timer_callback_t    cb;
	u32                     t;
	u32                     interval;
	u32                     timer_id;
	int                     mode;
	int                     run_state;
} blt_time_event_t;

// timer table managemnt
typedef struct blt_soft_timer_t {
	blt_time_event_t	timer[MAX_TIMER_NUM];  //timer0 - timer3
	u8					currentNum;  //total valid timer num
} blt_soft_timer_t;

//////////////////////// USER  INTERFACE ///////////////////////////////////
//return 0 means Fail, others OK
int 	blt_soft_timer_add(blt_timer_callback_t func, u32 interval_us);
int 	blt_soft_timer_delete(blt_timer_callback_t func);




//////////////////////// SOFT TIMER MANAGEMENT  INTERFACE ///////////////////////////////////
void 	blt_soft_timer_init(void);
void  	blt_soft_timer_process(int type);
int 	blt_soft_timer_delete_by_index(u8 index);

int  blt_soft_timer_task_create(void **timer_id,u32 timeout_value_ms, tuya_ble_timer_mode mode,tuya_ble_timer_handler_t timeout_handler);
int  blt_soft_timer_task_delete(void *timer_id);
int  blt_soft_timer_task_start(void *timer_id);
int  blt_soft_timer_task_stop(void *timer_id);
int  blt_soft_timer_task_restart(void *timer_id,u32 timeout_value_ms);

int is_timer_expired(blt_timer_callback_t *e);


#endif /* BLT_SOFT_TIMER_H_ */
