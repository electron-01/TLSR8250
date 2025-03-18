/********************************************************************************************************
 * @file     putchar.c 
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

#ifndef WIN32

#include "tl_common.h"
#include "application/print/putchar.h"
#include "application/usbstd/usbhw.h"
#include "drivers.h"

#define BIT_INTERVAL (16000000/BOARD_LOG_BAUD_RATE)

_attribute_ram_code_
int uart_putc(char byte) //GPIO simulate uart print func
{
#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)
    extern unsigned int tkl_log_gpio;
    unsigned char  j = 0;
    unsigned int t1 = 0,t2 = 0;

    unsigned int  pcTxReg = (0x583+((tkl_log_gpio>>8)<<3));//register GPIO output
    unsigned char tmp_bit0 = read_reg8(pcTxReg) & (~(tkl_log_gpio & 0xff));
    unsigned char tmp_bit1 = read_reg8(pcTxReg) | (tkl_log_gpio & 0xff);
    unsigned char bit[10] = {0};

    bit[0] = tmp_bit0;
    bit[1] = (byte & 0x01)? tmp_bit1 : tmp_bit0;
    bit[2] = ((byte>>1) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[3] = ((byte>>2) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[4] = ((byte>>3) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[5] = ((byte>>4) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[6] = ((byte>>5) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[7] = ((byte>>6) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[8] = ((byte>>7) & 0x01)? tmp_bit1 : tmp_bit0;
    bit[9] = tmp_bit1;

    unsigned char r = irq_disable();
    t1 = read_reg32(0x740);
    for(j = 0;j<10;j++)
    {
        t2 = t1;
        while(t1 - t2 < BIT_INTERVAL){
            t1  = read_reg32(0x740);
        }
        write_reg8(pcTxReg,bit[j]);        //send bit0
    }
    irq_restore(r);
#endif
    return byte;
}

int putchar(int c){
#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)
    return uart_putc((char)c);
#else
    return c;
#endif
}

#endif

