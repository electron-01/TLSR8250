/********************************************************************************************************
 * @file     main.c 
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

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_fw_sign.h"
#include "vendor/common/blt_common.h"
#include "board.h"

extern void user_init_normal();
extern void user_init_deepRetn();
extern void main_loop (void);
extern VOID_T (*tkl_board_irq_func)(VOID_T);

#define   FirmwareAddr     0x40000  //如果FirmwareSize大于124k,则配置为0x40000,否则配置为0x20000

#ifdef HCI_ACCESS
u16 uart_tx_irq=0, uart_rx_irq=0;
extern my_fifo_t hci_rx_fifo;

_attribute_ram_code_ void irq_uart_handle()
{
#if (HCI_ACCESS==HCI_USE_UART)
	unsigned char irqS = reg_dma_rx_rdy0;
	if(irqS & FLD_DMA_CHN_UART_RX)	//rx
	{
		uart_rx_irq++;
		reg_dma_rx_rdy0 = FLD_DMA_CHN_UART_RX;
		u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
		if(w[0]!=0)
		{
			my_fifo_next(&hci_rx_fifo);
			u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
			reg_dma0_addr = (u16)((u32)p);
		}
	}

	if(irqS & FLD_DMA_CHN_UART_TX)	//tx
	{
		uart_tx_irq++;
		reg_dma_rx_rdy0 = FLD_DMA_CHN_UART_TX;
	}
#endif
}
#endif

_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler ();

#ifdef HCI_ACCESS
	irq_uart_handle();
#endif

    extern OPERATE_RET hal_uart_irq_handler(VOID_T);
    hal_uart_irq_handler();

    extern OPERATE_RET hal_hw_timer_handler(VOID_T);
    hal_hw_timer_handler();

    extern VOID_T hal_irq_gpio_handler(VOID_T);
    hal_irq_gpio_handler();

	if(tkl_board_irq_func){
		tkl_board_irq_func();
	}
}

_attribute_ram_code_ int main (void)    //must run in ramcode
{
#if defined(BOARD_ENABLE_EXTERNAL_32K_RC) && (BOARD_ENABLE_EXTERNAL_32K_RC == 1)
	blc_pm_select_external_32k_crystal();
#else
	blc_pm_select_internal_32k_crystal();
#endif

	bls_ota_set_fwSize_and_fwBootAddr(BOARD_FLASH_OTA_SIZE/1024, FirmwareAddr);//修改要存储的固件地址和固件大小信息

	blc_app_setExternalCrystalCapEnable(1);

	cpu_wakeup_init();

	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init( !deepRetWakeUp );  //analog resistance will keep available in deepSleep mode, so no need initialize again

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 32000000)
	clock_init(SYS_CLK_32M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 48000000)
	clock_init(SYS_CLK_48M_Crystal);
#endif

	if(!deepRetWakeUp){//read flash size
		blc_readFlashSize_autoConfigCustomFlashSector();
	}

	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value

	if( deepRetWakeUp ){
		user_init_deepRetn ();
	} else {
		#if FIRMWARES_SIGNATURE_ENABLE
			blt_firmware_signature_check();
		#endif
        
		user_init_normal ();

		unsigned char  boot_flag = read_reg8(0x63e);
		write_reg8(0x40004,0xA5);
		write_reg8(0x40005,boot_flag);
	}

    irq_enable();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}
