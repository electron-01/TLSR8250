/**
 * @file tkl_spi.c
 * @brief This is tkl_spi file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "board.h"

#include "tkl_spi.h"
#include "tkl_memory.h"
#include "tal_spi.h"
#include "tal_memory.h"
#include "tal_sdk_test.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT32_T cs;
    UINT32_T clk;
    UINT32_T sdi;
    UINT32_T sdo;
} TKL_SPI_GPIO_T;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
STATIC UINT32_T cs_tlsr_pin = 0;
STATIC UINT8_T  spi_io_group_index = 0;

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC OPERATE_RET tkl_spi_mode_transform(TUYA_SPI_MODE_E mode, SPI_ModeTypeDef *tlsr825x_spi_mode)
{
    if (tlsr825x_spi_mode == NULL) {
        return OPRT_INVALID_PARM;
    }

    switch (mode) {
        case TUYA_SPI_MODE0:
            *tlsr825x_spi_mode = SPI_MODE0;
        break;
        case TUYA_SPI_MODE1:
            *tlsr825x_spi_mode = SPI_MODE1;
        break;
        case TUYA_SPI_MODE2:
            *tlsr825x_spi_mode = SPI_MODE2;
        break;
        case TUYA_SPI_MODE3:
            *tlsr825x_spi_mode = SPI_MODE3;
        break;
        default:
            *tlsr825x_spi_mode = SPI_MODE0;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_BASE_CFG_T *cfg)
{
    if (port != TUYA_SPI_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (cfg->databits != TUYA_SPI_DATA_BIT8) {
        return OPRT_NOT_SUPPORTED;
    }

    if (cfg->type != TUYA_SPI_AUTO_TYPE) {
        return OPRT_NOT_SUPPORTED;
    }

    SPI_ModeTypeDef mode;
    OPERATE_RET ret = OPRT_OK;
    ret = tkl_spi_mode_transform(cfg->mode, &mode);
    if (ret == OPRT_OK) {
        spi_master_init((unsigned char)(CLOCK_SYS_CLOCK_HZ/(2*cfg->freq_hz)-1), mode);
        if(spi_io_group_index == 0) {
            spi_master_gpio_set(SPI_GPIO_GROUP_A2A3A4D6);
            cs_tlsr_pin = GPIO_PD6;
        } else if(spi_io_group_index == 1) {
            spi_master_gpio_set(SPI_GPIO_GROUP_B6B7D2D7);
            cs_tlsr_pin = GPIO_PD2;
        } else {
            ret = OPRT_COM_ERROR;
        }
    }

    return ret;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    reset_spi_moudle();
    return OPRT_OK;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    if (port != TUYA_SPI_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (data == NULL) {
        return OPRT_NOT_SUPPORTED;
    }

    spi_write(NULL, 0, data, size, cs_tlsr_pin);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    if (port != TUYA_SPI_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (data == NULL) {
        return OPRT_NOT_SUPPORTED;
    }

    spi_read(NULL, 0, data, size, cs_tlsr_pin);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, VOID_T* send_buf, VOID_T* receive_buf, UINT32_T length)
{
    if ((port != TUYA_SPI_NUM_0)) {
        return OPRT_NOT_SUPPORTED;
    }

    if (send_buf == NULL) {
        return OPRT_NOT_SUPPORTED;
    }

    if (receive_buf == NULL) {
        return OPRT_NOT_SUPPORTED;
    }
    
    extern void spi_xfer(unsigned char *txCmd, int CmdLen, unsigned char *txData, unsigned char *rxData, int DataLen, GPIO_PinTypeDef CSPin);
    spi_xfer(NULL, 0, send_buf, receive_buf, length, cs_tlsr_pin);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer_with_length(TUYA_SPI_NUM_E port, VOID_T* send_buf, UINT32_T send_len, VOID_T* recv_buf, UINT32_T recv_len)
{
    UINT32_T i = 0;
    UINT8_T temp = 0;

    if (port != TUYA_SPI_NUM_0) {
        return OPRT_NOT_SUPPORTED;
    }
    if (send_buf == NULL) {
        return OPRT_NOT_SUPPORTED;
    }
    if(send_len == 0) {
        return OPRT_NOT_SUPPORTED;
    }
    if (recv_buf == NULL) {
        return OPRT_NOT_SUPPORTED;
    }
    if(recv_len == 0) {
        return OPRT_NOT_SUPPORTED;
    }

    gpio_write(cs_tlsr_pin,0);//CS level is low

    /***write Data***/
    reg_spi_ctrl &= ~FLD_SPI_DATA_OUT_DIS; //0x09- bit2 enables spi data output
    reg_spi_ctrl &= ~FLD_SPI_RD; //enable write,0x09-bit3 : 0 for read ,1 for write
    for (i = 0; i < send_len; i++) {
    	reg_spi_data = ((UINT8_T *)send_buf)[i];
        while(reg_spi_ctrl & FLD_SPI_BUSY); //wait writing finished
    }

    /***read data***/
    reg_spi_ctrl |= FLD_SPI_DATA_OUT_DIS;
    /***when the read_bit was set 1,you can read 0x800008 to take eight clock cycle***/
    reg_spi_ctrl |= FLD_SPI_RD; //enable read,0x09-bit3 : 0 for read ,1 for write
    temp = reg_spi_data; //first byte isn't useful data,only take 8 clock cycle
    while(reg_spi_ctrl &FLD_SPI_BUSY ); //wait reading finished
    for (i = 0; i < recv_len; i++) {
        if(i == (recv_len - 1)){
            reg_spi_ctrl &= ~FLD_SPI_RD;
        }
        ((UINT8_T *)recv_buf)[i] = reg_spi_data; //take 8 clock cycles
        while(reg_spi_ctrl & FLD_SPI_BUSY ); //wait reading finished
    }

    /***pull up CS***/
    gpio_write(cs_tlsr_pin,1);//CS level is high
    return OPRT_OK;
}

OPERATE_RET tkl_spi_ioctl(TUYA_SPI_NUM_E port, UINT32_T cmd, VOID *arg)
{
    if (port != TUYA_SPI_NUM_0) {
        return OPRT_INVALID_PARM;
    }

    UINT8_T *spi_index = (UINT8_T *)arg;
    if(cmd == 0) {
        if((*spi_index) >= 2){
            return OPRT_INVALID_PARM;
        }

        spi_io_group_index = *spi_index;
    } else {
        return OPRT_INVALID_PARM;
    }
    return OPRT_OK;
}

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

#if defined(TUYA_SDK_TEST) && (TUYA_SDK_TEST==1)

OPERATE_RET test_group_spi(UINT8_T cmd, UINT8_T *cmd_data, UINT32_T cmd_data_len, UINT8_T *p_rsp_data)
{
    OPERATE_RET ret  = 0;
    UINT32_T    idx  = 0;
    UINT8_T     *rsp = p_rsp_data;

    switch (cmd) {
        case TEST_CID_TX_SPI_DATA: {
            UINT32_T channel        = (cmd_data[0] << 24) | (cmd_data[1] << 16) | (cmd_data[2] << 8) | cmd_data[3];
            UINT32_T frequency      = (cmd_data[4] << 24) | (cmd_data[5] << 16) | (cmd_data[6] << 8) | cmd_data[7];
            UINT8_T  *spi_data      = cmd_data + 8;
            UINT32_T spi_data_len   = cmd_data_len - 8;

            tal_spi_deinit(channel);

            TUYA_SPI_BASE_CFG_T spi_cfg = {
                .mode = TUYA_SPI_MODE0,
                .type = TUYA_SPI_AUTO_TYPE,
                .databits = TUYA_SPI_DATA_BIT8,
                .freq_hz = frequency
            };
            ret = tal_spi_init(channel, &spi_cfg);
            if (ret == OPRT_OK) {
                UINT8_T *buf = tal_malloc(spi_data_len);
                if (buf) {
                    ret = tal_spi_xfer(channel, spi_data, buf, spi_data_len);
                    if (ret == OPRT_OK) {
                        if (memcmp(spi_data, buf, spi_data_len) == 0) {
                            test_cmd_send(TEST_ID_GET(TEST_GID_SPI, TEST_CID_RX_SPI_DATA), buf, spi_data_len);
                        } else {
                            ret = OPRT_RECV_DA_NOT_ENOUGH;
                        }
                    }

                    tal_free(buf);
                } else {
                    ret = OPRT_MALLOC_FAILED;
                }
            }

            rsp[idx++] = (ret >> 24) & 0xFF;
            rsp[idx++] = (ret >> 16) & 0xFF;
            rsp[idx++] = (ret >> 8) & 0xFF;
            rsp[idx++] = (ret) & 0xFF;
        } break;

        case TEST_CID_RX_SPI_DATA: {
        } break;

        default: {
        } break;
    }

    return idx;
}

#endif
