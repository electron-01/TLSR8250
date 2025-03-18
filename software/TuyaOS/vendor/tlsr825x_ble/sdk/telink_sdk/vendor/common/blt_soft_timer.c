/********************************************************************************************************
 * @file     blt_soft_timer.c 
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
 * blt_soft_timer.c
 *
 *  Created on: 2016-10-28
 *      Author: Administrator
 */

#include "stack/ble/ble.h"
#include "tl_common.h"
#include "../common/blt_soft_timer.h"

#if (BLT_SOFTWARE_TIMER_ENABLE)
_attribute_data_retention_	blt_soft_timer_t	blt_timer = {
    .timer[0].timer_id = TLSR_TIMER0_EVT,
    .timer[1].timer_id = TLSR_TIMER1_EVT,
    .timer[2].timer_id = TLSR_TIMER2_EVT,
    .timer[3].timer_id = TLSR_TIMER3_EVT,
    .timer[4].timer_id = TLSR_TIMER4_EVT,
    .timer[5].timer_id = TLSR_TIMER5_EVT,
    .timer[6].timer_id = TLSR_TIMER6_EVT,
    .timer[7].timer_id = TLSR_TIMER7_EVT,
    .timer[8].timer_id = TLSR_TIMER8_EVT,
    .timer[9].timer_id = TLSR_TIMER9_EVT,
    .timer[10].timer_id = TLSR_TIMER10_EVT,
    .timer[11].timer_id = TLSR_TIMER11_EVT,
    .timer[12].timer_id = TLSR_TIMER12_EVT,
    .timer[13].timer_id = TLSR_TIMER13_EVT,
    .timer[14].timer_id = TLSR_TIMER14_EVT,
    .timer[15].timer_id = TLSR_TIMER15_EVT,
    .timer[16].timer_id = TLSR_TIMER16_EVT,
    .timer[17].timer_id = TLSR_TIMER17_EVT,
    .timer[18].timer_id = TLSR_TIMER18_EVT,
    .timer[19].timer_id = TLSR_TIMER19_EVT,
};
//Sort the timers according to the timing time, which is convenient for triggering the timers in sequence during the process
int  blt_soft_timer_sort(void)
{
	if(blt_timer.currentNum < 1 || blt_timer.currentNum > MAX_TIMER_NUM)
	{
		while(1); //debug ERR
		return 0;
    }
	else
	{
		//BubbleSort
		int n = blt_timer.currentNum;
		int m = 0;
		u8 temp[sizeof(blt_time_event_t)];

		for(int i=0;i<n-1;i++)
		{
			for(int j=0;j<n-i-1;j++)
			{
				//if(TIME_COMPARE_BIG(blt_timer.timer[j].t, blt_timer.timer[j+1].t))
				if((blt_timer.timer[j].run_state == 0)&&(blt_timer.timer[j+1].run_state != 0))
				{
					//swap
					memcpy(temp, &blt_timer.timer[j+1], sizeof(blt_time_event_t));
					memcpy(&blt_timer.timer[j+1], &blt_timer.timer[j], sizeof(blt_time_event_t));
					memcpy(&blt_timer.timer[j], temp, sizeof(blt_time_event_t));
				}
			}
		}

		for(int i=0;i<n;i++)
		{
			if(blt_timer.timer[i].run_state != 0)
			{
				m = i;
			}else{
				break;
			}
		}


		for(int i=0;i<m;i++)
		{
			for(int j=0;j<m-i;j++)
			{
				if(TIME_COMPARE_BIG(blt_timer.timer[j].t, blt_timer.timer[j+1].t))
				{
					//swap
					memcpy(temp, &blt_timer.timer[j], sizeof(blt_time_event_t));
					memcpy(&blt_timer.timer[j], &blt_timer.timer[j+1], sizeof(blt_time_event_t));
					memcpy(&blt_timer.timer[j+1], temp, sizeof(blt_time_event_t));
				}
			}
		}


	}
	return 1;
}


//user add timer
int blt_soft_timer_add(blt_timer_callback_t func, u32 interval_us)
{
	int i;
	u32 now = clock_time();
	if(blt_timer.currentNum >= MAX_TIMER_NUM)
	{  //timer full
		return 	0;
	}
	else
	{
		blt_timer.timer[blt_timer.currentNum].cb = func;
		blt_timer.timer[blt_timer.currentNum].interval = interval_us * CLOCK_16M_SYS_TIMER_CLK_1US;
		blt_timer.timer[blt_timer.currentNum].t = now + blt_timer.timer[blt_timer.currentNum].interval;
		blt_timer.currentNum ++;

		blt_soft_timer_sort();

		bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);

		return  1;
	}
}

int blt_soft_timer_task_create(void **timer_id,u32 timeout_value_ms, tuya_ble_timer_mode mode,tuya_ble_timer_handler_t timeout_handler)
{
	int i;
	u32 interval_us = timeout_value_ms*1000;
	blt_soft_timer_add(timeout_handler,interval_us);
	for(i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].cb == timeout_handler)
		{
			*timer_id = blt_timer.timer[i].timer_id;
			blt_timer.timer[i].mode = mode;
			blt_timer.timer[i].run_state = 0;
			break;
		}
	}
	return 0;
}

int  blt_soft_timer_task_delete(void *timer_id)
{
	int res = 0,i;
	for(i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].timer_id == timer_id)
		{
			blt_timer.timer[i].run_state = 0;
			blt_soft_timer_delete_by_index(i);
			if(i == 0)
			{  //The most recent timer is deleted, and the time needs to be updated
				if( (u32)(blt_timer.timer[0].t - clock_time()) < 3000 *  CLOCK_16M_SYS_TIMER_CLK_1MS)
				{
					bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);
				}
				else
				{
					bls_pm_setAppWakeupLowPower(0, 0);  //disable
				}
			}
			res = 1;
			break;
		}
	}
	return res;
}

int  blt_soft_timer_task_start(void *timer_id)
{
	int res = 0;
	for(int i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].timer_id == timer_id)
		{
			if(blt_timer.timer[i].run_state == 0)
			{
				u32 now = clock_time();
				blt_timer.timer[i].run_state = 1;
				blt_timer.timer[i].t = now + blt_timer.timer[i].interval;
				blt_soft_timer_sort();
				if( (u32)(blt_timer.timer[0].t - now) < 3000 *  CLOCK_16M_SYS_TIMER_CLK_1MS)
				{
					bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);
				}
				else
				{
					bls_pm_setAppWakeupLowPower(0, 0);  //disable
				}
				res = 1;
			}
			break;
		}
	}
	return res;
}

int  blt_soft_timer_task_stop(void *timer_id)
{
	int res = 0;
	for(int i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].timer_id == timer_id)
		{
			blt_timer.timer[i].run_state = 0;
			res = 1;
			break;
		}
	}
	return res;
}

int  blt_soft_timer_task_restart(void *timer_id,u32 timeout_value_ms)
{
	int res = 0;
	for(int i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].timer_id == timer_id)
		{
			u32 now = clock_time();
			blt_timer.timer[i].run_state = 1;
			blt_timer.timer[i].interval = timeout_value_ms * 1000 * CLOCK_16M_SYS_TIMER_CLK_1US;
			blt_timer.timer[i].t = now + blt_timer.timer[i].interval;
			blt_soft_timer_sort();
			if( (u32)(blt_timer.timer[0].t - now) < 3000 *  CLOCK_16M_SYS_TIMER_CLK_1MS)
			{
				bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);
			}
			else
			{
				bls_pm_setAppWakeupLowPower(0, 0);  //disable
			}
			res = 1;
			break;
		}
	}
	return res;
}

//Timers are originally ordered. When deleted, they are overwritten in advance, so the order will not be destroyed, and there is no need to reorder
int  blt_soft_timer_delete_by_index(u8 index)
{
	if(index >= blt_timer.currentNum)
	{
		while(1); //debug ERR
		return 0;
	}


	for(int i=index; i<blt_timer.currentNum - 1; i++)
	{
		memcpy(&blt_timer.timer[i], &blt_timer.timer[i+1], sizeof(blt_time_event_t));
	}

	blt_timer.currentNum --;
}

int blt_soft_timer_delete(blt_timer_callback_t func)
{
	for(int i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_timer.timer[i].cb == func)
		{
			blt_soft_timer_delete_by_index(i);

			if(i == 0)
			{  //The most recent timer is deleted, and the time needs to be updated

				if( (u32)(blt_timer.timer[0].t - clock_time()) < 3000 *  CLOCK_16M_SYS_TIMER_CLK_1MS)
				{
					bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);
				}
				else
				{
					bls_pm_setAppWakeupLowPower(0, 0);  //disable
				}
			}
			return 1;
		}
	}
	return 0;
}

/**/
void  	blt_soft_timer_process(int type)
{
	if(type == CALLBACK_ENTRY)
	{

	}
	u32 now = clock_time();
	if(!blt_timer.currentNum)
	{
		bls_pm_setAppWakeupLowPower(0, 0);  //disable
		return;
	}

	if( !blt_is_timer_expired(blt_timer.timer[0].t, now) )
	{
		return;
	}

	int  change_flg = 0;
	int  result;
	for(int i=0; i<blt_timer.currentNum; i++)
	{
		if(blt_is_timer_expired(blt_timer.timer[i].t ,now))
		{
			if(blt_timer.timer[i].cb != NULL)
			{
				if(blt_timer.timer[i].run_state == 1)
				{
					result = blt_timer.timer[i].cb(NULL);
				}
				if(blt_timer.timer[i].mode == TUYA_BLE_TIMER_SINGLE_SHOT)
				{
					blt_timer.timer[i].run_state = 0;
				}
				change_flg = 1;
				blt_timer.timer[i].t = now + blt_timer.timer[i].interval;
			}
		}
	}

	if(blt_timer.currentNum )
	{ //timer table not empty
		if(change_flg)
		{
			blt_soft_timer_sort();
		}

		if(( (u32)(blt_timer.timer[0].t - now) < 3000 *  CLOCK_16M_SYS_TIMER_CLK_1MS)&&(blt_timer.timer[0].run_state !=0))
		{
			bls_pm_setAppWakeupLowPower(blt_timer.timer[0].t,  1);
		}
		else
		{
			bls_pm_setAppWakeupLowPower(0, 0);  //disable
		}
	}
	else
	{
		bls_pm_setAppWakeupLowPower(0, 0);  //disable
	}
}


void 	blt_soft_timer_init(void)
{
	bls_pm_registerAppWakeupLowPowerCb(blt_soft_timer_process);
}


#endif  //end of  BLT_SOFTWARE_TIMER_ENABLE

