/**
 * @file tkl_i2c.c
 * @brief This is tkl_i2c file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "string.h"
#include "i2c.h"
#include "board.h"

#include "tkl_i2c.h"
#include "tkl_memory.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TKL_I2C_SCL_GPIO 16
#define TKL_I2C_SDA_GPIO 15

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC TUYA_IIC_BASE_CFG_T tkl_i2c_cfg = {0};
STATIC UINT8_T tkl_i2c_io_group_index = 0;
STATIC I2C_GPIO_GroupTypeDef tlsr_i2c_gpio_group_list[] = {
    I2C_GPIO_GROUP_C2C3, I2C_GPIO_GROUP_C0C1, I2C_GPIO_GROUP_B6D7, I2C_GPIO_GROUP_A3A4
};

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC UINT32_T tkl_i2c_freq_transform(UINT32_T freq)
{
    switch (freq) {
        case 100000:    {return 100000; }
        case 400000:    {return 400000; }
        case 1000000:   {return 1000000; }
        case 3400000:   {return 3400000; }
        default:        {return -1; }
    }
}

STATIC OPERATE_RET ty_i2c_send(UINT32_T addr, UINT8_T addr_len, UINT8_T *buf, UINT8_T size)
{
    OPERATE_RET ret = OPRT_OK;

    //telink 82xx slave mapping mode no need send any address
    if (addr_len == 0) {
        //lanuch start /id    start
        reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START);
    } else if (addr_len == 1) {
        //address
        reg_i2c_adr = (UINT8_T)addr;
        //lanuch start /id/04    start
        reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_START);
    }
    else if (addr_len == 2) {
        //address high
        reg_i2c_adr = (UINT8_T)(addr>>8);
        //address low
        reg_i2c_do  = (UINT8_T)addr;
        //lanuch start /id/04/05    start
        reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_START);
    }
    else if (addr_len == 3) {
        //address high
        reg_i2c_adr = (UINT8_T)(addr>>16);
        //address middle
        reg_i2c_do  = (UINT8_T)(addr>>8);
        //address low
        reg_i2c_di  = (UINT8_T)(addr);
        //lanuch start /id/04/05/06    start
        reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_DI | FLD_I2C_CMD_START);
    }
    while (reg_i2c_status & FLD_I2C_CMD_BUSY);
    if (reg_i2c_status & FLD_I2C_NAK) {
        ret = OPRT_TIMEOUT;
    }

    if (ret == OPRT_OK) {
        if (buf) {
            //write data
            UINT8_T i = 0;
            for (i = 0; i < size; i++) {
                reg_i2c_di   = buf[i];
                //launch data read cycle
                reg_i2c_ctrl = FLD_I2C_CMD_DI;
                while (reg_i2c_status & FLD_I2C_CMD_BUSY);
                if (reg_i2c_status & FLD_I2C_NAK) {
                    ret = OPRT_TIMEOUT;
                }
            }
        }
    }

    //stop
    reg_i2c_ctrl = FLD_I2C_CMD_STOP;
    while (reg_i2c_status & FLD_I2C_CMD_BUSY);
    return ret;
}

STATIC OPERATE_RET ty_i2c_read(UINT32_T addr, UINT8_T addr_len, UINT8_T *buf, UINT8_T size)
{
    OPERATE_RET ret = OPRT_OK;

    //start + id(Read)
    reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START);
    while (reg_i2c_status & FLD_I2C_CMD_BUSY);
    if (reg_i2c_status & FLD_I2C_NAK) {
        ret = OPRT_TIMEOUT;
    }

    if (size > 0) {
        if (ret == OPRT_OK) {
            //read data
            UINT8_T index = 0;

            //the length of reading data must larger than 0
            size--;

            //if not the last byte master read slave, master wACK to slave
            while (size) {
                reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID);
                while (reg_i2c_status & FLD_I2C_CMD_BUSY);
                buf[index] = reg_i2c_di;
                index++;
                size--;
            }

            //when the last byte, master will ACK to slave
            reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID | FLD_I2C_CMD_ACK);
            while (reg_i2c_status & FLD_I2C_CMD_BUSY);
            buf[index] = reg_i2c_di;
        }

    }

    //termiante
    reg_i2c_ctrl = FLD_I2C_CMD_STOP;
    while (reg_i2c_status & FLD_I2C_CMD_BUSY);
    return ret;
}

OPERATE_RET tkl_i2c_init(TUYA_I2C_NUM_E port, CONST TUYA_IIC_BASE_CFG_T *cfg)
{
    OPERATE_RET ret = OPRT_OK;
    I2C_GPIO_GroupTypeDef tlsr_i2c_gpio_group;

    if (port != TUYA_I2C_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    UINT32_T baudrate = 0;
    switch (cfg->speed) {
        case TUYA_IIC_BUS_SPEED_100K:
            baudrate = 100000;
        break;
        case TUYA_IIC_BUS_SPEED_400K:
            baudrate = 400000;
        break;
        case TUYA_IIC_BUS_SPEED_1M:
            baudrate = 1000000;
        break;
        case TUYA_IIC_BUS_SPEED_3_4M:
            baudrate = 3400000;
        break;
    }

    if (tkl_i2c_freq_transform(baudrate) == (UINT32_T)-1) {
        return OPRT_NOT_SUPPORTED;
    }

    memcpy(&tkl_i2c_cfg, cfg, SIZEOF(TUYA_IIC_BASE_CFG_T));
        
    tlsr_i2c_gpio_group = tlsr_i2c_gpio_group_list[tkl_i2c_io_group_index];
    i2c_gpio_set(tlsr_i2c_gpio_group);
    i2c_master_init(0, (UINT8_T)(CLOCK_SYS_CLOCK_HZ/(4*baudrate)));

    // PCx will toggle (pulse width is around 60ns) when wakeup from deepsleep
    if ((tlsr_i2c_gpio_group == I2C_GPIO_GROUP_C2C3) || (tlsr_i2c_gpio_group == I2C_GPIO_GROUP_C0C1)) {
        reg_i2c_ctrl = FLD_I2C_CMD_START;
        while (reg_i2c_status & FLD_I2C_CMD_BUSY);

        reg_i2c_ctrl = FLD_I2C_CMD_STOP;
        while (reg_i2c_status & FLD_I2C_CMD_BUSY);
    }
    return ret;
}

OPERATE_RET tkl_i2c_deinit(TUYA_I2C_NUM_E port)
{
    if (port != TUYA_I2C_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }
    reset_i2c_moudle();

    return OPRT_OK;
}

OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port, CONST TUYA_I2C_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, UINT16_T dev_addr, CONST VOID_T *data, UINT32_T size, BOOL_T xfer_pending)
{
    OPERATE_RET ret = OPRT_OK;

    if (port != TUYA_I2C_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (data == NULL) {
        return TUYA_BLE_ERR_NO_MEM;
    }

    i2c_slave_addr_init((dev_addr<<1) | 0);

    if (size) {
        UINT8_T* i2c_data = (UINT8_T*)data;
        UINT32_T addr = i2c_data[0];
        ret = ty_i2c_send(addr, 0x01, &i2c_data[1], size-1);
    } else {
        ret = ty_i2c_send(0x00, 0x00, NULL, 0);
    }

    return ret;
}

OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, UINT16_T dev_addr, VOID *data, UINT32_T size, BOOL_T xfer_pending)
{
    OPERATE_RET ret = OPRT_OK;

    if (port != TUYA_I2C_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (data == NULL) {
        return TUYA_BLE_ERR_NO_MEM;
    }

    i2c_slave_addr_init((dev_addr<<1) | 1);

    if (size) {
        UINT8_T* i2c_data = (UINT8_T*)data;
        UINT32_T addr = i2c_data[0];
        ret = ty_i2c_read(addr, 0x01, &i2c_data[1], size-1);
    } else {
        ret = ty_i2c_read(0x00, 0x00, NULL, 0);
    }

    return ret;
}

OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, UINT16_T dev_addr)
{
    i2c_slave_addr_init((dev_addr<<1) | 1);
    return OPRT_OK;
}

OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, CONST VOID *data, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, VOID *data, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_reset(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_i2c_ioctl(TUYA_I2C_NUM_E port, UINT32_T cmd,  VOID *args)
{
    if (port != TUYA_I2C_NUM_0) {
        return OPRT_INVALID_PARM;
    }

    UINT8_T *i2c_index = (UINT8_T *)args;
    if(cmd == 0) {
        if((*i2c_index) >= (SIZEOF(tlsr_i2c_gpio_group_list) / SIZEOF(tlsr_i2c_gpio_group_list[0]))){
            return OPRT_INVALID_PARM;
        }

        tkl_i2c_io_group_index = *i2c_index;
    } else {
        return OPRT_INVALID_PARM;
    }

    return OPRT_OK;
}
