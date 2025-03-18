/**
 * @file tkl_bluetooth.c
 * @brief This is tkl_bluetooth file
 * @version 1.0
 * @date 2020-08-15
 *
 * @copyright Copyright 2020-2023 Tuya Inc. All Rights Reserved.
 *
 */

#include "drivers.h"
#include "stack/ble/ble.h"
#include "stack/ble/gap/gap_event.h"
#include "vendor/common/blt_common.h"

#include "board.h"

#include "tuya_ble_protocol_callback.h"
#include "tuya_ble_main.h"

#include "tal_log.h"
#include "tal_sw_timer.h"

#include "tkl_flash.h"
#include "tkl_memory.h"
#include "tkl_bluetooth.h"

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define     DEFAULT_ADV_DATA    \
            {                   \
                3,              \
                {               \
                    0x02,       \
                    0x01,       \
                    0x06,       \
                },              \
            }

#define     DEFAULT_SCAN_RSP    \
            {                   \
                13,             \
                {               \
                    0x0C,       \
                    0x09,       \
                    'T', 'u', 'y', 'a', 'O', 'S', '_', 'D', 'e', 'm', 'o', \
                },              \
            }

#define     DEFAULT_ADV_PARAM                \
            {                                \
                .adv_type         = 0x01,    \
                .direct_addr      = {0,{0}}, \
                .adv_interval_min = 50, \
                .adv_interval_max = 50, \
                .adv_channel_map  = 7,    \
            }

#define     DEFAULT_SCAN_PARAM              \
            {                               \
                .extended         = 0,      \
                .active           = 1,      \
                .scan_phys        = 1,      \
                .interval         = 160,    \
                .window           = 80,     \
                .timeout          = 0,   \
                .scan_channel_map = 7,      \
            }
#define MY_DEV_NAME                        "TuyaOS"

#define     TKL_BLE_CONN_CFG_TAG            (1)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    UINT32_T len;
    UINT8_T  value[31];
} tkl_ble_data_t;

typedef struct {
  UINT16_T intervalMin;
  UINT16_T intervalMax;
  UINT16_T latency;
  UINT16_T timeout;
} gap_periConnectParams_t;

typedef struct {
    UINT8_T uuid[16];
    UINT8_T prop;
    UINT8_T buf[1];
    UINT8_T CCC[2];
} tkl_character_t;

typedef struct {
    UINT8_T    initA[6];            //scanA
    UINT8_T    advA[6];            //
    UINT8_T    accessCode[4];        // access code
    UINT8_T    crcinit[3];
    UINT8_T    winSize;
    UINT16_T    winOffset;
    UINT16_T interval;
    UINT16_T latency;
    UINT16_T timeout;
    UINT8_T    chm[5];
    UINT8_T    hop;                //sca(3)_hop(5)
} packet_connect_t;
typedef struct {
    UINT16_T interval;
    UINT16_T latency;
    UINT16_T timeout;
} update_conn_para_t;

/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
//Adv
STATIC tkl_ble_data_t             tkl_adv_data          = DEFAULT_ADV_DATA;
STATIC tkl_ble_data_t             tkl_scan_rsp          = DEFAULT_SCAN_RSP;
STATIC TKL_BLE_GAP_ADV_PARAMS_T   tkl_adv_param         = DEFAULT_ADV_PARAM;

STATIC BOOL_T                     tkl_is_advertising    = FALSE;

STATIC TKL_BLE_GAP_EVT_FUNC_CB    tkl_ble_gap_evt_func  = NULL;
STATIC TKL_BLE_GATT_EVT_FUNC_CB   tkl_ble_gatt_evt_func = NULL;

STATIC UINT16_T                   s_cccd_handle         = 0xFFFF;

//UUID
STATIC       UINT16_T my_primaryServiceUUID     = GATT_UUID_PRIMARY_SERVICE;
STATIC CONST UINT16_T my_characterUUID          = GATT_UUID_CHARACTER;
STATIC CONST UINT16_T clientCharacterCfgUUID    = GATT_UUID_CLIENT_CHAR_CFG;

STATIC CONST UINT16_T my_gapServiceUUID         = SERVICE_UUID_GENERIC_ACCESS;
STATIC CONST UINT16_T my_gattServiceUUID        = SERVICE_UUID_GENERIC_ATTRIBUTE;

STATIC CONST UINT16_T my_devNameUUID            = GATT_UUID_DEVICE_NAME;
STATIC CONST UINT16_T my_appearanceUIID         = GATT_UUID_APPEARANCE;
STATIC CONST UINT16_T my_periConnParamUUID      = GATT_UUID_PERI_CONN_PARAM;
STATIC CONST UINT16_T serviceChangeUUID         = GATT_UUID_SERVICE_CHANGE;

//GAP attribute
extern  UINT8_T ble_devName[]; //tModule
STATIC CONST UINT16_T my_appearance = GAP_APPEARE_UNKNOWN;
STATIC CONST gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};
//GATT attribute
STATIC       UINT16_T serviceChangeVal[2] = {0};
STATIC       UINT8_T  serviceChangeCCC[2] = {0, 0};

//GAP attribute values
STATIC CONST UINT8_T my_devNameCharVal[5] = {
    CHAR_PROP_READ | CHAR_PROP_NOTIFY,
    U16_LO(GenericAccess_DeviceName_DP_H), U16_HI(GenericAccess_DeviceName_DP_H),
    U16_LO(GATT_UUID_DEVICE_NAME), U16_HI(GATT_UUID_DEVICE_NAME)
};
STATIC CONST UINT8_T my_appearanceCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_Appearance_DP_H), U16_HI(GenericAccess_Appearance_DP_H),
    U16_LO(GATT_UUID_APPEARANCE), U16_HI(GATT_UUID_APPEARANCE)
};
STATIC CONST UINT8_T my_periConnParamCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(CONN_PARAM_DP_H), U16_HI(CONN_PARAM_DP_H),
    U16_LO(GATT_UUID_PERI_CONN_PARAM), U16_HI(GATT_UUID_PERI_CONN_PARAM)
};
//GATT attribute values
STATIC CONST UINT8_T my_serviceChangeCharVal[5] = {
    CHAR_PROP_INDICATE,
    U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
    U16_LO(GATT_UUID_SERVICE_CHANGE), U16_HI(GATT_UUID_SERVICE_CHANGE)
};

//tuya
STATIC UINT8_T tkl_service_uuid[2]      = {0x50, 0xfd};
STATIC tkl_character_t tkl_character[3] = {
    {
        .uuid = {0xD0, 0x07, 0x9b, 0x5f, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00},
        .prop = CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RSP,
        .buf = {0},
    },
    {
        .uuid = {0xD0, 0x07, 0x9b, 0x5f, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
        .prop = CHAR_PROP_NOTIFY,
        .buf = {0},
        .CCC = {0, 0},
    },
    {
        .uuid = {0xD0, 0x07, 0x9b, 0x5f, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00},
        .prop = CHAR_PROP_READ,
        .buf = {0},
    },
};

STATIC OPERATE_RET tkl_module_on_receive_data(rf_packet_att_write_t *p);

STATIC attribute_t tkl_Attributes[] = {
    {ATT_END_H - 1, 0,0,0,0,0},    // total num of attribute

    // 0001 - 0007  gap
    {7,ATT_PERMISSIONS_READ,2,2,                              (UINT8_T*)(&my_primaryServiceUUID), (UINT8_T*)(&my_gapServiceUUID),      0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF(my_devNameCharVal),      (UINT8_T*)(&my_characterUUID),      (UINT8_T*)(my_devNameCharVal),       0},
    {0,ATT_PERMISSIONS_READ,2,MAX_DEV_NAME_LEN,               (UINT8_T*)(&my_devNameUUID),        (UINT8_T*)(ble_devName),             0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF(my_appearanceCharVal),   (UINT8_T*)(&my_characterUUID),      (UINT8_T*)(my_appearanceCharVal),    0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF (my_appearance),         (UINT8_T*)(&my_appearanceUIID),     (UINT8_T*)(&my_appearance),          0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF(my_periConnParamCharVal),(UINT8_T*)(&my_characterUUID),      (UINT8_T*)(my_periConnParamCharVal), 0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF (my_periConnParameters), (UINT8_T*)(&my_periConnParamUUID),  (UINT8_T*)(&my_periConnParameters),  0},

    // 0008 - 000b gatt
    {4,ATT_PERMISSIONS_READ,2,2,                              (UINT8_T*)(&my_primaryServiceUUID),  (UINT8_T*)(&my_gattServiceUUID),     0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF(my_serviceChangeCharVal),(UINT8_T*)(&my_characterUUID),       (UINT8_T*)(my_serviceChangeCharVal), 0},
    {0,ATT_PERMISSIONS_READ,2,SIZEOF (serviceChangeVal),      (UINT8_T*)(&serviceChangeUUID),      (UINT8_T*)(&serviceChangeVal),       0},
    {0,ATT_PERMISSIONS_RDWR,2,SIZEOF (serviceChangeCCC),      (UINT8_T*)(&clientCharacterCfgUUID), (UINT8_T*)(serviceChangeCCC),        0},

    // 000c - 0013 tuya
    {8,ATT_PERMISSIONS_READ,  2,  2,                            (UINT8_T*)(&my_primaryServiceUUID),  (UINT8_T*)&tkl_service_uuid,      0, 0},
    {0,ATT_PERMISSIONS_READ,  2,  1,                            (UINT8_T*)(&my_characterUUID),       &tkl_character[0].prop, 0},
    {0,ATT_PERMISSIONS_WRITE, 16, SIZEOF(tkl_character[0].buf), (UINT8_T*)(tkl_character[0].uuid),   (UINT8_T*)tkl_character[0].buf,   (att_readwrite_callback_t)&tkl_module_on_receive_data, 0},
    {0,ATT_PERMISSIONS_READ,  2,  1,                            (UINT8_T*)(&my_characterUUID),       &tkl_character[1].prop, 0},
    {0,ATT_PERMISSIONS_READ,  16, SIZEOF(tkl_character[1].buf), (UINT8_T*)(tkl_character[1].uuid),   tkl_character[1].buf,   0},
    {0,ATT_PERMISSIONS_RDWR,  2,  SIZEOF(tkl_character[1].CCC), (UINT8_T*)(&clientCharacterCfgUUID), tkl_character[1].CCC,   0},

    {0,ATT_PERMISSIONS_READ,  2,  1,                            (UINT8_T*)(&my_characterUUID),       &tkl_character[2].prop, 0},
    {0,ATT_PERMISSIONS_READ,  16, SIZEOF(tkl_character[2].buf), (UINT8_T*)(tkl_character[2].uuid),   tkl_character[2].buf,   0},
};

#define BLE_DEVICE_ADDRESS_TYPE         BLE_DEVICE_ADDRESS_RANDOM_STATIC
STATIC UINT8_T  mac_public[6];
STATIC UINT8_T  mac_type;
STATIC own_addr_type_t     app_own_address_type = OWN_ADDRESS_RANDOM;

#define TKL_BLE_DATA_LENGTH         MAX_OCTETS_DATA_LEN_EXTENSION
#define RX_FIFO_SIZE                ((TKL_BLE_DATA_LENGTH+24)/16+1)*16    //rx-24   max:251+24 = 275  16 align-> 288
#define RX_FIFO_NUM                 8
#define TX_FIFO_SIZE                ((TKL_BLE_DATA_LENGTH+12)/4+1)*4    //tx-12   max:251+12 = 263  4 align-> 264
#define TX_FIFO_NUM                 16

_attribute_no_ret_bss_ UINT8_T              blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_no_ret_data_    my_fifo_t    blt_rxfifo = {RX_FIFO_SIZE, RX_FIFO_NUM, 0, 0, blt_rxfifo_b};
_attribute_no_ret_bss_ UINT8_T              blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_no_ret_data_    my_fifo_t    blt_txfifo = {TX_FIFO_SIZE, TX_FIFO_NUM, 0, 0, blt_txfifo_b};

#define MTU_RX_MAX                  TKL_BLE_DATA_LENGTH-4
#define MTU_RX_BUFF_SIZE_MAX        ATT_ALLIGN4_DMA_BUFF(MTU_RX_MAX)
#define MTU_TX_BUFF_SIZE_MAX        ATT_ALLIGN4_DMA_BUFF(MTU_RX_MAX)

_attribute_data_retention_ UINT8_T  mtu_rx_fifo[MTU_RX_BUFF_SIZE_MAX] = {0};
_attribute_data_retention_ UINT8_T  mtu_tx_fifo[MTU_TX_BUFF_SIZE_MAX] = {0};

#if TKL_BLUETOOTH_SUPPORT_SCAN
// When BLE under combined state，it is close which is SCAN after excuted  the function of "blc_ll_removeScanningFromAdvState" and "blc_ll_removeScanningFromConnSLaveRole" , so do not need excute fuction blc_ll_setScanEnable
// And the function of "blc_ll_removeScanningFromAdvState" And "blc_ll_removeScanningFromConnSLaveRole" is used to stop scan under combo state.
// If BLE only scan state, use the function of "blc_ll_setScanEnable" to control scan on/off
// the same logic to open scan
STATIC TKL_BLE_GAP_SCAN_PARAMS_T  tkl_scan_param        = DEFAULT_SCAN_PARAM;
STATIC bool_t                     tkl_is_scanning       = FALSE;
STATIC UINT8_T                    tkl_only_scan         = 0;
#endif

TKL_BOARD_BLE_STATUS_E tkl_ble_state = TY_BLE_STA_IDLE;
STATIC OPERATE_RET (*tkl_conn_param_update_rsp_cb)(UINT8_T, UINT16_T);

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/
STATIC OPERATE_RET controller_event_handler(UINT32_T h, UINT8_T *para, INT32_T n)
{
    if ((h&HCI_FLAG_EVENT_TLK_MODULE) != 0) {

        UINT8_T event = (UINT8_T)(h&0xff);
        switch (event) {
            case BLT_EV_FLAG_SCAN_RSP: {
            } break;

            case BLT_EV_FLAG_CONNECT: {
                tkl_ble_state = TY_BLE_STA_CONN;
                blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, MTU_RX_MAX);
                blc_ll_exchangeDataLength(LL_LENGTH_REQ , TKL_BLE_DATA_LENGTH);
#if TKL_BLUETOOTH_PHY_2M
                blc_ll_setPhy(BLS_CONN_HANDLE, PHY_TRX_PREFER, PHY_PREFER_2M, PHY_PREFER_2M, CODED_PHY_PREFER_NONE); //There is little impact on OTA speed
#endif
                packet_connect_t *param = (packet_connect_t *)para;
                if (tkl_ble_gap_evt_func != NULL) {
                    TKL_BLE_GAP_PARAMS_EVT_T gap_param = {0};
                    gap_param.type = TKL_BLE_GAP_EVT_CONNECT;
                    gap_param.conn_handle = BLS_CONN_HANDLE;
                    gap_param.result = 0;
                    gap_param.gap_event.connect.role = TKL_BLE_ROLE_SERVER;
                    gap_param.gap_event.connect.peer_addr.type = 0;
                    memcpy(&gap_param.gap_event.connect.peer_addr.addr, param->initA, MAC_ADDR_LEN);
                    gap_param.gap_event.connect.conn_params.conn_interval_min = param->interval;
                    gap_param.gap_event.connect.conn_params.conn_interval_max = param->interval;
                    gap_param.gap_event.connect.conn_params.conn_latency = param->latency;
                    gap_param.gap_event.connect.conn_params.conn_sup_timeout = param->timeout;
                    gap_param.gap_event.connect.conn_params.connection_timeout = 0;
                    tkl_ble_gap_evt_func(&gap_param);
                }
            } break;

            case BLT_EV_FLAG_TERMINATE: {
                if (tkl_is_advertising) {
                    tkl_ble_state = TY_BLE_STA_ADV;
                } else {
                    tkl_ble_state = TY_BLE_STA_IDLE;
                }
                if (tkl_ble_gap_evt_func != NULL) {
                    TKL_BLE_GAP_PARAMS_EVT_T gap_param = {0};
                    gap_param.type = TKL_BLE_GAP_EVT_DISCONNECT;
                    gap_param.conn_handle = BLS_CONN_HANDLE;
                    gap_param.result = 0;
                    gap_param.gap_event.disconnect.reason = *para;
                    tkl_ble_gap_evt_func(&gap_param);
                }

#if TKL_BLUETOOTH_SUPPORT_SCAN
                    if (tkl_is_scanning && (bls_ll_isConnectState() == 0) && (tkl_is_advertising == false)) {
                        ble_sts_t ble_sts = blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_ENABLE);
                        if (ble_sts == BLE_SUCCESS) {
                            tkl_only_scan = 1;
                        } else {
                            TAL_PR_ERR("enable err: %d", ble_sts);
                        }
                    }
#endif
            } break;

            case BLT_EV_FLAG_GPIO_EARLY_WAKEUP: {
            } break;

            case BLT_EV_FLAG_CHN_MAP_REQ: {
            } break;

            case BLT_EV_FLAG_PHY_UPDATE: {
                hci_le_phyUpdateCompleteEvt_t *pEvt = (hci_le_phyUpdateCompleteEvt_t *)para;
                TY_PRINTF("PHY TX = %d, RX = %d", pEvt->tx_phy, pEvt->rx_phy);
            } break;

            case BLT_EV_FLAG_CONN_PARA_REQ: {
            } break;

            case BLT_EV_FLAG_CHN_MAP_UPDATE: {
            } break;

            case BLT_EV_FLAG_CONN_PARA_UPDATE: {
                update_conn_para_t *param = (update_conn_para_t *)para;
                if (tkl_ble_gap_evt_func != NULL) {
                    TKL_BLE_GAP_PARAMS_EVT_T gap_param = {0};
                    gap_param.type = TKL_BLE_GAP_EVT_CONN_PARAM_UPDATE;
                    gap_param.conn_handle = BLS_CONN_HANDLE;
                    gap_param.result = 0;
                    gap_param.gap_event.conn_param.conn_interval_min = param->interval;
                    gap_param.gap_event.conn_param.conn_interval_max = param->interval;
                    gap_param.gap_event.conn_param.conn_latency = param->latency;
                    gap_param.gap_event.conn_param.conn_sup_timeout = param->timeout;
                    gap_param.gap_event.conn_param.connection_timeout = 0;
                    tkl_ble_gap_evt_func(&gap_param);
                }
            } break;

            case BLT_EV_FLAG_ADV_DURATION_TIMEOUT: {
            } break;

            case BLT_EV_FLAG_SUSPEND_ENTER: {
            } break;

            case BLT_EV_FLAG_SUSPEND_EXIT: {
                //user must set rf power index after every suspend wakeUp, cause relative setting will be reset in suspend
                tkl_ble_gap_tx_power_set(0, RF_POWER_P3p01dBm);
                extern VOID_T app_suspend_exit (VOID_T);
                app_suspend_exit ();
            } break;

            default: {
            } break;
        }
    }

    if ((h & HCI_FLAG_EVENT_BT_STD) != 0) {
#if TKL_BLUETOOTH_SUPPORT_SCAN
        UINT8_T subcode = para[0];
        // ADV packet
        if (subcode == HCI_SUB_EVT_LE_ADVERTISING_REPORT) {
            if (tkl_ble_gap_evt_func != NULL) {
                TKL_BLE_GAP_PARAMS_EVT_T gap_param = {0};
                event_adv_report_t *pa = (event_adv_report_t *)para;

                gap_param.type = TKL_BLE_GAP_EVT_ADV_REPORT;
                gap_param.conn_handle = BLS_CONN_HANDLE;
                gap_param.result = 0;
                gap_param.gap_event.adv_report.adv_type       = pa->event_type;
                gap_param.gap_event.adv_report.peer_addr.type = pa->adr_type;
                memcpy(gap_param.gap_event.adv_report.peer_addr.addr, pa->mac, 6);
                gap_param.gap_event.adv_report.rssi           = -pa->data[pa->len];
                gap_param.gap_event.adv_report.channel_index  = 0;
                gap_param.gap_event.adv_report.data.length    = pa->len;
                gap_param.gap_event.adv_report.data.p_data    = pa->data;
                tkl_ble_gap_evt_func(&gap_param);
            }
        }
#endif
    }
    return OPRT_OK;
}

STATIC OPERATE_RET tkl_module_on_receive_data(rf_packet_att_write_t *p)
{
    UINT8_T len = p->l2capLen - 3;
    if (len > 0) {
        if (tkl_ble_gatt_evt_func != NULL) {
            TKL_BLE_GATT_PARAMS_EVT_T gatt_param = {0};
            gatt_param.type = TKL_BLE_GATT_EVT_WRITE_REQ;
            gatt_param.conn_handle = BLS_CONN_HANDLE;
            gatt_param.result = 0;
            gatt_param.gatt_event.write_report.char_handle = 0x10;

            if ((len <= 244) && (p->handle != s_cccd_handle)) {
                gatt_param.gatt_event.write_report.report.length = len;
                gatt_param.gatt_event.write_report.report.p_data = tkl_system_malloc(len);
                if (gatt_param.gatt_event.write_report.report.p_data) {
                    memcpy(gatt_param.gatt_event.write_report.report.p_data, &(p->value), len);

                    tkl_ble_gatt_evt_func(&gatt_param);

                    tkl_system_free(gatt_param.gatt_event.write_report.report.p_data);
                }
            }
        }
    }

    return OPRT_OK;
}

STATIC VOID_T tkl_ble_mac_init(VOID_T)
{
    extern UINT32_T tuya_ble_storage_load_settings(VOID_T);
    tuya_ble_storage_load_settings();

    memcpy(mac_public, tuya_ble_current_para.auth_settings.mac, 6);
    if ((mac_public[0] == 0) && (mac_public[1] == 0) && (mac_public[2] == 0) && (mac_public[3] == 0) && (mac_public[4] == 0) && (mac_public[5] == 0)) {
        UINT8_T mac_temp[6];
        extern UINT32_T tal_util_str_hexstr2hexarray(UINT8_T* hexStr, UINT32_T size, UINT8_T* hexArray);
        tal_util_str_hexstr2hexarray((UINT8_T* )TY_DEVICE_MAC, (UINT32_T)MAC_STRING_LEN, mac_temp);
        for (UINT8_T i=0; i<6; i++) {
            mac_public[i] = mac_temp[5-i];
        }
    }


#if (BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
    app_own_address_type = OWN_ADDRESS_PUBLIC;
#elif (BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
    app_own_address_type = OWN_ADDRESS_RANDOM;
    blc_ll_setRandomAddr(mac_public);
#endif
}

STATIC VOID_T tkl_ble_controller_init(VOID_T)
{
    blc_ll_initBasicMCU();                      //mandatory
    blc_ll_initStandby_module(mac_public);      //mandatory
    blc_ll_initAdvertising_module(mac_public);  //adv module:          mandatory for BLE slave,
#if TKL_BLUETOOTH_SUPPORT_SCAN
    blc_ll_initScanning_module(mac_public);
#endif
    blc_ll_initConnection_module();         //connection module  mandatory for BLE slave/master
    blc_ll_initSlaveRole_module();          //slave module:      mandatory for BLE slave,

#if TKL_BLUETOOTH_PHY_2M
    blc_ll_init2MPhyCodedPhy_feature();  //There is little impact on OTA speed
#endif
}

STATIC OPERATE_RET app_conn_param_update_response(UINT8_T id, UINT16_T result)
{
    OPERATE_RET ret = OPRT_OK;
    if (tkl_conn_param_update_rsp_cb) {
        ret = tkl_conn_param_update_rsp_cb(id, result);
    }
    return ret;
}

STATIC OPERATE_RET app_host_event_callback(UINT32_T h, UINT8_T *para, int n)
{
    UINT8_T event = h & 0xFF;

    switch (event) {
        case GAP_EVT_ATT_EXCHANGE_MTU: {
            gap_gatt_mtuSizeExchangeEvt_t *p = (gap_gatt_mtuSizeExchangeEvt_t*)para;
            if (tkl_ble_gatt_evt_func != NULL) {
                TKL_BLE_GATT_PARAMS_EVT_T gatt_param = {0};
                gatt_param.type = TKL_BLE_GATT_EVT_MTU_REQUEST;
                gatt_param.conn_handle = BLS_CONN_HANDLE;
                gatt_param.result = 0;
                gatt_param.gatt_event.exchange_mtu = p->effective_MTU;
                tkl_ble_gatt_evt_func(&gatt_param);
            }
        } break;

        default: {
        } break;
    }

    return OPRT_OK;
}

STATIC VOID_T tkl_ble_host_init(VOID_T)
{
    blc_gap_peripheral_init(); //gap initialization
    blc_l2cap_register_handler (blc_l2cap_packet_receive); //l2cap initialization
    blc_l2cap_registerConnUpdateRspCb(app_conn_param_update_response); //register sig process handler
    blc_l2cap_initMtuBuffer(mtu_rx_fifo, MTU_RX_BUFF_SIZE_MAX, mtu_tx_fifo, MTU_TX_BUFF_SIZE_MAX);

    blc_att_setRxMtuSize(MTU_RX_MAX);

    // To Recive host mtu
    blc_gap_setEventMask(GAP_EVT_MASK_ATT_EXCHANGE_MTU);
    blc_gap_registerHostEventHandler(app_host_event_callback);
}

STATIC VOID_T tkl_ble_smp_init(VOID_T)
{
    blc_smp_setSecurityLevel(No_Security);
}

STATIC VOID_T tkl_ble_adv_init(VOID_T)
{
    bls_ll_setAdvData(tkl_adv_data.value, tkl_adv_data.len);
    bls_ll_setScanRspData(tkl_scan_rsp.value, tkl_scan_rsp.len);
    bls_ll_setAdvParam(tkl_adv_param.adv_interval_min,
                        tkl_adv_param.adv_interval_max,
                        ADV_TYPE_CONNECTABLE_UNDIRECTED,
                        OWN_ADDRESS_RANDOM,
                        0,
                        NULL,
                        BLT_ENABLE_ADV_ALL,
                        ADV_FP_NONE);
}

STATIC VOID_T tkl_ble_evt_init(VOID_T)
{
#if TKL_BLUETOOTH_SUPPORT_SCAN
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
#endif
    blc_hci_registerControllerEventHandler(controller_event_handler);   //register event callback
    bls_hci_mod_setEventMask_cmd(0xFFFFFFFF);  //enable all 18 events, event list see ll.h
}

OPERATE_RET tkl_ble_stack_init(UCHAR_T role)
{
    if ((role & TKL_BLE_ROLE_SERVER) == TKL_BLE_ROLE_SERVER) {
        // BLE stack initialization
        tkl_ble_mac_init();
        tkl_ble_controller_init();
        tkl_ble_host_init();
        tkl_ble_smp_init();

        // USER application initialization
        tkl_ble_gap_tx_power_set(0, RF_POWER_P3p01dBm);
        tkl_ble_adv_init();
        tkl_ble_evt_init();
    } else if ((role & TKL_BLE_ROLE_CLIENT) == TKL_BLE_ROLE_CLIENT) {
    }
    return OPRT_OK;
}

OPERATE_RET tkl_ble_stack_deinit(UCHAR_T role)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_stack_gatt_link(USHORT_T *p_link)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_gap_callback_register(TKL_BLE_GAP_EVT_FUNC_CB p_gap_evt)
{
    tkl_ble_gap_evt_func = p_gap_evt;
    return OPRT_OK;
}

OPERATE_RET tkl_ble_gatt_callback_register(TKL_BLE_GATT_EVT_FUNC_CB p_gatt_evt)
{
    tkl_ble_gatt_evt_func = p_gatt_evt;
    return OPRT_OK;
}

OPERATE_RET tkl_ble_gap_addr_set(TKL_BLE_GAP_ADDR_T CONST *p_addr)
{
    bool adv_flag = FALSE;

    if (tkl_is_advertising) {
        adv_flag = true;
        tkl_ble_gap_adv_stop();
    }

    memcpy(mac_public, p_addr->addr, 6);
    mac_type = p_addr->type;
    extern rf_packet_scan_rsp_t pkt_scan_rsp;
    if (mac_type == 0) {
        memcpy((char *)bltMac.macAddress_public, (char *)mac_public, 6);
        memcpy((char *)pkt_adv.advA, (char *)mac_public, 6);
        memcpy((char *)pkt_scan_rsp.advA, (char *)mac_public, 6);
    }

    if (adv_flag) {
        tkl_ble_gap_adv_start(NULL);
    }
    return OPRT_OK;
}

OPERATE_RET tkl_ble_gap_address_get(TKL_BLE_GAP_ADDR_T *p_addr)
{
    if (mac_type == OWN_ADDRESS_PUBLIC) {
        p_addr->type = TUYA_BLE_ADDRESS_TYPE_PUBLIC;
    } else {
        p_addr->type = TUYA_BLE_ADDRESS_TYPE_RANDOM;
    }
    memcpy(p_addr->addr, mac_public, 6);

    return OPRT_OK;
}

STATIC adv_type_t tkl_adv_type_transform(UINT8_T adv_type)
{
    switch (adv_type) {
        case TKL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED:                {return ADV_TYPE_CONNECTABLE_UNDIRECTED;}
        case TKL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIR_HIGHDUTY_CYCLE:        {return ADV_TYPE_CONNECTABLE_DIRECTED_HIGH_DUTY;}
        case TKL_BLE_GAP_ADV_TYPE_NONCONN_SCANNABLE_UNDIRECTED:                {return ADV_TYPE_SCANNABLE_UNDIRECTED;}
        case TKL_BLE_GAP_ADV_TYPE_NONCONN_NONSCANNABLE_UNDIRECTED:            {return ADV_TYPE_NONCONNECTABLE_UNDIRECTED;}
        case TKL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIRECTED:                {return ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY;}
        default:      {return -1;}
    }
}

OPERATE_RET tkl_ble_gap_adv_start(TKL_BLE_GAP_ADV_PARAMS_T CONST *p_adv_params)
{
    OPERATE_RET ret = OPRT_OK;

    if (p_adv_params != NULL) {
        memcpy(&tkl_adv_param, p_adv_params, SIZEOF(TKL_BLE_GAP_ADV_PARAMS_T));
    }

    if (tkl_adv_type_transform(tkl_adv_param.adv_type) == -1) {
        return OPRT_NOT_SUPPORTED;
    }

#if TKL_BLUETOOTH_SUPPORT_SCAN
    if (tkl_only_scan) {
        ble_sts_t ble_sts = blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_ENABLE);
        if (ble_sts == BLE_SUCCESS) {
            tkl_only_scan = 0;
        } else {
            TAL_PR_ERR("disable err");
        }
    }
#endif

    if (tkl_is_advertising) {
        bls_ll_setAdvEnable(0);  //adv disable
    }

    bls_ll_setAdvParam(tkl_adv_param.adv_interval_min, tkl_adv_param.adv_interval_max, tkl_adv_type_transform(tkl_adv_param.adv_type), OWN_ADDRESS_RANDOM, 0, NULL, tkl_adv_param.adv_channel_map, ADV_FP_NONE);
    bls_ll_setAdvEnable(1);  //adv enable
    tkl_is_advertising = true;

    if (bls_ll_isConnectState() == 0) {
        tkl_ble_state = TY_BLE_STA_ADV;
    }
    return ret;
}

OPERATE_RET tkl_ble_gap_adv_stop(VOID_T)
{
    OPERATE_RET ret = OPRT_OK;

    ret = bls_ll_setAdvEnable(0);  //adv disable
    tkl_is_advertising = FALSE;

#if TKL_BLUETOOTH_SUPPORT_SCAN
    if (tkl_is_scanning && (bls_ll_isConnectState() == 0)) {
        ble_sts_t ble_sts = blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_ENABLE);
        if (ble_sts == BLE_SUCCESS) {
            tkl_only_scan = 1;
        } else {
            TAL_PR_ERR("enable err: %d", ble_sts);
        }
    }
#endif

    if (bls_ll_isConnectState()) {
        tkl_ble_state = TY_BLE_STA_CONN;
    } else {
        tkl_ble_state = TY_BLE_STA_IDLE;
    }
    return ret;
}

BOOL_T tkl_ble_gap_adv_is_running(VOID_T)
{
    return tkl_is_advertising;
}

BOOL_T tkl_ble_gap_scan_is_running(VOID_T)
{
#if TKL_BLUETOOTH_SUPPORT_SCAN
    return tkl_is_scanning;
#else
    return FALSE;
#endif
}

OPERATE_RET tkl_ble_gap_adv_rsp_data_set(TKL_BLE_DATA_T CONST *p_adv, TKL_BLE_DATA_T CONST *p_scan_rsp)
{
    OPERATE_RET ret = OPRT_OK;
    bool_t adv_flag = FALSE;

    if (tkl_is_advertising) {
        adv_flag = TRUE;
        bls_ll_setAdvEnable(0);  //adv disable
    }

    if (p_adv->p_data != NULL) {
        tkl_adv_data.len         = p_adv->length;
        memcpy(tkl_adv_data.value, p_adv->p_data, p_adv->length);
        bls_ll_setAdvData(tkl_adv_data.value, tkl_adv_data.len);
    }
    if (p_scan_rsp->p_data != NULL) {
        tkl_scan_rsp.len         = p_scan_rsp->length;
        memcpy(tkl_scan_rsp.value, p_scan_rsp->p_data, p_scan_rsp->length);
        bls_ll_setScanRspData(tkl_scan_rsp.value, tkl_scan_rsp.len);
    }
    if (adv_flag) {
        tkl_ble_gap_adv_start(NULL);
    }

    return ret;
}

OPERATE_RET tkl_ble_gap_adv_rsp_data_update(TKL_BLE_DATA_T CONST *p_adv, TKL_BLE_DATA_T CONST *p_scan_rsp)
{
    OPERATE_RET ret = OPRT_OK;
    bool_t adv_flag = FALSE;

    if (tkl_is_advertising) {
        adv_flag = TRUE;
        bls_ll_setAdvEnable(0);  //adv disable
    }

    if (p_adv->p_data != NULL) {
        tkl_adv_data.len         = p_adv->length;
        memcpy(tkl_adv_data.value, p_adv->p_data, p_adv->length);
        bls_ll_setAdvData(tkl_adv_data.value, tkl_adv_data.len);
    }
    if (p_scan_rsp->p_data != NULL) {
        tkl_scan_rsp.len         = p_scan_rsp->length;
        memcpy(tkl_scan_rsp.value, p_scan_rsp->p_data, p_scan_rsp->length);
        bls_ll_setScanRspData(tkl_scan_rsp.value, tkl_scan_rsp.len);
    }

    if (adv_flag) {
        tkl_ble_gap_adv_start(NULL);
    }

    return ret;
}

OPERATE_RET tkl_ble_gap_scan_start(TKL_BLE_GAP_SCAN_PARAMS_T CONST *p_scan_params)
{
#if TKL_BLUETOOTH_SUPPORT_SCAN
    if (p_scan_params == NULL) {
        return OPRT_INVALID_PARM;
    }

    if ((p_scan_params->interval == 0) || (p_scan_params->window == 0)) {
        return OPRT_INVALID_PARM;
    }

    memcpy(&tkl_scan_param, p_scan_params, sizeof(TKL_BLE_GAP_SCAN_PARAMS_T));

    // SCAN_TYPE_ACTIVE will send scan_req, and SCAN_TYPE_PASSIVE will not
    ble_sts_t ret;
    if(p_scan_params->active) {
        ret = blc_ll_setScanParameter(SCAN_TYPE_ACTIVE, tkl_scan_param.interval, tkl_scan_param.window, OWN_ADDRESS_RANDOM, SCAN_FP_ALLOW_ADV_ANY);
    } else {
        ret = blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, tkl_scan_param.interval, tkl_scan_param.window, OWN_ADDRESS_RANDOM, SCAN_FP_ALLOW_ADV_ANY);
    }
    if (ret != BLE_SUCCESS) {
        TAL_PR_ERR("set param err: %d", ret);
        return OPRT_COM_ERROR;
    }

    blc_ll_addScanningInAdvState();
    blc_ll_addScanningInConnSlaveRole();
    if ((tkl_is_advertising == 0) && (bls_ll_isConnectState() == 0)) {
        ble_sts_t ble_sts = blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_ENABLE);
        if (ble_sts == BLE_SUCCESS) {
            tkl_only_scan = 1;
        } else {
            TAL_PR_ERR("enable err: %d", ble_sts);
        }
    }

    tkl_is_scanning = TRUE;

    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

OPERATE_RET tkl_ble_gap_scan_stop(VOID_T)
{
#if TKL_BLUETOOTH_SUPPORT_SCAN
    blc_ll_removeScanningFromAdvState();
    blc_ll_removeScanningFromConnSLaveRole();
    if ((tkl_is_advertising == 0) && (bls_ll_isConnectState() == 0)) {
        ble_sts_t ble_sts = blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_ENABLE);
        if (ble_sts == BLE_SUCCESS) {
            tkl_only_scan = 0;
        } else {
            TAL_PR_ERR("enable err: %d", ble_sts);
        }
    }
    tkl_is_scanning = FALSE;

    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

OPERATE_RET tkl_ble_gap_connect(TKL_BLE_GAP_ADDR_T CONST *p_peer_addr, TKL_BLE_GAP_SCAN_PARAMS_T CONST *p_scan_params, TKL_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params)
{
    OPERATE_RET ret = OPRT_OK;

    return ret;
}

OPERATE_RET tkl_ble_gap_disconnect(USHORT_T conn_handle, UCHAR_T hci_reason)
{
    OPERATE_RET ret = OPRT_OK;

    if (bls_ll_isConnectState()) {
        ret = bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);
    } else {
        ret = OPRT_NETWORK_ERROR;
    }

    return ret;
}

OPERATE_RET tkl_ble_gap_conn_param_update(USHORT_T conn_handle, TKL_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params)
{
    OPERATE_RET ret = OPRT_OK;

    if (bls_ll_isConnectState()) {
        bls_l2cap_requestConnParamUpdate (p_conn_params->conn_interval_min,
            p_conn_params->conn_interval_max, p_conn_params->conn_latency, p_conn_params->conn_sup_timeout);
    } else {
        ret = OPRT_NETWORK_ERROR;
    }

    return ret;
}

OPERATE_RET tkl_ble_gap_tx_power_set(UCHAR_T role, INT32_T tx_power)
{
    rf_set_power_level_index (tx_power);
    return OPRT_OK;
}

OPERATE_RET tkl_ble_gap_rssi_get(USHORT_T conn_handle)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_gatts_service_add(TKL_BLE_GATTS_PARAMS_T *p_service)
{
    if (p_service == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (p_service->p_service == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (p_service->p_service->p_char == NULL) {
        return OPRT_INVALID_PARM;
    }

    for (UINT32_T idy=0; idy<1; idy++) {
        UINT32_T idx = 0x0C;

        // Add service
        tkl_Attributes[idx].uuidLen = 2;
        my_primaryServiceUUID = p_service->p_service->type;

        if (p_service->p_service->svc_uuid.uuid_type == TKL_BLE_UUID_TYPE_16) {
            tkl_Attributes[idx].attrLen = 2;
            memcpy(tkl_service_uuid, p_service->p_service->svc_uuid.uuid.uuid128, tkl_Attributes[idx].attrLen);
        } else {
            return OPRT_NOT_SUPPORTED;
        }

        // Add Characteristic.
        TKL_BLE_CHAR_PARAMS_T *p_char = p_service->p_service->p_char;
        for (UINT32_T char_num=0; char_num<p_service->p_service->char_num; char_num++) {
            //char
            idx++;
            tkl_Attributes[idx].uuidLen = 2;
            tkl_character[char_num].prop = p_char->property;

            //char value
            idx++;
            tkl_Attributes[idx].uuidLen = (p_char->char_uuid.uuid_type == TKL_BLE_UUID_TYPE_16) ? 2 :
                ((p_char->char_uuid.uuid_type == TKL_BLE_UUID_TYPE_32) ? 4 : 16);
            memcpy(tkl_character[char_num].uuid, p_char->char_uuid.uuid.uuid128, tkl_Attributes[idx].uuidLen);

            p_char->handle = idx;

            //char client characteristic configuration
            if ((p_char->property & TKL_BLE_GATT_CHAR_PROP_NOTIFY) != 0) {
                idx++;
                s_cccd_handle = idx;
            }

            p_char++;
        }
    }

    bls_att_setAttributeTable ((UINT8_T *)tkl_Attributes);
    UINT8_T device_name[] = MY_DEV_NAME;
    bls_att_setDeviceName(device_name, SIZEOF(MY_DEV_NAME));

    return OPRT_OK;
}

OPERATE_RET tkl_ble_gatts_value_set(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_gatts_value_get(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_gatts_value_notify(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    if (bls_ll_isConnectState()) {
        return bls_att_pushNotifyData(0x10, p_data, length);
    } else {
        return OPRT_NETWORK_ERROR;
    }
}

OPERATE_RET tkl_ble_gatts_value_indicate(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    if (bls_ll_isConnectState()) {
        return bls_att_pushIndicateData(0x10, p_data, length);
    } else {
        return OPRT_NETWORK_ERROR;
    }
}

OPERATE_RET tkl_ble_gatts_exchange_mtu_reply(USHORT_T conn_handle, USHORT_T server_rx_mtu)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_all_service_discovery(USHORT_T conn_handle)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_all_char_discovery(USHORT_T conn_handle, USHORT_T start_handle, USHORT_T end_handle)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_char_desc_discovery(USHORT_T conn_handle, USHORT_T start_handle, USHORT_T end_handle)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_write_without_rsp(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_write(USHORT_T conn_handle, USHORT_T char_handle, UCHAR_T *p_data, USHORT_T length)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_read(USHORT_T conn_handle, USHORT_T char_handle)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_gattc_exchange_mtu_request(USHORT_T conn_handle, USHORT_T client_rx_mtu)
{
    UINT32_T err_code = OPRT_OK;

    return err_code;
}

OPERATE_RET tkl_ble_vendor_command_control(USHORT_T opcode, VOID_T *user_data, USHORT_T data_len)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_ble_conn_param_update_rsp_cb_register(OPERATE_RET(*func)(UINT8_T, UINT16_T))
{
    if (func == NULL) {
        return OPRT_INVALID_PARM;
    }

    tkl_conn_param_update_rsp_cb = func;

    return OPRT_OK;
}

