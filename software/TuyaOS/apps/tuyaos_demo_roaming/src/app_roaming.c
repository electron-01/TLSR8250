/**
 * @file app_roaming.c
 * @brief This is app_roaming file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "app_roaming.h"

#include "tal_log.h"
#include "tal_ble_ccm.h"
#include "tal_sw_timer.h"
#include "tal_util.h"
#include "tal_flash.h"
#include "tal_oled.h"

#include "tuya_ble_type.h"
#include "tuya_ble_port.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_main.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_sdk_callback.h"
#include "tuya_ble_protocol_callback.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
uint32_t adv_frame_counter = 0;

roaming_param_t g_roaming_param = {
    .adv_duration = 2000,
    .adv_interval = 1000,
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/




void app_roaming_init(void)
{
    app_roaming_sn_read();
    app_roaming_adv_interval_read();
}

void app_roaming_sn_write(void)
{
    tal_flash_erase(ADV_FRAME_COUNTER_ADDR, 0x1000);
    tal_flash_write(ADV_FRAME_COUNTER_ADDR, (void*)&adv_frame_counter, sizeof(uint32_t));
}

void app_roaming_sn_read(void)
{
    tal_flash_read(ADV_FRAME_COUNTER_ADDR, (void*)&adv_frame_counter, sizeof(uint32_t));
    adv_frame_counter += 256;

    app_roaming_sn_write();
}

void app_roaming_sn_clear(void)
{
    adv_frame_counter = 0;
    app_roaming_sn_write();
    tal_oled_show_string(120, 2, (VOID_T*)"-", 16);
}

void app_roaming_adv_interval_write(void)
{
    tal_flash_erase(ADV_INTERVAL_ADDR, 0x1000);
    tal_flash_write(ADV_INTERVAL_ADDR, (void*)&g_roaming_param.adv_interval, sizeof(uint32_t));
}

void app_roaming_adv_interval_read(void)
{
    tal_flash_read(ADV_INTERVAL_ADDR, (void*)&g_roaming_param.adv_interval, sizeof(uint32_t));
    if (g_roaming_param.adv_interval > 10240 || g_roaming_param.adv_interval < 20) {
        g_roaming_param.adv_interval = 1000;
    }
}

