/**
 * @file tkl_system.c
 * @brief This is tkl_system file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "tl_common.h"
#include "stack/ble/ble.h"
#include "board.h"
#include "drivers.h"

#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_gpio.h"
#include "tal_sw_timer.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define NUMBER_OF_PINS          23

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef enum {
    ETIMER_IDLE = 0,
    ETIMER_RUNNING,
} TIMER_STATUS_E;

typedef struct __ETIMER {
    struct __ETIMER *next;
    VOID_T          *cb_arg;
    TAL_TIMER_CB    callback;
    UINT32_T        period_time;
    UINT32_T        expired_time;
    TIMER_TYPE      type;
    TIMER_STATUS_E  status;
} __ETIMER_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern TUYA_WAKEUP_SOURCE_BASE_CFG_T *g_wakeup_cfg;
STATIC TUYA_RESET_REASON_E boot_type;
STATIC UINT8_T soft_reset_flag = 0;
STATIC __ETIMER_T *timer_list;
volatile SYS_TIME_T tkl_system_millisecond = 0;
STATIC SYS_TIME_T tkl_system_tick_last = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
// TKL
extern OPERATE_RET tal_init_first(VOID_T);
extern OPERATE_RET tal_init_second(VOID_T);
extern OPERATE_RET tal_init_third(VOID_T);
extern OPERATE_RET tal_init_last(VOID_T);
extern OPERATE_RET tal_main_loop(VOID_T);
extern VOID_T tuya_ble_pm_module_init(VOID_T);

// C STDLIB FUNCTION
char	*__ctype_ptr__;
STATIC INLINE INT32_T isdigit(INT32_T c) {
    if (c >= '0' && c <= '9') {
        return 1;
    } else {
        return 0;
    }
}

STATIC INLINE INT32_T isspace(INT32_T c) {  
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
        return 1;
    } else {
        return 0; 
    }
} 

int atoi(const char* str) {  
    int sign = 1, result = 0;  
    int i = 0;  
    // 跳过空格字符  
    while (isspace(str[i])) {  
        i++;  
    }  
    // 处理正负号  
    if (str[i] == '-') {  
        sign = -1;  
        i++;  
    } else if (str[i] == '+') {  
        i++;  
    }  
    // 转换数字字符为整数  
    while (isdigit(str[i])) {  
        result = result * 10 + (str[i] - '0');  
        i++;  
    }  
    return sign * result;  
}

unsigned int strspn(const char *s, const char *accept) {  
    unsigned int spn = 0;  
    const char *scan = s;  
      
    while (*scan != '\0') {  
        if (strchr(accept, *scan) != NULL) {  
            spn++;  
        } else {  
            break;  
        }  
        scan++;  
    }  
      
    return spn;  
}

unsigned int strcspn(const char *s1, const char *s2) {  
    unsigned int len1 = 0, len2 = 0;  
    const char *p1 = s1, *p2 = s2;  
  
    while (memcmp(p1, p2, 1) == 0) {  
        if (++len2 == strlen(s2)) {  
            // 如果已经检查完了s2中的所有字符，并且没有找到不匹配的，那么返回s1的长度  
            return strlen(s1);  
        }  
        ++p2;  
    }  
  
    // 找到了不匹配的字符，返回它在s1中的位置  
    return len1 + (p1 - s1);  
}  

float atof(const char* str) {  
    float result = 0.0;  
    int sign = 1;  
  
    // skip leading whitespace  
    while (isspace(*str)) {  
        str++;  
    }  
  
    // check for sign  
    if (*str == '-') {  
        sign = -1;  
        str++;  
    } else if (*str == '+') {  
        str++;  
    }  
  
    // process digits  
    while (isdigit(*str)) {  
        result = result * 10.0 + (*str - '0');  
        str++;  
    }  
  
    // process decimal point and fractions  
    if (*str == '.') {  
        str++;  
        float decimal = 0.0;  
        int divisor = 1;  
        while (isdigit(*str)) {  
            decimal = decimal * 10.0 + (*str - '0');  
            divisor *= 10;  
            str++;  
        }  
        result += decimal / divisor;  
    }  
  
    // apply sign  
    result *= sign;  
  
    return result;  
}

int sscanf(const char *str, const char *format, ...)  
{  
    int count = 0;  
    va_list args;  
    va_start(args, format);  
  
    while (*format != '\0') {  
        if (*str == '\0') {  
            break;  
        }  
        if (*format == '%') {  
            format++;  
            switch (*format) {  
                case 'd': {  
                    int *ip = va_arg(args, int*);  
                    *ip = atoi(str);  
                    count++;  
                    str += strspn(str, "0123456789");  
                    break;  
                }  
                case 'f': {  
                    double *fp = va_arg(args, double*);  
                    *fp = atof(str);  
                    count++;  
                    str += strspn(str, "0123456789.");  
                    break;  
                }  
                case 's': {  
                    char *s = va_arg(args, char*);  
                    strcpy(s, str);  
                    count++;  
                    str += strcspn(str, " \t\n\r");  
                    break;  
                }  
            }  
        } else {  
            if (*format != *str) {  
                break;  
            }  
            format++;  
            str++;  
        }  
    }  
    va_end(args);  
    return count;  
}

STATIC VOID_T tkl_boot_type_init(VOID_T)
{
    if (soft_reset_flag) {
        soft_reset_flag = 0;
        boot_type = TUYA_RESET_REASON_SOFTWARE;
        return;
    }

    UINT8_T start_source = (analog_read(0x44) & 0x0E);
    analog_write(0x44, analog_read(0x44) | 0x0f) ;  // clear analog
    if (start_source & BIT(1) || start_source & BIT(3)) {
        boot_type = TUYA_RESET_REASON_DEEPSLEEP;
    } else {
        boot_type = TUYA_RESET_REASON_POWERON;
    }
}

#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)

UINT32_T tkl_log_gpio;
STATIC VOID_T tkl_log_pin_init(VOID_T)
{
    tkl_log_gpio = tkl_gpio_to_tlsr_gpio(BOARD_LOG_TX_PIN);
    if (tkl_log_gpio == 0xFFFFFFFF) {
        return;
    }

    TUYA_GPIO_BASE_CFG_T tkl_log_gpio_cfg = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_LOW,
    };

    tkl_gpio_init(BOARD_LOG_TX_PIN, &tkl_log_gpio_cfg);

}

#endif

UINT32_T tkl_system_enter_critical(VOID_T)
{
    return irq_disable();
}

VOID_T tkl_system_exit_critical(UINT32_T irq_mask)
{
    irq_restore(irq_mask);
}

VOID_T tkl_system_reset(VOID_T)
{
    analog_write(DEEP_ANA_REG0, analog_read(DEEP_ANA_REG0) | FLD_SOFT_REBOOT_FLAG);
    start_reboot();
}

SYS_TICK_T tkl_system_get_tick_count(VOID_T)
{
    return clock_time();
}

SYS_TIME_T tkl_system_get_millisecond(VOID_T)
{
    SYS_TIME_T now_tick  = 0;
    SYS_TIME_T last_tick = 0;
    SYS_TIME_T pass_tick = 0;

    last_tick = tkl_system_tick_last;
    now_tick = clock_time();
    pass_tick = ((now_tick >= last_tick) ? (now_tick - last_tick) : (0xFFFFFFFF - last_tick + now_tick)) / CLOCK_16M_SYS_TIMER_CLK_1MS;
    if (pass_tick > 0) {
        tkl_system_millisecond = (tkl_system_millisecond + pass_tick) & 0x7FFFFFFFUL;
        tkl_system_tick_last = tkl_system_tick_last + (pass_tick * CLOCK_16M_SYS_TIMER_CLK_1MS);
    }

    return tkl_system_millisecond;
}

INT32_T tkl_system_get_random(UINT32_T range)
{
    return (rand() % range);
}

TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** describe)
{
    return boot_type;
}

VOID_T tkl_system_sleep(UINT32_T num_ms)
{
    sleep_ms(num_ms);
}

VOID_T tkl_system_delay(UINT32_T num_ms)
{
    sleep_ms(num_ms);
}

VOID_T tkl_system_log_output(CONST UINT8_T *buf, UINT32_T size)
{
#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)
    extern int uart_putc(char byte);
    for (UINT32_T idx=0; idx<size; idx++) {
        uart_putc(buf[idx]);
    }
#endif
}

OPERATE_RET tkl_init_first(VOID_T)
{
#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)
    tkl_log_pin_init();
#endif
    UINT8_T reg1 = analog_read(DEEP_ANA_REG0);
    if (reg1 & FLD_SOFT_REBOOT_FLAG) {
        analog_write(DEEP_ANA_REG0, analog_read(DEEP_ANA_REG0) & (~FLD_SOFT_REBOOT_FLAG));
        soft_reset_flag = 1;
    }

    tuya_ble_pm_module_init();
    return tal_init_first();
}

OPERATE_RET tkl_init_second(VOID_T)
{
    return tal_init_second();
}

OPERATE_RET tkl_init_third(VOID_T)
{
#if (TUYA_BLE_LOG_ENABLE || TUYA_APP_LOG_ENABLE)
    tkl_log_pin_init();
#endif
    tkl_boot_type_init();
    return tal_init_third();
}

OPERATE_RET tkl_init_last(VOID_T)
{
    tkl_system_tick_last = clock_time();
    TY_PRINTF("FIRMWARE VERSION: %s", FIRMWARE_VERSION);
    return tal_init_last();
}

OPERATE_RET tkl_main_loop(VOID_T)
{
    return tal_main_loop();
}

VOID_T tuya_etimer_loop_handler(VOID_T)
{
    extern VOID_T tal_sw_timer_loop_handler(VOID_T);
    tal_sw_timer_loop_handler();
}

