/**
 * @file board.h
 * @brief This is board file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "tuya_cloud_types.h"
#include "app_config.h"
#include "tal_log.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#ifndef BOARD_ENABLE_LOG
#define BOARD_ENABLE_LOG                        (0)
#endif

#if (BOARD_ENABLE_LOG)
    #define TY_PRINTF(fmt, ...)                 TAL_PR_INFO(fmt, ##__VA_ARGS__)
    #define TY_HEXDUMP(title, buf, size)        TAL_PR_HEXDUMP_INFO(title, buf, size)
#else
    #define TY_PRINTF(fmt, ...)
    #define TY_HEXDUMP(title, buf, size)
#endif

//RAM
#ifndef BOARD_HEAP_SIZE
#define BOARD_HEAP_SIZE                         (1024*5)
#endif

#ifndef BOARD_UART_TX_BUF_SIZE
#define BOARD_UART_TX_BUF_SIZE                  (512)
#endif

// Flash
#ifndef BOARD_FLASH_SDK_TEST_START_ADDR
#define BOARD_FLASH_SDK_TEST_START_ADDR        (0x2C000)
#endif

#ifndef BOARD_FLASH_TUYA_INFO_START_ADDR
#define BOARD_FLASH_TUYA_INFO_START_ADDR        (0x6C000)
#endif

// OTA
extern unsigned int ota_program_offset;
#ifndef BOARD_FLASH_OTA_INFO_ADDR
#define BOARD_FLASH_OTA_INFO_ADDR               (0x70000)
#endif

#ifndef BOARD_FLASH_OTA_START_ADDR
#define BOARD_FLASH_OTA_START_ADDR              (ota_program_offset)
#endif

#ifndef BOARD_FLASH_OTA_END_ADDR
#define BOARD_FLASH_OTA_END_ADDR                (ota_program_offset + 0x2B000)
#endif

#ifndef BOARD_FLASH_OTA_SIZE
#define BOARD_FLASH_OTA_SIZE                    (0x2B000)
#endif

// #ifndef BOARD_FLASH_MAC_START_ADDR
// #define BOARD_FLASH_MAC_START_ADDR              (0)
// #endif

// PIN
#ifndef BOARD_POWER_ON_PIN
#define BOARD_POWER_ON_PIN                      (TUYA_GPIO_NUM_26)
#endif

#ifndef BOARD_KEY_PIN
#define BOARD_KEY_PIN                           (TUYA_GPIO_NUM_28)
#endif

#ifndef BOARD_LOG_TX_PIN
#define BOARD_LOG_TX_PIN                        (TUYA_GPIO_NUM_0)
#endif

// IRQ NUM
#ifndef BOARD_GPIO_IRQ_NUM
#define BOARD_GPIO_IRQ_NUM                      (2)
#else
#if BOARD_GPIO_IRQ_NUM < 1
#error "BOARD_GPIO_IRQ_NUM must >= 1 !!!!"
#endif
#endif

// LOG
#ifndef TUYA_BLE_LOG_ENABLE
#define TUYA_BLE_LOG_ENABLE                     (0)
#endif

#ifndef TUYA_APP_LOG_ENABLE
#define TUYA_APP_LOG_ENABLE                     ENABLE_LOG
#endif

#ifndef BOARD_LOG_BAUD_RATE
#define BOARD_LOG_BAUD_RATE                     (1000000)
#endif

#ifndef ENABLE_SCAN
#define ENABLE_SCAN (0) 
#endif
#define TKL_BLUETOOTH_SUPPORT_SCAN      ENABLE_SCAN

#ifndef TKL_BLUETOOTH_PHY_2M
#define TKL_BLUETOOTH_PHY_2M    (0)
#endif

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef enum {
    TY_BLE_STA_IDLE = 0,
    TY_BLE_STA_ADV,
    TY_BLE_STA_CONN,
} TKL_BOARD_BLE_STATUS_E;

typedef enum {
    TKL_PM_CLEAR = 0,
    TKL_PM_SET,
} TKL_PM_STATE_E;

typedef enum {
    PM_EVENT_LED = 0,
    PM_EVENT_KEY,
    PM_EVENT_BUZZER,
    PM_EVENT_UART,
    PM_EVENT_PAIRING,
    PM_EVENT_OTA,
} TKL_PM_EVENT_E;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
// 这个函数可以判定设备是处于连接状态还是广播状态,或者都不是,方便低功耗的设计
extern TKL_BOARD_BLE_STATUS_E tkl_ble_state;
extern UINT32_T tkl_board_state_suspend;
extern UINT32_T tkl_board_state_working;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
/**
 * @brief connect parameters update result callback register
 *     callback fucntion with 2 input param:
 *      first: id , default 0, user don't need care
 *      second: result, result = 0x0000 means CONN_PARAM_UPDATE_ACCEPT
 *                      result = 0x0001 means CONN_PARAM_UPDATE_REJECT,
 *
 * @param[in] param: OPERATE_RET(*func)(UINT8_T, UINT16_T)
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
extern OPERATE_RET tkl_ble_conn_param_update_rsp_cb_register(OPERATE_RET(*func)(UINT8_T, UINT16_T));

/**
 * @brief telink irq handler callback. user can define private process function, such as FLD_IRQ_PWM0_IR_DMA_FIFO_DONE
 *        the function of callback must descript as _attribute_ram_code_

 * @param[in] param: VOID_T(*func)(VOID_T)
 *
 * @return VOID
 */
extern VOID_T tkl_board_irq_callback_register(VOID_T(*func)(VOID_T));

/**
 * @brief check rf fifo status

 * @param[in] param: VOID_T(*func)(VOID_T)
 *
 * @return VOID
 */
extern UINT32_T tkl_ble_trxfifo_not_empty(VOID_T);

/**
 * @brief To set system only support suspend mode, will not enter deep sleep. this priority is lower than tkl_board_pm_working
 *        only all the event bit be cleared, system can enter deep sleep. It usually be used operation which need cooperated hardware, like GPIO
 * @param[in] param:
 *              state: TKL_PM_CLEAR: clear the bit of event
 *                     TKL_PM_SET: set the bit of event
 *              event: event type, 0 - 4 are defined by board, others for user. event range: [0 - 31]
 * @return VOID
 */
extern VOID_T tkl_board_pm_suspend(TKL_PM_STATE_E state, TKL_PM_EVENT_E event);

/**
 * @brief To set system do not enter any type of sleep mode. this priority is higher than tkl_board_pm_suspend
 *        only all the event bit be cleared, system can enter sleep
 * @param[in] param:
 *              state: TKL_PM_CLEAR: clear the bit of event
 *                     TKL_PM_SET: set the bit of event
 *              event: event type, 0 - 4 are defined by board, others for user. event range: [0 - 31]
 * @return VOID
 */
extern VOID_T tkl_board_pm_working(TKL_PM_STATE_E state, TKL_PM_EVENT_E event);



#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
