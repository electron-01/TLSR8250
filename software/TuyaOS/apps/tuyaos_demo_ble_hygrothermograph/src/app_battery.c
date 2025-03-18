/**
 * @file app_battery.c
 * @brief This is app_battery file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "board.h"

#include "tal_log.h"
#include "tal_sw_timer.h"
#include "tal_i2c.h"
#include "tal_rtc.h"
#include "tal_util.h"
#include "tal_sw_timer.h"

#include "tuya_ble_api.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_protocol_callback.h"

#if ENABLE_LED
#include "app_led.h"
#endif
#if ENABLE_KEY
#include "app_key.h"
#endif
#include "app_misc.h"
#include "app_sensor.h"
#include "app_dp_reporter.h"
#include "app_battery.h"

#if ENABLE_BATTERY

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define APP_BATTERY_ADC_TYPE                 TUYA_ADC_INNER_SAMPLE_VOL
#define APP_BATTERY_ADC_CH                   (1<<6)

#define APP_BATTERY_VBAT_PER_CHANGE_MAX     (20)

#define APP_BATTERY_VBAT_GET_PERIOD         (1800)
#define APP_BATTERY_REPORT_PERIOD_HIGHPOWER (4 * 3600)
#define APP_BATTERY_REPORT_PERIOD_LOWPOWER  (1 * 3600)
#define APP_BATTERY_POWER_ON_TIMER          (10)

#define APP_BATTERY_BUFFER_SIZE             (8)

typedef enum {
    APP_BATTERY_POWER_ON = 0,
    APP_BATTERY_HIGHPOWER,
    APP_BATTERY_LOWPOWER,
} APP_BATTERY_WORK_MODE_E;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    APP_BATTERY_WORK_MODE_E  mode;
    UINT8_T  vbat_percent;
    UINT16_T vbat_mv;
    UINT16_T time_pass;
    UINT16_T update_time;
} APP_BATTERY_INFO_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC APP_BATTERY_INFO_T sg_app_battery_info;
STATIC UINT8_T  sg_app_battery_buffer_index;
STATIC UINT16_T sg_app_battery_value_buf[APP_BATTERY_BUFFER_SIZE];
STATIC UINT8_T  sg_vbat_percent_last;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC VOID_T app_battery_info_poll(VOID_T)
{
    UINT16_T vbat_mv_max = 0;
    UINT8_T  temp = 0;

    for (UINT8_T i = 0; i < APP_BATTERY_BUFFER_SIZE; i++) {
        if (vbat_mv_max < sg_app_battery_value_buf[i]) {
            vbat_mv_max = sg_app_battery_value_buf[i];
        }
    }

    temp = 100 * (vbat_mv_max - APP_BATTERY_VBAT_MV_MIN) / (APP_BATTERY_VBAT_MV_MAX - APP_BATTERY_VBAT_MV_MIN);
    if (sg_vbat_percent_last > APP_BATTERY_VBAT_PER_CHANGE_MAX) {
        if (temp > (sg_vbat_percent_last + APP_BATTERY_VBAT_PER_CHANGE_MAX)) {
            temp = sg_vbat_percent_last + APP_BATTERY_VBAT_PER_CHANGE_MAX;
        } else if (temp < (sg_vbat_percent_last - APP_BATTERY_VBAT_PER_CHANGE_MAX)) {
            temp = sg_vbat_percent_last - APP_BATTERY_VBAT_PER_CHANGE_MAX;
        } else {
            temp = temp;
        }
    } else {
        if (temp > sg_vbat_percent_last + APP_BATTERY_VBAT_PER_CHANGE_MAX) {
            temp = sg_vbat_percent_last + APP_BATTERY_VBAT_PER_CHANGE_MAX;
        } else {
            temp = temp;
        }
    }

    sg_app_battery_info.vbat_mv      = vbat_mv_max;
    sg_app_battery_info.vbat_percent = temp;
    sg_vbat_percent_last             = sg_app_battery_info.vbat_percent;

    app_dp_reporter_event_set(APP_REPORT_BATTERY);
    TAL_PR_DEBUG("VBAT POLL: %d %d", sg_app_battery_info.vbat_mv, sg_app_battery_info.vbat_percent);
}

STATIC OPERATE_RET app_battery_vbat_power_on(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;
    INT32_T value = 0;
    TUYA_ADC_BASE_CFG_T adc_cfg = {
        .ch_list.data = APP_BATTERY_ADC_CH,
        .width = 12,
        .type = APP_BATTERY_ADC_TYPE,
    };

    ret = tal_adc_init(TUYA_ADC_NUM_0, &adc_cfg);
    if (ret) {
        return ret;
    }

    UINT16_T vbat_mv_max = 0;
    for (UINT8_T i = 0; i < APP_BATTERY_BUFFER_SIZE; i++) {
        tal_adc_read_voltage(TUYA_ADC_NUM_0, &value, 1);
        if (value > APP_BATTERY_VBAT_MV_MAX) {
            value = APP_BATTERY_VBAT_MV_MAX;
        }
        if (value < APP_BATTERY_VBAT_MV_MIN) {
            value = APP_BATTERY_VBAT_MV_MIN;
        }
        sg_app_battery_value_buf[sg_app_battery_buffer_index] = (UINT16_T)value;
        sg_app_battery_buffer_index = (sg_app_battery_buffer_index + 1) % APP_BATTERY_BUFFER_SIZE;
        if (vbat_mv_max < value) {
            vbat_mv_max = value;
        }
        tal_system_delay(1);
    }
    ret = tal_adc_deinit(TUYA_ADC_NUM_0);

    sg_app_battery_info.vbat_mv      = vbat_mv_max;
    sg_app_battery_info.vbat_percent = 100 * (vbat_mv_max - APP_BATTERY_VBAT_MV_MIN) / (APP_BATTERY_VBAT_MV_MAX - APP_BATTERY_VBAT_MV_MIN);
    TAL_PR_DEBUG("VBAT POWER ON: %d %d", sg_app_battery_info.vbat_mv, sg_app_battery_info.vbat_percent);
    sg_vbat_percent_last = sg_app_battery_info.vbat_percent;
    return ret;
}

STATIC OPERATE_RET app_battery_data_update(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;
    INT32_T value = 0;

    TUYA_ADC_BASE_CFG_T adc_cfg = {
        .ch_list.data = APP_BATTERY_ADC_CH,
        .width = 12,
        .type = APP_BATTERY_ADC_TYPE,
    };
    ret = tal_adc_init(TUYA_ADC_NUM_0, &adc_cfg);
    if (ret) {
        return ret;
    }

    ret = tal_adc_read_voltage(TUYA_ADC_NUM_0, &value, 1);
    tal_adc_deinit(TUYA_ADC_NUM_0);

    if (value > APP_BATTERY_VBAT_MV_MAX) {
        value = APP_BATTERY_VBAT_MV_MAX;
    }
    if (value < APP_BATTERY_VBAT_MV_MIN) {
        value = APP_BATTERY_VBAT_MV_MIN;
    }
    sg_app_battery_value_buf[sg_app_battery_buffer_index] = (UINT16_T)value;
    sg_app_battery_buffer_index = (sg_app_battery_buffer_index + 1) % APP_BATTERY_BUFFER_SIZE;
    return ret;
}

VOID_T app_battery_process(VOID_T)
{
    sg_app_battery_info.time_pass += APP_SENSOR_RUNNIGN_CYCLE;

    if (sg_app_battery_info.mode == APP_BATTERY_POWER_ON) {
        if (sg_app_battery_info.time_pass >= APP_BATTERY_POWER_ON_TIMER) {
            sg_app_battery_info.time_pass = 0;

            app_battery_vbat_power_on();
            app_battery_info_poll();

            if (sg_app_battery_info.vbat_percent < APP_BATTERY_LOWPOWER_PERCENT) {
                sg_app_battery_info.mode = APP_BATTERY_LOWPOWER;
            } else {
                sg_app_battery_info.mode = APP_BATTERY_HIGHPOWER;
            }
        }
    } else {
        sg_app_battery_info.update_time += APP_SENSOR_RUNNIGN_CYCLE;
        if (sg_app_battery_info.update_time >= APP_BATTERY_VBAT_GET_PERIOD) {
            sg_app_battery_info.update_time = 0;
            app_battery_data_update();
        }

        if (sg_app_battery_info.vbat_percent < APP_BATTERY_LOWPOWER_PERCENT) {
            sg_app_battery_info.mode = APP_BATTERY_LOWPOWER;
        }

        UINT32_T timeout_tick = 0;
        if (sg_app_battery_info.mode == APP_BATTERY_HIGHPOWER) {
            timeout_tick = APP_BATTERY_REPORT_PERIOD_HIGHPOWER;
        } else {
            timeout_tick = APP_BATTERY_REPORT_PERIOD_LOWPOWER;
        }

        if (sg_app_battery_info.time_pass >= timeout_tick) {
            sg_app_battery_info.time_pass = 0;

            app_battery_info_poll();
        }
    }
}

OPERATE_RET app_battery_vbat_percent_get(UINT8_T *value)
{
    if (value) {
        *value = sg_app_battery_info.vbat_percent;
        return OPRT_OK;
    }
    return OPRT_INVALID_PARM;
}

OPERATE_RET app_battery_vbat_value_get(UINT16_T *value)
{
    if (value) {
        *value = sg_app_battery_info.vbat_mv;
        return OPRT_OK;
    }
    return OPRT_INVALID_PARM;
}

OPERATE_RET app_battery_hw_init(VOID_T)
{
    return OPRT_OK;
}

OPERATE_RET app_battery_sw_init(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;
    sg_app_battery_info.mode         = APP_BATTERY_POWER_ON;
    sg_app_battery_info.vbat_percent = 100;
    sg_app_battery_info.vbat_mv      = APP_BATTERY_VBAT_MV_MAX;
    sg_app_battery_info.time_pass    = 0;
    sg_app_battery_buffer_index      = 0;
    memset(sg_app_battery_value_buf, 0, sizeof(sg_app_battery_value_buf));
    sg_vbat_percent_last = 100;
    return ret;
}

#endif
