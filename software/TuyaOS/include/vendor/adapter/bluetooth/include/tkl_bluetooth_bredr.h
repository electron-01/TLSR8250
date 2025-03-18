/**
 * @file tkl_bluetooth_bredr.h
 * @brief This is tkl_bluetooth_bredr file
 * @version 1.0
 * @date 2022-08-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_BLUETOOTH_BREDR_H__
#define __TKL_BLUETOOTH_BREDR_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#include "tuya_ble_type.h"

#include "tal_log.h"
#include "app_prompt_wav.h"
#include "drv_audio.h"
#include "app_bt.h"

#include "tkl_bluetooth_bredr.h"
#include "bluetooth.h"
#include "app_beken_includes.h"

#include "beken_external.h"
#include "app_env.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
* 统一蓝牙BREDR接口，主要应用场景如下：
* 1，作为蓝牙音箱设备（Sink），由手机端（Source）播放音频于设备上。如：涂鸦智能音箱，涂鸦wifi语音音箱等
* 2，作为蓝牙耳机（Sink），由手机端或移动终端（Sink）播放音频于设备上。如：涂鸦蓝牙耳机
*
* 注：制定该接口主要应用场景为Sink端，由于暂无Source需求，我们将专注于Sink端接口规范，且无需关心音频数据流。
* 如：将音箱数据送入下一个音频播放设备等不在我们考虑范围，协议不支持点到多点连接，故暂不考虑。
*
* 规范蓝牙BREDR接口目的：
* 1，剥离业务到驱动接口，最大限度规范上下层行为。
* 2，更好地按需进行业务与驱动拓展，避免无效的接口导入与应用。
* 3，更好地拓展TuyaOS在蓝牙业务上的需求点。
* 4，轻量化接口便于理解。
*/

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
typedef enum {
    TUYA_BT_BREDR_STACK_INIT  = 0x01 ,                  /**< Init Bluetooth BR-EDR Stack, return  refer@Stack Error Code.result */

    TUYA_BT_BREDR_STACK_DEINIT,                         /**< Deinit Bluetooth BR-EDR Stack */

    TUYA_BT_BREDR_STACK_RESET,                          /**< Reset Bluetooth BR-EDR Stack */

    TUYA_BT_BREDR_GAP_EVT_CONNECT,                      /**< General Connected */

    TUYA_BT_BREDR_GAP_EVT_DISCONNECT,                   /**< General Disconnected */

    TUYA_BT_BREDR_GAP_EVT_PAIR,                         /**< General Pairing */

    TUYA_BT_BREDR_GAP_INFO_INQUIRY,                     /**< Report Device Info inquiry */

    TUYA_BT_BREDR_STREAM_STATUS,                        /**< Report BR-EDR Stream Status */

    TUYA_BT_BREDR_PHONE_STATUS,                         /**< Report Phone Status */
} TUYA_BT_BREDR_EVENT_TYPE_E;

typedef enum {
    TUYA_BT_BREDR_MODE_IDLE = 0x00,                     /**< device is idle mode */
    TUYA_BT_BREDR_MODE_SCAN_INQUIRY_ONLY,               /**< device is only inquiry_scan mode */
    TUYA_BT_BREDR_MODE_SCAN_PAGE_ONLY,                  /**< device is only page_scan mode */
    TUYA_BT_BREDR_MODE_SCAN_INQUIRY_AND_SCAN_PAGE,      /**< device is both inquiry_scan and page_scan mode */
} TUYA_BT_SCAN_MODE_E;

typedef enum {
    TUYA_BT_PAIR_BOND_START = 0x01,                     /**< Indicatet the pairing procedure running*/
    TUYA_BT_PAIR_BOND_REQUEST,                          /**< After the phone request pairing with any passkey, we need to input the passkey*/
    TUYA_BT_PAIR_BOND_CONFIRM,                          /**< After the phone request pairing with one passkey, we can show it and do confirm */
    TUYA_BT_PAIR_BOND_SUCCESS,                          /**< Pairing successfully*/
    TUYA_BT_PAIR_BOND_FAIL,                             /**< Pairing fail*/
} TUYA_BT_PAIR_EVENT_TYPE_E;

typedef enum {
    // [AVRCP], Audio/Video Remote Control, 4.6.X Support level in CT&TG
    TUYA_BT_BREDR_RCP_START = 0x01,                    /**< Start to control Audio */
    TUYA_BT_BREDR_RCP_PLAY,                             /**< Play Music */
    TUYA_BT_BREDR_RCP_STOP,                             /**< Stop Playing Music */
    TUYA_BT_BREDR_RCP_PAUSE,                            /**< Pause Music */
    TUYA_BT_BREDR_RCP_NEXT,                             /**< Play Next Music */
    TUYA_BT_BREDR_RCP_PREV,                             /**< Play Previous Music */
    TUYA_BT_BREDR_RCP_FORWARD,                          /**< Forward Music */
    TUYA_BT_BREDR_RCP_REWIND,                           /**< Rewind Music */
    TUYA_BT_BREDR_RCP_REPEAT,                           /**< Repeat Music */
    TUYA_BT_BREDR_RCP_MUTE,                             /**< Mute for Music, user_data: NULL */
    TUYA_BT_BREDR_RCP_VOLUME_UP,                        /**< Volume up for Music, step: 1, user_data: NULL */
    TUYA_BT_BREDR_RCP_VOLUME_DOWN,                      /**< Volume down for Music, step: 1, user_data: NULL */

    // [HFP], Hands-Free Profile
    TUYA_BT_BREDR_HFP_START,                            /**< Start to control Phone */
    TUYA_BT_BREDR_HFP_ANSWER,                           /**< Answer the Phone */
    TUYA_BT_BREDR_HFP_HANGUP,                           /**< Hang Up the Phone */
    TUYA_BT_BREDR_HFP_REJECT,                           /**< Reject the Phone */
    TUYA_BT_BREDR_HFP_CALL,                             /**< Call the Phone */

    // [HFP], Need to post the data
    TUYA_BT_BREDR_HFP_UPDATE_BATTERY,                   /**< Update Battery to the Phone */
    TUYA_BT_BREDR_HFP_VOLUME_UP,                        /**< Volume up for the phone, step: 1, user_data: NULL */
    TUYA_BT_BREDR_HFP_VOLUME_DOWN,                      /**< Volume down for the phone, step: 1, user_data: NULL */
    TUYA_BT_BREDR_HFP_SET_VOLUME,                       /**< Set the volume for the phone, post volume percent into adapter.
                                                                Eg: Set Volume into 50%[Range: 0-100],we should post 50(or hex: 0x32)*/
    TUYA_BT_BREDR_HFP_GET_VOLUME,                       /**< Get the volume from the phone, user_data: NULL */

    TUYA_BT_BREDR_A2DP_START,                           /**< Start to play audio*/

    TUYA_BT_BREDR_AVRCP_DISCONNECT,                     /**< Request Disconnect AVRCP Profile */
    TUYA_BT_BREDR_A2DP_DISCONNECT,                      /**< Request Disconnect A2DP Profile */
    TUYA_BT_BREDR_HFP_DISCONNECT,                       /**< Request Disconnect HFP Profile */
    TUYA_BT_BREDR_START_CONNECTION,                     /**< Request Connect One Device*/
    TUYA_BT_BREDR_CANCEL_CONNECTION,                    /**< Cancel Connecting One Device*/
} TUYA_BT_BREDR_CONTROL_E;

/**< BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 2, Part B 2.2.1 Master-Slave definition */
typedef enum {
	TUYA_BT_BREDR_ROLE_MASTER = 0x01,                   /**< BR-EDR Role: Master, Not Central From Core Spec */
	TUYA_BT_BREDR_ROLE_SLAVE  = 0x02,                   /**< BR-EDR Role: Slave */
} TUYA_BT_BREDR_ROLE_E;

typedef enum {
    TUYA_BT_STREAM_STATUS_IDLE = 0x01,                  /**< Current Audio Streaming Being in Idle Mode. */
    TUYA_BT_STREAM_STATUS_CONNECTED,                    /**< A2DP Connected */
    TUYA_BT_STREAM_STATUS_DISCONNECTED,                 /**< A2DP Disonnected */
    TUYA_BT_STREAM_STATUS_START,                        /**< Current Audio Streaming Has been started. */
    TUYA_BT_STREAM_STATUS_SUSPENDING,                   /**< Current Audio Streaming Being in Suspend Mode. */
    TUYA_BT_STREAM_STATUS_STOP,                         /**< Current Audio Streaming Has been stoped. */
} TUYA_BT_BREDR_STEAM_STATUS_E;

typedef enum {
    TUYA_BT_PHONE_STATUS_IDLE = 0x01,                   /**< Current Device Being in Idle Mode. */
    TUYA_BT_PHONE_STATUS_CONNECTED,                     /**< HFP Connected */
    TUYA_BT_PHONE_STATUS_DISCONNECTED,                  /**< HFP Disonnected */
    TUYA_BT_PHONE_INCOMING,                             /**< Incoming phone, indicate the status for device */
    TUYA_BT_PHONE_OUTGOING,                             /**< Outgoing phone, indicate the status for device */
    TUYA_BT_PHONE_ACTIVE,                               /**< Currently, the device is being phone active */
    TUYA_BT_PHONE_HANGUP,                               /**< Hang Up phone, indicate the status for device */
    TUYA_BT_PHONE_VOLOUME_CHANGED,                      /**< Indicate the volume changed */
    TUYA_BT_PHONE_UPDATE_BATTERY,                       /**< Update the battery */
} TUYA_BT_BREDR_PHONE_STATUS_E;

typedef enum {
    TUYA_BT_PAIR_MODE_NO_PAIRING,                       /**< Pairing is not allowed */
    TUYA_BT_PAIR_MODE_WAIT_FOR_REQ,                     /**< Wait for a pairing request or slave security request */
    TUYA_BT_PAIR_MODE_INITIATE,                         /**< Don't wait, initiate a pairing request or slave security request*/
} TUYA_BT_PAIR_MODE_E;

typedef enum {
    TUYA_BT_IO_CAP_DISPLAY_ONLY,                        /**< Display Only Device */
    TUYA_BT_IO_CAP_DISPLAY_YES_NO,                      /**< Display and Yes and No Capable */
    TUYA_BT_IO_CAP_KEYBOARD_ONLY,                       /**< Keyboard Only */
    TUYA_BT_IO_CAP_NO_INPUT_NO_OUTPUT,                  /**< No Display or Input Device */
    TUYA_BT_IO_CAP_KEYBOARD_DISPLAY,                    /**< Both Keyboard and Display Capable */
} TUYA_BT_PAIR_IO_CAP_E;

typedef enum {
    TUYA_BT_PAIR_REQUEST_CONFIRMATION,                  /**< Confirmation request then should send pair_enable*/ 
    TUYA_BT_PAIR_REQUEST_PASSKEY,                       /**< passkey request then should enter passkey  */ 
    TUYA_BT_PAIR_REQUEST_PRESSKEY,                      /**< */
    TUYA_BT_PAIR_REQUEST_PIN,                           /**< pin request then should enter pair_enable */
} TUYA_BT_PAIR_REQUEST_T;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/
typedef struct {
    // UCHAR_T                             type;           /**< Mac Address Type, Refer to @ TKL_BLE_GAP_ADDR_TYPE_PUBLIC or TKL_BLE_GAP_ADDR_TYPE_RANDOM*/
    UCHAR_T                             addr[6];        /**< Mac Address, Address size, 6 bytes */
} TUYA_BT_GAP_ADDR_T;

typedef struct {
    TUYA_BT_PAIR_MODE_E                 mode;           /**< Bond Manager Pairing Modes */
	TUYA_BT_PAIR_IO_CAP_E               io_cap;         /**< Bond Manager I/O Capabilities Refer to @TUYA_BT_PAIR_IO_CAP_E */

	UCHAR_T                             oob_data;
	UCHAR_T                             mitm;           /**< Man In The Middle mode enalbe/disable */
    BOOL_T                              ble_secure_conn;/**< BLE Secure Simple Pairing, also called Secure Connection mode. Enable or not */
    UINT_T                              passkey;        /**< Init passkey. */
} TUYA_BT_PAIR_INIT_PARAM_T;

typedef struct {
	TUYA_BT_GAP_ADDR_T                  addr;           /**< Address of the remote device. */
    UCHAR_T                             link_key[16];   /**< security keys. */

    VOID                                *user_data;
} TUYA_BT_PAIR_BOND_INFO_T;

typedef struct {
	TUYA_BT_GAP_ADDR_T                  addr;           /**< Address of the remote device. */
    UINT_T                              passkey;        /**< respond passkey. */

    VOID                                *user_data;
} TUYA_BT_PAIR_DEVICE_T;

typedef struct {
    TUYA_BT_GAP_ADDR_T                  addr;           /**< Disconnection handle on which the event occured.*/
    UINT_T                              reason;         /**< Disconnection Reason */
    VOID                                *user_data;
} TUYA_BT_DISCONNECT_EVT_T;

typedef struct {
    TUYA_BT_PAIR_REQUEST_T              req;            /**< pair request */
    UINT_T                              passkey;        /**< Init passkey. */
    VOID                                *user_data;
} TUYA_BT_PAIR_BOND_EVT_T;

typedef struct {
    TUYA_BT_GAP_ADDR_T                  addr;           /**< Address of the remote device. */
    UCHAR_T                             *name;          /**< BT name of the remote device. */
    UCHAR_T                             name_len;
    VOID                                *user_data;
} TUYA_BT_PAIR_INQUIRY_EVT_T;


typedef struct {
    TUYA_BT_BREDR_STEAM_STATUS_E        status;         /**< Stream Status */

    VOID_T                              *p_endpoint;    /**< [Reserved] Stream Endpoint Pointer */
    VOID_T                              *p_connection;  /**< [Reserved] Stream Connection Pointer */
	VOID_T                              *user_data;     /**< Stream User Data */
} TUYA_BT_BREDR_STEAM_T;

typedef struct {
    TUYA_BT_BREDR_PHONE_STATUS_E        status;         /**< Phone Status */
    UINT8_T                             user_data_len;  /**< User Data Length */
	VOID_T                              *user_data;     /**< Stream User Data */
} TUYA_BT_BREDR_PHONE_T;

typedef struct {
    TUYA_BT_BREDR_EVENT_TYPE_E          type;           /**< Tuya BR-EDR Event */
    INT_T                               result;         /**< Indiacte event result for bluetooth callback */

    union {
        TUYA_BT_PAIR_BOND_EVT_T         pair;           /**< Pairing Event callback */
        TUYA_BT_PAIR_INQUIRY_EVT_T      device;         /**< Information of device which is inquiry */
        TUYA_BT_DISCONNECT_EVT_T        disconnect;     /**< Disconnect Event callback */
        TUYA_BT_PAIR_BOND_INFO_T        bond;           /**< After paring successfully, we will report link key,  Version 5.2 | Vol 2, Part F, Figure 3.10
                                                            If fail, we will report NULL and report fail result */
        TUYA_BT_BREDR_STEAM_T           audio;          /**< Tuya Bluetooth Audio Streaming Callback */
        TUYA_BT_BREDR_PHONE_T           phone;          /**< Tuya Bluetooth Phone Callback */
    } profile_event;
} TUYA_BT_BREDR_EVENT_T;


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
/**< Tuya Bluetooth BR-EDR Callback Register function definition */
typedef VOID(*TUYA_BT_BREDR_EVT_FUNC_CB)(TUYA_BT_BREDR_EVENT_T *p_event);

/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/



/**
 * @brief   Init the Bluetooth BR-EDR Interface.
 * @param   [in] role: Init the bt bredr role;
 *          [in] p_event: register callback;
 *          [in] user_data: Init the user data;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_init(TUYA_BT_BREDR_ROLE_E role, TUYA_BT_BREDR_EVT_FUNC_CB p_event, VOID_T *user_data);

/**
 * @brief   De-Init the Bluetooth BR-EDR Interface.
 * @param   [in] role: De-init the bt bredr role;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_deinit(TUYA_BT_BREDR_ROLE_E role);

/**
 * @brief   Reset the Bluetooth BR-EDR Interface.
 * @param   [in] role: Reset the bt bredr role;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_reset(TUYA_BT_BREDR_ROLE_E role);

/**
 * @brief   Set the Bluetooth BR-EDR pair mode Interface.
 * @param   [in] pair: Bluetooth BR-EDR pair mode;
 *          [in] user_data: Init the user data;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_pair_set(TUYA_BT_PAIR_INIT_PARAM_T pair);

/**
 * @brief   Enable the Bluetooth BR-EDR Interface.
 * @param   [in] mode: see TUYA_BT_SCAN_MODE_E
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_enable(TUYA_BT_SCAN_MODE_E mode);

/**
 * @brief   Enable the Bluetooth BR-EDR Interface.
 * @param   [in] enable: TRUE: Enable the bluetooth bredr page.
 *                       FALSE: Disable the bluetooth bredr page.
 *          [in] peer_addr: peer address for device which should be paging. if is NULL, will page the last device
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_page_enable(BOOL_T enable, TUYA_BT_GAP_ADDR_T *p_peer_addr);

/**
 * @brief   Enable the Bluetooth BR-EDR Interface.
 * @param   [in] enable: TRUE: Enable the bluetooth bredr inquiry.
 *                       FALSE: Disable the bluetooth bredr inquiry.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_inquiry_enable(BOOL_T enable);

/**
 * @brief   Set the BT Address
 * @param   [in] p_peer_addr: set peer address for BT;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_address_set(TUYA_BT_GAP_ADDR_T CONST *p_peer_addr);

/**
 * @brief   Get the BT Address
 * @param   [out] p_peer_addr: get peer address for BT;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_address_get(TUYA_BT_GAP_ADDR_T *p_peer_addr);

/**
 * @brief   Set the BT GAP Name
 * @param   [in] p_peer_addr: set local bluetooth gap name for BT;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_name_set(CHAR_T *name);

/**
 * @brief   Get the BT GAP Name
 * @param   [in] p_peer_addr: get local bluetooth gap name for BT;
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_name_get(CHAR_T *name);

/**
 * @brief   Get the BT GAP Name
 * @param   [in] p_peer_addr: get local bluetooth gap name for BT;
 *
 * @return  SUCCESS
 *          ERROR
 * */
tuya_ble_br_edr_connect_status_t tkl_bt_connect_status_get(VOID_T);

/**
 * @brief   Request the pair while in BT-Master Mode.
 * @param   [in] mode: request security pairing mode for BT;
 *          [in] bond_info: post the major information for pairing.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_paring_request(TUYA_BT_PAIR_DEVICE_T *p_device);

/**
 * @brief   Send paring passkey when in keyboard mode.
 * @param   [in] passkey: eg: 0x0001E240 means the passkey is 123456
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_paring_passkey_send(UINT_T passkey);

/**
 * @brief   Enable or Disable pair when pairing request
 * @param   [in] en: TRUE for enable and FALSE for disable pair
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_paring_enable_send(BOOL_T en);

/**
 * @brief   Delete the pair informations.
 * @param   [in] bond_info: delete the bond info.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_paring_delete(TUYA_BT_PAIR_BOND_INFO_T *bond_info);

/**
 * @brief   check if device is paired
 * @param   [in] NULL
 *
 * @return  TRUE
 *          FALSE
 * */
BOOL_T tkl_bt_pairing_status_get(VOID_T);

/**
 * @brief   disconnect the link.
 * @param   [in] bond_info: disconnect the bond info.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_gap_disconnect(TUYA_BT_PAIR_BOND_INFO_T *bond_info);

/**
 * @brief   Control the audio or phone, please refer to @TUYA_BT_BREDR_CONTROL_E.
 * @param   [in] crtl_event: control operations.
 *          [in] user_data: post the user-data for audio-control.
 *          [in] data_len: post the user-data length for audio-control.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_control(TUYA_BT_BREDR_CONTROL_E crtl_event, UCHAR_T *user_data, USHORT_T data_len);

/**
 * @brief   Control the bredr eq
 * @param   [in] eq_mode: control equalizer mode.
 *          [in] eq_data: post the eq-data for eq.
 *          [in] eq_data_len: post the eq-data length for eq.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_equalizer_set(UCHAR_T eq_mode, UCHAR_T *eq_data, USHORT_T eq_data_len);

/**
 * @brief   Swicth the bredr eq mode
 * @param   [in] eq_mode: Switch equalizer mode.
 *          [in] enable: Enable the current mode or not
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_equalizer_switch(UCHAR_T eq_mode, BOOL_T enable);

/**
 * @brief   Control the bredr noise
 * @param   [in] noise_mode: control noise mode.
 *          [in] noise_data: post the noise data.
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_noise_set(UCHAR_T noise_mode, USHORT_T noise_data);

/**
 * @brief   Swicth the bredr noise mode
 * @param   [in] noise_mode: Switch noise mode.
 *          [in] enable: Enable the current mode or not
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_noise_switch(UCHAR_T noise_mode, BOOL_T enable);

/**
 * @brief   Get avrcp connection status
 * @param   [in] p_peer_addr: peer address.
 *
 * @return  connection
 *          disconnection
 * */
BOOL_T tkl_bt_bredr_avrcp_has_connection(TUYA_BT_GAP_ADDR_T *p_peer_addr);

/**
 * @brief   Get hfp connection status
 * @param   [in] p_peer_addr: peer address.
 *
 * @return  connection
 *          disconnection
 * */
BOOL_T tkl_bt_bredr_hfp_has_connection(TUYA_BT_GAP_ADDR_T *p_peer_addr);

/**
 * @brief   Get battery value status
 * @param   [in] battery: battery value
 *
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_bt_bredr_hfp_battery_get(UINT8_T* battery);








/************   hesuping  add ***************/

typedef enum {
    TUYA_BT_BREDR_PAGE_LAST_CONN,
    TUYA_BT_BREDR_PAGE_ALL,
    TUYA_BT_BREDR_PAGE_FROM_ADDR,
} TUYA_BT_BREDR_PAGE_MODE_T;

typedef struct {
    TUYA_BT_BREDR_PAGE_MODE_T  mode;
    TUYA_BT_GAP_ADDR_T addr;
    UINT32_T page_interval_ms;
} TUYA_BT_BREDR_PAGE_T;

OPERATE_RET tkl_bt_bredr_page_start(TUYA_BT_BREDR_PAGE_T* page);

OPERATE_RET tkl_bt_bredr_page_stop();

OPERATE_RET tkl_bt_bredr_pairing_start();
OPERATE_RET tkl_bt_bredr_pairing_stop();
BOOL_T tkl_bt_bredr_is_pairing();

OPERATE_RET tkl_bt_bredr_get_conn_addr(TUYA_BT_GAP_ADDR_T* addr);

OPERATE_RET tkl_bt_bredr_get_linkkey_from_addr(TUYA_BT_GAP_ADDR_T addr, UINT8_T* linkkey);

OPERATE_RET tkl_bt_bredr_delete_bond_info_from_addr(TUYA_BT_GAP_ADDR_T addr);


OPERATE_RET tkl_bt_bredr_pinkey_missing_callback(TUYA_BT_GAP_ADDR_T addr);


OPERATE_RET tkl_bt_bredr_get_device_rssi(TUYA_BT_GAP_ADDR_T addr);

OPERATE_RET tkl_bt_bredr_wave_file_play_start( UINT32_T file_id );

OPERATE_RET tkl_bt_bredr_wave_file_play_stop();



/************************************* AVRCP *********************************************/



/* AV/C pass through control commands */
typedef enum {
    TUYA_BT_BREDR_AVRCP_AVC_OP_VOLUME_UP    = 0x41,
    TUYA_BT_BREDR_AVRCP_AVC_OP_VOLUME_DOWN  = 0x42,
    TUYA_BT_BREDR_AVRCP_AVC_OP_MUTE         = 0x43,
    TUYA_BT_BREDR_AVRCP_AVC_OP_PLAY         = 0x44,
    TUYA_BT_BREDR_AVRCP_AVC_OP_STOP         = 0x45,
    TUYA_BT_BREDR_AVRCP_AVC_OP_PAUSE        = 0x46,
    TUYA_BT_BREDR_AVRCP_AVC_OP_RECORD       = 0x47,
    TUYA_BT_BREDR_AVRCP_AVC_OP_REWIND       = 0x48,
    TUYA_BT_BREDR_AVRCP_AVC_OP_FAST_FORWARD = 0x49,
    TUYA_BT_BREDR_AVRCP_AVC_OP_EJECT        = 0x4a,
    TUYA_BT_BREDR_AVRCP_AVC_OP_NEXT         = 0x4b,
    TUYA_BT_BREDR_AVRCP_AVC_OP_PREV         = 0x4c,
} TUYA_BT_BREDR_AVRCP_PASS_THROUGH_E;


/*
 * Table 4.5: AVRCP Specific operations
 * AV/C VENDOR DEPENDENDENT and Browsing commands
 */
typedef enum {
    /* Capabilities */
    TUYA_BT_BREDR_AVRCP_OP_GET_CAPABILITIES = 0x10,   /* Capabilities */             //获取支持的事件通知

    /* Player Application Settings */
    TUYA_BT_BREDR_AVRCP_OP_ListPlayerApplicationSettingAttributes     = 0x11,       //获取支持的音乐模式信息， 单曲循环，列表循环，随机播放，EQ开关等
    TUYA_BT_BREDR_AVRCP_OP_ListPlayerApplicationSettingValues         = 0x12,
    TUYA_BT_BREDR_AVRCP_OP_GetCurrentPlayerApplicationSettingValue    = 0x13,
    TUYA_BT_BREDR_AVRCP_OP_SetPlayerApplicationSettingValue           = 0x14,
    TUYA_BT_BREDR_AVRCP_OP_GetPlayerApplicationSettingAttributeText   = 0x15,
    TUYA_BT_BREDR_AVRCP_OP_GetPlayerApplicationSettingValueText       = 0x16,

    TUYA_BT_BREDR_AVRCP_OP_InformDisplayableCharacterSet              = 0x17,
    TUYA_BT_BREDR_AVRCP_OP_InformBatteryStatusOfCT                    = 0x18,

    /* Metadata Attributes for Current Media Item */
    TUYA_BT_BREDR_AVRCP_OP_GetElementAttributes                       = 0x20,       // 获取歌曲信息，包括名称、歌手、时长等

    /* Notifications */
    TUYA_BT_BREDR_AVRCP_OP_GetPlayStatus                              = 0x30,
    TUYA_BT_BREDR_AVRCP_OP_RegisterNotification                       = 0x31,

    /* Continuation */
    TUYA_BT_BREDR_AVRCP_OP_RequestContinuingResponse                  = 0x40,
    TUYA_BT_BREDR_AVRCP_OP_AbortContinuingResponse                    = 0x41,

    /* Absolute Volume */
    TUYA_BT_BREDR_AVRCP_OP_SetAbsoluteVolume                          = 0x50,

    /* MediaPlayerSelection */
    TUYA_BT_BREDR_AVRCP_OP_SetAddressedPlayer                         = 0x60,

    /* Browsing */
    TUYA_BT_BREDR_AVRCP_OP_SetBrowsedPlayer                           = 0x70,
    TUYA_BT_BREDR_AVRCP_OP_GetFolderItems                             = 0x71,
    TUYA_BT_BREDR_AVRCP_OP_ChangePath                                 = 0x72,
    TUYA_BT_BREDR_AVRCP_OP_GetItemAttributes                          = 0x73,
    TUYA_BT_BREDR_AVRCP_OP_PlayItem                                   = 0x74,
    TUYA_BT_BREDR_AVRCP_OP_GetTotalNumberOfItems                      = 0x75,

    /* Search */
    TUYA_BT_BREDR_AVRCP_OP_Search                                     = 0x80,

    /* NowPlaying */
    TUYA_BT_BREDR_AVRCP_OP_AddToNowPlaying                            = 0x90,

    /* Error Response */
    TUYA_BT_BREDR_AVRCP_OP_General_Reject                             = 0xA0

} TUYA_BT_BREDR_AVRCP_PDU_ID_E;


typedef struct {
    UINT8_T num;
    UINT8_T attribute_id[];
} avrcp_get_current_player_setting_value_t;


typedef struct {
    UINT8_T num;
    struct {
        UINT8_T attribute_id;
        UINT8_T value_id;
    }setting[];
} avrcp_set_player_setting_value_t;

typedef   avrcp_get_current_player_setting_value_t  avrcp_get_player_setting_attribute_text_t;

typedef struct {
    UINT8_T attribute_id;
    UINT8_T num;
    UINT8_T value_id[];
} avrcp_get_player_setting_value_text_t;

typedef struct {
    UINT8_T num;
    UINT16_T character_set_id[];
} avrcp_inform_display_character_set_t;

//ATTRIBUTE
typedef enum {
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_ALL = 0x0000,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_TITLE,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_ARTIST,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_ALBUM,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_TRACK,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_TOTAL_NUM_ITEMS,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_GENRE,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_SONG_LENGTH_MS,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_DEFAULT_COVER_ART,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_RESERVED = 0x0009,
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTR_NONE = 0x7FFF
} TUYA_BT_BREDR_AVRCP_MEDIA_ATTRIBUTE_ID_E;

/*
 * Appendix H: List of defined notification events
 */
typedef enum {
    TUYA_BT_BREDR_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED                = 0x01,            // 播放状态改变
    TUYA_BT_BREDR_AVRCP_EVENT_TRACK_CHANGED                          = 0x02,            // 歌曲名称改变
    TUYA_BT_BREDR_AVRCP_EVENT_TRACK_REACHED_END                      = 0x03,            // 歌曲到结尾
    TUYA_BT_BREDR_AVRCP_EVENT_TRACK_REACHED_START                    = 0x04,            // 歌曲开始
    TUYA_BT_BREDR_AVRCP_EVENT_PLAYBACK_POS_CHANGED                   = 0x05,            // 播放进度改变
    TUYA_BT_BREDR_AVRCP_EVENT_BATT_STATUS_CHANGED                    = 0x06,            // 电池电量改变
    TUYA_BT_BREDR_AVRCP_EVENT_SYSTEM_STATUS_CHANGED                  = 0x07,            // 系统状态改变
    TUYA_BT_BREDR_AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED     = 0x08,            // 播放器设置改变（EQ, 单曲循环等模式改变）
    TUYA_BT_BREDR_AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED            = 0x09,
    TUYA_BT_BREDR_AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED              = 0x0A,
    TUYA_BT_BREDR_AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED               = 0x0B,
    TUYA_BT_BREDR_AVRCP_EVENT_UIDS_CHANGED                           = 0x0C,
    TUYA_BT_BREDR_AVRCP_EVENT_VOLUME_CHANGED                         = 0x0D,
} TUYA_BT_BREDR_AVRCP_EVENT_E;


typedef struct {
    UINT8_T identifier[8];
    UINT8_T num;
    TUYA_BT_BREDR_AVRCP_MEDIA_ATTRIBUTE_ID_E attribute_id[];
} avrcp_get_element_attributes_t;


typedef struct {
    TUYA_BT_BREDR_AVRCP_EVENT_E event_id;
    UINT32_T   playback_interval;
} avrcp_register_notification_t;

typedef uint8_t avrcp_request_continue_pdu_t;
typedef uint8_t avrcp_abort_continue_res_t;
typedef uint8_t avrcp_set_absolute_volume_t;
typedef UINT16_T avrcp_set_addressed_player_t;
typedef UINT16_T avrcp_set_browsed_player_t;

typedef struct {
    UINT16_T UID;
    UINT8_T  direction;
    UINT8_T  folder_uid[8];
} avrcp_change_path_t;

typedef struct {
    UINT8_T scope;
    UINT32_T start_item;
    UINT32_T end_item;
    UINT8_T  attribute_cnt;
    UINT8_T  attribute_list[];
} avrcp_get_folder_items_t;

typedef struct {
    UINT8_T scope;
    UINT8_T UID[8];
    UINT16_T counter;
    UINT8_T  num;
    UINT8_T  attribute_id[4];
} avrcp_get_item_attributes_t;

typedef struct {
    UINT8_T scope;
    UINT8_T UID[8];
    UINT16_T counter;
} avrcp_play_item_t;

typedef UINT8_T  avrcp_get_total_num_of_items_t;

typedef struct {
    UINT16_T id;
    UINT16_T length;
    UINT8_T  search_str[];
} avrcp_search_t;

typedef struct {
    UINT8_T scope;
    UINT8_T UID[8];
    UINT16_T counter;
}avrcp_add_now_playing_t;

typedef enum {
    TUYA_BT_BREDR_AVRCP_APPLICATION_ILLEGAL   = 0x00,
    TUYA_BT_BREDR_AVRCP_APPLICATION_EQUALIZER = 0x01,
    TUYA_BT_BREDR_AVRCP_APPLICATION_REPEAT    = 0x02,
    TUYA_BT_BREDR_AVRCP_APPLICATION_SHUFFLE   = 0x03,
    TUYA_BT_BREDR_AVRCP_APPLICATION_SCAN      = 0x04
} TUYA_BT_BREDR_AVRCP_APPLICATION_SETTINGS_AND_VALUES_E;

typedef enum {
    TUYA_BT_BREDR_AVRCP_CAPABILITY_ID_COMPANY = 0x02,
    TUYA_BT_BREDR_AVRCP_CAPABILITY_ID_EVENT = 0x03
} TUYA_BT_BREDR_AVRCP_CAPABILITY_ID_E;

/* Battery status */
typedef enum {
    TUYA_bT_BREDR_AVRCP_BATTERY_STATUS_NORMAL                        = 0x00,
    TUYA_bT_BREDR_AVRCP_BATTERY_STATUS_WARNING                       = 0x01,
    TUYA_bT_BREDR_AVRCP_BATTERY_STATUS_CRITICAL                      = 0x02,
    TUYA_bT_BREDR_AVRCP_BATTERY_STATUS_EXTERNAL                      = 0x03,
    TUYA_bT_BREDR_AVRCP_BATTERY_STATUS_FULL_CHARGE                   = 0x04
} TUYA_BT_BREDR_AVRCP_BATTERY_STATUS_E;

typedef union {
        TUYA_BT_BREDR_AVRCP_CAPABILITY_ID_E capability_id;
        TUYA_BT_BREDR_AVRCP_APPLICATION_SETTINGS_AND_VALUES_E attribute_id;
        avrcp_get_current_player_setting_value_t    player_setting_value;
        avrcp_set_player_setting_value_t            player_id_value;
        avrcp_get_player_setting_attribute_text_t    attribute_text;
        avrcp_get_player_setting_value_text_t        value_text;
        avrcp_inform_display_character_set_t         display_character;
        TUYA_BT_BREDR_AVRCP_BATTERY_STATUS_E         battery_status;
        avrcp_get_element_attributes_t               element_attributes;
        avrcp_register_notification_t                notification;
        avrcp_request_continue_pdu_t                 continue_pdu_id;
        avrcp_abort_continue_res_t                   continue_abort_pdu_id;
        avrcp_set_absolute_volume_t                  absolute_volume;
        avrcp_set_addressed_player_t                 addressed_player_id;
        avrcp_set_browsed_player_t                   browsed_player_id;
        
        avrcp_get_folder_items_t                    folder_items;             //Browsing cmd
        avrcp_change_path_t                         change_path;
        avrcp_get_item_attributes_t                 item_attributes;
        avrcp_play_item_t                           play_item;
        avrcp_get_total_num_of_items_t              num_of_items;

        avrcp_search_t                              search;                     // Search

        avrcp_add_now_playing_t                     now_playing;

}tuya_bt_bredr_avrcp_vendor_pdu_u;

typedef  UINT8_T avrcp_playback_status_changed;
typedef  UINT8_T avrcp_track_changed[8];
typedef  UINT32_T avrcp_playback_pos_changed;
typedef  UINT8_T avrcp_batt_status_changed;
typedef  UINT8_T avrcp_system_status_changed;
typedef  UINT16_T avrcp_uids_changed;
typedef  UINT8_T avrcp_volume_changed;

typedef struct {
    UINT8_T     attr_n;         /* num of attributes */

    /* TODO: consider implementing an array of id/val pairs */
    UINT8_T     attr_id;        /* 1st attribute id */
    UINT8_T     attr_val;       /* 1st attribute value */
    /* ... and so on for the number of attributes specified by attr_n */
} avrcp_player_application_setting_changed;


typedef struct {
    UINT16_T    player_id;
    UINT16_T    uid_counter;
} avrcp_addressed_players_changed;



/* RegisterNotification response */
typedef union {
        /* EVENT_PLAYBACK_STATUS_CHANGED */
        avrcp_playback_status_changed     play_status;

        /* EVENT_TRACK_CHANGED */
        avrcp_track_changed    cur_track_index;    /* if no track currently selected,
                                                    * return 0xf..ff in the INTERIM
                                                    * response */

        /* EVENT_TRACK_REACHED_END - no params */
        /* EVENT_TRACK_REACHED_START - no params */

        /* EVENT_PLAYBACK_POS_CHANGED */
        avrcp_playback_pos_changed    playback_pos;

        /* EVENT_BATT_STATUS_CHANGED */
        avrcp_batt_status_changed     batt_status;

        /* EVENT_SYSTEM_STATUS_CHANGED */
        avrcp_system_status_changed     system_status;

        /* EVENT_PLAYER_APPLICATION_SETTING_CHANGED */
        avrcp_player_application_setting_changed  application_setting;

        /* EVENT_NOW_PLAYING_CONTENT_CHANGED - no params */
        /* EVENT_AVAILABLE_PLAYERS_CHANGED - no params */

        /* EVENT_ADDRESSED_PLAYER_CHANGED */
        avrcp_addressed_players_changed  addressed_players;

          /* EVENT_UIDS_CHANGED */
        avrcp_uids_changed    uid_counter;

          /* EVENT_VOLUME_CHANGED */
        avrcp_volume_changed     volume;
} tuya_bt_bredr_avrcp_event_res_u;


/* GetCapabilities response */
typedef struct {
    UINT8_T     id;
    UINT8_T     cap_count;
    UINT8_T     capability[];
    /* Followed by an array of company or event ids */
} avrcp_get_caps_res_t;

typedef struct {
    UINT8_T num;
    UINT8_T values[];
} avrcp_list_player_setting_values_rsp_t;

/* GetPlayStatus response */
typedef struct {
    UINT32_T            song_length;    /* in milliseconds */
    UINT32_T            song_position;  /* in milliseconds (elapsed time) */
    UINT8_T             play_status;    /* AVRCP_PLAY_STATUS_... */
} avrcp_get_play_status_res_t;

typedef avrcp_get_current_player_setting_value_t avrcp_list_player_setting_attributes_rsp_t;

typedef struct {
    UINT8_T num;
    UINT8_T attribute_id[];
}avrcp_get_cur_player_setting_value_rsp_t;

typedef struct {
    UINT8_T num;

    struct {
        UINT8_T attribute_id;
        UINT16_T character_set_id;
        UINT8_T attribute_str_len;
        UINT8_T* str;
    }text;
} avrcp_get_player_setting_attr_text_rsp_t;

typedef struct {
    UINT8_T num;

    struct {
        UINT8_T value_id;
        UINT16_T character_set_id;
        UINT8_T value_str_len;
        UINT8_T *str;
    }text[];
} avrcp_get_player_setting_value_text_rsp_t;

typedef struct {
    UINT8_T num;
    UINT32_T attribute_id;
    UINT16_T character_set_id;
    UINT16_T length;
    UINT8_T *atttr_value;
} avrcp_get_element_attributes_rsp_t;

typedef UINT8_T avrcp_set_addr_player_rsp_t;
typedef UINT8_T avrcp_set_browsed_player_rsp_t;
typedef UINT8_T avrcp_absolute_volume_t;

typedef struct {
    UINT8_T status;
    UINT32_T num;
} avrcp_change_path_rsp_t;

typedef struct {
    UINT8_T status;
    UINT16_T UID;
    UINT16_T num;
    UINT8_T* list;
} avrcp_get_folder_items_rsp_t;

typedef struct {
    UINT8_T status;
    UINT8_T num;
    UINT8_T entry[];
} avrcp_get_items_attr_rsp_t;

typedef struct {
    UINT8_T status;
    UINT16_T UID;
    UINT32_T num;
} avrcp_get_total_num_items_rsp_t;

typedef uint8_t   avrcp_play_item_rsp_t;
typedef uint8_t   avrcp_add_to_now_playing_rsp_t;

typedef avrcp_get_total_num_items_rsp_t  avrcp_search_rsp_t;

typedef union {
        uint32_t  company_id;
        avrcp_list_player_setting_attributes_rsp_t          list_player_attributes;
        avrcp_list_player_setting_values_rsp_t              list_player_values;
        avrcp_get_cur_player_setting_value_rsp_t            current_player_value;
        avrcp_get_player_setting_attr_text_rsp_t            player_attribute_text;
        avrcp_get_player_setting_value_text_rsp_t           player_value_text;
        avrcp_get_element_attributes_rsp_t                  element_attr;

        avrcp_get_play_status_res_t                         play_status;

        avrcp_set_addr_player_rsp_t                 addr_player_status;
        avrcp_set_browsed_player_rsp_t              browsed_player_status;

        avrcp_absolute_volume_t                  absolute_volume;
        avrcp_change_path_rsp_t                     change_path;

        avrcp_get_folder_items_rsp_t                folder_items;
        avrcp_get_items_attr_rsp_t                  items_attr;
        avrcp_get_total_num_items_rsp_t             total_num_items;
        avrcp_search_rsp_t                           search_rsp;


        avrcp_play_item_rsp_t                        play_item_status;
        avrcp_add_to_now_playing_rsp_t              now_playing_status;
        avrcp_add_now_playing_t                     now_playing;

} tuya_bt_bredr_avrcp_vendor_pdu_rsp_u;



typedef enum{
    TUYA_BT_BREDR_AVRCP_ROLE_CONTROLLER = 0x01,
    TUYA_BT_BREDR_AVRCP_ROLE_TARGET
} TUYA_BT_BREDR_AVRCP_ROLE_E;


// typedef OPERATE_RET (*tkl_bt_bredr_avrcp_unit_info_response_cb)(UINT8_T* company_id,  UINT16_T len);
// typedef OPERATE_RET (*tkl_bt_bredr_avrcp_subunit_info_response_cb)(UINT8_T* payload,  UINT16_T len);

typedef OPERATE_RET (*tkl_bt_bredr_avrcp_vendor_dependent_response_cb)(TUYA_BT_BREDR_AVRCP_PDU_ID_E pdu_id, tuya_bt_bredr_avrcp_vendor_pdu_rsp_u* pdu_rs);
typedef OPERATE_RET (*tkl_bt_bredr_avrcp_pass_through_response_cb)(TUYA_BT_BREDR_AVRCP_PASS_THROUGH_E opcode, UINT8_T resopnse);
typedef OPERATE_RET (*tkl_bt_bredr_avrcp_event_cb)(TUYA_BT_BREDR_AVRCP_EVENT_E event_id, tuya_bt_bredr_avrcp_event_res_u* event_data);

typedef struct {
    void (*connected_cb)(TUYA_BT_GAP_ADDR_T addr);
    void (*disconnected_cb)(UINT8_T reason);
} tuya_bt_bredr_avrc_conn_cbs_t;

typedef struct {
    TUYA_BT_BREDR_AVRCP_ROLE_E role;
    // VOID_T*  p_session;  // 多连接？
    tuya_bt_bredr_avrc_conn_cbs_t  conn_cbs;
    tkl_bt_bredr_avrcp_vendor_dependent_response_cb vendor_dependent_response_handler;
    tkl_bt_bredr_avrcp_pass_through_response_cb   pass_through_response_handler;
    tkl_bt_bredr_avrcp_event_cb event_handler;
    
} TUYA_BT_BREDR_AVRCP_CONTEXT_T;




/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/


/**
 * @brief tkl_bt_bredr_avrcp_init
 *
 * @param context: avrcp context struct
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_bredr_avrcp_init(TUYA_BT_BREDR_AVRCP_CONTEXT_T*  context);

/**
 * @brief tkl_bt_bredr_avrcp_uninit
 *
 * @param context: avrcp context struct
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_bredr_avrcp_uninit();

// /**
//  * @brief function description
//  *
//  * @param none
//  *
//  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
//  */
// OPERATE_RET tkl_bt_bredr_avrcp_controller_unit_info();


// /**
//  * @brief function description
//  *
//  * @param none
//  *
//  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
//  */
// OPERATE_RET tkl_bt_bredr_avrcp_controller_subunit_info();

/**
 * @brief avrcp passthrough cmd
 *
 * @param opcode
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_bredr_avrcp_send_pass_through_cmd(TUYA_BT_BREDR_AVRCP_PASS_THROUGH_E opcode);

/**
 * @brief avrcp sepcification cmd
 *
 * @param pdu_id
 * @param payload
 * @param len
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_bredr_avrcp_send_vendor_dependent_cmd(TUYA_BT_BREDR_AVRCP_PDU_ID_E pdu_id, tuya_bt_bredr_avrcp_vendor_pdu_u* pdu_data);

BOOL_T tkl_bt_bredr_avrcp_has_connection(TUYA_BT_GAP_ADDR_T *p_peer_addr);
OPERATE_RET tkl_bt_bredr_avrcp_connect(TUYA_BT_GAP_ADDR_T *p_peer_addr);
OPERATE_RET tkl_bt_bredr_avrcp_disconnect();

OPERATE_RET tkl_bt_bredr_avrcp_set_volume(UINT8_T volume);

UINT8_T tkl_bt_bredr_avrcp_get_volume();

extern tkl_bt_bredr_avrcp_vendor_dependent_response_cb  tkl_bt_bredr_avrcp_vendor_dependent_response_func;
extern tkl_bt_bredr_avrcp_pass_through_response_cb     tkl_bt_bredr_avrcp_pass_through_response_func;
extern tkl_bt_bredr_avrcp_event_cb     tkl_bt_bredr_avrcp_event_func;
extern tuya_bt_bredr_avrc_conn_cbs_t  tuya_bt_bredr_avrc_conn_func;



/************************************************** A2DP **********************************************/

typedef enum{
    A2DP_SBC_48000 = 0x10,
    A2DP_SBC_44100 = 0x20,
    A2DP_SBC_32000 = 0x40,
    A2DP_SBC_16000 = 0x80,
} TUYA_BT_BREDR_A2DP_SBC_SAMPLING_FREQUENCY_E;

typedef enum{
    A2DP_SBC_JOINT_STEREO  = 0x1,
    A2DP_SBC_STEREO        = 0x2,
    A2DP_SBC_DUAL_CHANNEL  = 0x4,
    A2DP_SBC_MONO          = 0x8
} TUYA_BT_BREDR_A2DP_SBC_CHANNEL_MODE_E;

typedef enum{
    A2DP_SBC_BLOCK_LENGTH_16 = 0x10,
    A2DP_SBC_BLOCK_LENGTH_12 = 0x20,
    A2DP_SBC_BLOCK_LENGTH_8  = 0x40,
    A2DP_SBC_BLOCK_LENGTH_4  = 0x80,
} TUYA_BT_BREDR_A2DP_SBC_BLOCK_LENGTH_E;

typedef enum{
    A2DP_SBC_SUBBANDS_8 = 0x4,
    A2DP_SBC_SUBBANDS_4 = 0x8,
} TUYA_BT_BREDR_A2DP_SBC_SUBBAND_E;

typedef enum{
    A2DP_SBC_ALLOCATION_METHOD_LOUDNESS = 1,
    A2DP_SBC_ALLOCATION_METHOD_SNR      = 2,
} TUYA_BT_BREDR_A2DP_SBC_ALLOCATION_METHOD_E;


typedef struct {
    TUYA_BT_BREDR_A2DP_SBC_SAMPLING_FREQUENCY_E     sampling_frequency;
    TUYA_BT_BREDR_A2DP_SBC_CHANNEL_MODE_E           channel_mode;
    TUYA_BT_BREDR_A2DP_SBC_BLOCK_LENGTH_E           block_length;
    TUYA_BT_BREDR_A2DP_SBC_SUBBAND_E                subbands;
    TUYA_BT_BREDR_A2DP_SBC_ALLOCATION_METHOD_E      allocation_method;
    UINT8_T   min_bitpool_value;
    UINT8_T   max_bitpool_value;
} TUYA_BT_BREDR_A2DP_SBC_CAPS_T;


typedef enum {
    TUYA_BT_BREDR_A2DP_CONNECTED,
    TUYA_BT_BREDR_A2DP_DISCONNECTED,

    TUYA_BT_BREDR_A2DP_EVENT_CODEC_SBC_CONFIGURATION,
    TUYA_BT_BREDR_A2DP_EVENT_CODEC_MPEG_AUDIO_CONFIGURATION,
    TUYA_BT_BREDR_A2DP_EVENT_CODEC_MPEG_AAC_CONFIGURATION,
    TUYA_BT_BREDR_A2DP_EVENT_CODEC_ATRAC_CONFIGURATION,
    TUYA_BT_BREDR_A2DP_EVENT_CODEC_OTHER_CONFIGURATION,

    TUYA_BT_BREDR_A2DP_EVENT_STREAM_ESTABLISHED,
    TUYA_BT_BREDR_A2DP_EVENT_STREAM_STARTED,
    TUYA_BT_BREDR_A2DP_EVENT_STREAM_SUSPENDED,
    TUYA_BT_BREDR_A2DP_EVENT_STREAM_STOPPED,
    TUYA_BT_BREDR_A2DP_EVENT_STREAM_RELEASED,
} TUYA_BT_BREDR_A2DP_EVENT_E;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/


typedef enum {
    TUYA_BT_BREDR_A2DP_ROLE_SINK =0x01,
    TUYA_BT_BREDR_A2DP_ROLE_SOURCE
} TUYA_BT_BREDR_A2DP_ROLE_E;

typedef OPERATE_RET (*tkl_bt_bredr_a2dp_event_cb)(TUYA_BT_BREDR_A2DP_EVENT_E evnet_id, UINT8_T* data, UINT16_T length);

extern tkl_bt_bredr_a2dp_event_cb tkl_bt_bredr_a2dp_event_func;

typedef struct {
    TUYA_BT_BREDR_A2DP_ROLE_E role;
    // TUYA_BT_BREDR_A2DP_SBC_CAPS_T  SBC;  // sink端可配置参数较少, 去掉
    tkl_bt_bredr_a2dp_event_cb event_handler;
} TUYA_BT_BREDR_A2DP_CONTEXT_T;



OPERATE_RET tkl_bt_bredr_a2dp_init(TUYA_BT_BREDR_A2DP_CONTEXT_T* context);
OPERATE_RET tkl_bt_bredr_a2dp_uninit();

OPERATE_RET tkl_bt_bredr_a2dp_connect(TUYA_BT_GAP_ADDR_T *addr);

OPERATE_RET tkl_bt_bredr_a2dp_disconnect();

BOOL_T tkl_bt_bredr_a2dp_has_connection(TUYA_BT_GAP_ADDR_T *p_peer_addr);



/**************************************************  HFP **********************************************/

typedef enum {
    TUYA_BT_BREDR_HF_AT_BRSF,                      //"AT+BRSF="
    TUYA_BT_BREDR_HF_AT_BAC,                       //"AT+BAC="
    TUYA_BT_BREDR_HF_AT_CONFIRM_CODEC,             //"AT+BCS="
    TUYA_BT_BREDR_HF_AT_CIND_TEST,                 //"AT+CIND=?"
    TUYA_BT_BREDR_HF_AT_CIND,                      //"AT+CIND?"
    TUYA_BT_BREDR_HF_AT_CMER_ENABLE,               //"AT+CMER=3,0,0,1"
    TUYA_BT_BREDR_HF_AT_CMER_DISABLE,              //"AT+CMER=3,0,0,0"
    TUYA_BT_BREDR_HF_AT_CHLD_TEST,                 //"AT+CHLD=?"
    TUYA_BT_BREDR_HF_AT_CHLD,                      //"AT+CHLD="     // 三方通话设置
    TUYA_BT_BREDR_HF_AT_BIND,                     //"AT+BIND="
    TUYA_BT_BREDR_HF_AT_BIND_TEST,                //"AT+BIND?"
    TUYA_BT_BREDR_HF_AT_BIEV,                     //"AT+BIEV="
    TUYA_BT_BREDR_HF_AT_BCC,                       //"AT+BCC"
    TUYA_BT_BREDR_HF_AT_CMEE,                     //"AT+CMEE="
    TUYA_BT_BREDR_HF_AT_ATA,                       //"ATA"                  接听
    TUYA_BT_BREDR_HF_AT_CHUP,                      //"AT+CHUP"              拒接/挂断
    TUYA_BT_BREDR_HF_AT_ATD,                       //"ATD"                  主动拨号
    TUYA_BT_BREDR_HF_AT_BLDN,                      //"AT+BLDN"              拨打最后一次通话号码
    TUYA_BT_BREDR_HF_AT_VTS,                       //"AT+VTS="              通话中按键选择  TRANSMIT_DTMF_CODES
    TUYA_BT_BREDR_HF_AT_CLCC,                      //"AT+CLCC"              查询通话列表
    TUYA_BT_BREDR_HF_AT_COPS_F,                    //"AT+COPS=3,0"          设置AG端信号格式
    TUYA_BT_BREDR_HF_AT_COPS,                      //"AT+COPS?"             获取AG端信号
    TUYA_BT_BREDR_HF_AT_CLIP,                      //"AT+CLIP="             来电显示
    TUYA_BT_BREDR_HF_AT_CCWA,                      //"AT+CCWA="             通话中来电，显示来电号码
    TUYA_BT_BREDR_HF_AT_NREC_DISABLE,              //"AT+NREC=0"            关闭降噪和回声
    TUYA_BT_BREDR_HF_AT_BVRA,                      //"AT+BVRA="             语音识别
    TUYA_BT_BREDR_HF_AT_BINP,                      //"AT+BINP=1"
    TUYA_BT_BREDR_HF_AT_VGM,                       //"AT+VGM="              设置mic增益
    TUYA_BT_BREDR_HF_AT_VGS,                       //"AT+VGS="              设置喇叭增益
    TUYA_BT_BREDR_HF_AT_CNUM,                      //"AT+CNUM"              获取本地号码
    TUYA_BT_BREDR_HF_AT_BIA,                       //"AT+BIA="
    TUYA_BT_BREDR_HF_AT_BTRH,                      //"AT+BTRH="             挂起来电
    TUYA_BT_BREDR_HF_AT_BTRH_READ,                 //"AT+BTRH?"

    TUYA_BT_BREDR_HF_AT_CCEV,                      // report hf battery
} TUYA_BT_BREDR_HFP_AT_CMD_E;

typedef UINT8_T hfp_at_cmee_t;
typedef UINT8_T hfp_at_clip_t;
typedef UINT8_T hfp_at_binp_t;
typedef UINT8_T hfp_at_vts_t;
typedef UINT8_T hfp_at_vgm_t;
typedef UINT8_T hfp_at_vgs_t;
typedef UINT8_T hfp_at_btrh_t;
typedef UINT8_T hfp_at_vgs_t;
typedef UINT8_T hfp_at_vgs_t;
typedef UINT8_T* hfp_at_atd_call_number_t;
typedef UINT8_T hfp_at_atd_call_memory_t;
typedef struct {
    UINT8_T indicator;
    UINT8_T value;
}hfp_at_biev_t;

typedef enum {
    TUYA_BT_BREDR_HFP_VOICE_RECOGNITION_DISABLE_IN_AG = 0x00,
    TUYA_BT_BREDR_HFP_VOICE_RECOGNITION_ENABLE_IN_AG   = 0x01,
}TUYA_BT_BREDR_HFP_RESULT_BVRA_E;

typedef enum {
    HFP_CHLD_RELEASE_ALL_HELD               = 0, /* Release all held calls or set
                                                   UDUB for a waiting call */
    HFP_CHLD_RELEASE_ALL_ACTIVE_ACCEPT_HELD = 1, /* Release all active calls and
                                                   accept the other call */
    HFP_CHLD_RELEASE_INDICATED_ACTIVE       = 1, /* Release the specified active
                                                   call */
    HFP_CHLD_HOLD_ALL_ACTIVE_ACCEPT_HELD    = 2, /* Place all active calls on
                                                   hold and accept other call */
    HFP_CHLD_HOLD_ALL_EXCEPT_INDICATED      = 2, /* Request private consultation
                                                   mode with the specified call
                                                 */
    HFP_CHLD_MULTIPARTY                     = 3, /* Add held call to conversation
                                                   -- place all calls on hold
                                                   except the specified call */
    HFP_CHLD_EXPLICIT_CALL_TRANSFER         = 4  /* Connect 2 calls & disconnect
                                                   subscriber from both calls
                                                   (Explicit Call Transfer) */
} TUYA_BT_BREDR_HFP_CMD_CHLD_E;

typedef struct {
    TUYA_BT_BREDR_HFP_CMD_CHLD_E cmd;
     UINT32_T call_id;
} hfp_at_chld_t;

typedef union {
        hfp_at_cmee_t  cmee;
        // three_call
        hfp_at_chld_t  chld;
        hfp_at_clip_t  clip;
        hfp_at_binp_t  binp;
        hfp_at_vts_t   vts;
        hfp_at_vgm_t   vgm;
        hfp_at_vgs_t   vgs;
        hfp_at_btrh_t  btrh;
        hfp_at_biev_t  biev;
        hfp_at_atd_call_number_t  atd_num;
        hfp_at_atd_call_memory_t  atd_memory;
        UINT8_T  value;

        TUYA_BT_BREDR_HFP_RESULT_BVRA_E bvra;
        // voice recog
} tuya_bt_avrcp_hfp_cmd_u;


typedef enum {
    TUYA_BT_BREDR_HFP_ROLE_INVALID = 0,
    TUYA_BT_BREDR_HFP_ROLE_AG,
    TUYA_BT_BREDR_HFP_ROLE_HF,
} TUYA_BT_BREDR_HFP_ROLE_E;

// typedef enum {
//     TUYA_BT_BREDR_HFP_CMD_NONE = 0,
//     TUYA_BT_BREDR_HFP_CMD_ERROR,
//     TUYA_BT_BREDR_HFP_CMD_UNKNOWN,
//     TUYA_BT_BREDR_HFP_CMD_OK,
//     TUYA_BT_BREDR_HFP_CMD_RING,
//     TUYA_BT_BREDR_HFP_CMD_SUPPORTED_FEATURES,                             // 5
//     TUYA_BT_BREDR_HFP_CMD_AVAILABLE_CODECS,
//     TUYA_BT_BREDR_HFP_CMD_RETRIEVE_AG_INDICATORS_GENERIC,
//     TUYA_BT_BREDR_HFP_CMD_RETRIEVE_AG_INDICATORS,
//     TUYA_BT_BREDR_HFP_CMD_RETRIEVE_AG_INDICATORS_STATUS, 
//     TUYA_BT_BREDR_HFP_CMD_ENABLE_INDICATOR_STATUS_UPDATE,                 // 10
//     TUYA_BT_BREDR_HFP_CMD_ENABLE_INDIVIDUAL_AG_INDICATOR_STATUS_UPDATE,
//     TUYA_BT_BREDR_HFP_CMD_SUPPORT_CALL_HOLD_AND_MULTIPARTY_SERVICES,
//     TUYA_BT_BREDR_HFP_CMD_ENABLE_CLIP,
//     TUYA_BT_BREDR_HFP_CMD_AG_SENT_CLIP_INFORMATION,
//     TUYA_BT_BREDR_HFP_CMD_ENABLE_CALL_WAITING_NOTIFICATION,               // 15
//     TUYA_BT_BREDR_HFP_CMD_AG_SENT_CALL_WAITING_NOTIFICATION_UPDATE,
//     TUYA_BT_BREDR_HFP_CMD_LIST_GENERIC_STATUS_INDICATORS,
//     TUYA_BT_BREDR_HFP_CMD_RETRIEVE_GENERIC_STATUS_INDICATORS,
//     TUYA_BT_BREDR_HFP_CMD_RETRIEVE_GENERIC_STATUS_INDICATORS_STATE,
//     TUYA_BT_BREDR_HFP_CMD_SET_GENERIC_STATUS_INDICATOR_STATUS,            // 20
//     TUYA_BT_BREDR_HFP_CMD_TRANSFER_AG_INDICATOR_STATUS,
//     TUYA_BT_BREDR_HFP_CMD_QUERY_OPERATOR_SELECTION_NAME,
//     TUYA_BT_BREDR_HFP_CMD_QUERY_OPERATOR_SELECTION_NAME_FORMAT,
//     TUYA_BT_BREDR_HFP_CMD_ENABLE_EXTENDED_AUDIO_GATEWAY_ERROR,
//     TUYA_BT_BREDR_HFP_CMD_EXTENDED_AUDIO_GATEWAY_ERROR,                   // 25
//     TUYA_BT_BREDR_HFP_CMD_TRIGGER_CODEC_CONNECTION_SETUP,
//     TUYA_BT_BREDR_HFP_CMD_AG_SEND_COMMON_CODEC,
//     TUYA_BT_BREDR_HFP_CMD_AG_SUGGESTED_CODEC,
//     TUYA_BT_BREDR_HFP_CMD_HF_CONFIRMED_CODEC,                             
//     TUYA_BT_BREDR_HFP_CMD_CALL_ANSWERED,                                  // 30
//     TUYA_BT_BREDR_HFP_CMD_CALL_HOLD,
//     TUYA_BT_BREDR_HFP_CMD_HANG_UP_CALL,
//     TUYA_BT_BREDR_HFP_CMD_CHANGE_IN_BAND_RING_TONE_SETTING,
//     TUYA_BT_BREDR_HFP_CMD_CALL_PHONE_NUMBER,
//     TUYA_BT_BREDR_HFP_CMD_REDIAL_LAST_NUMBER,                             // 35            
//     TUYA_BT_BREDR_HFP_CMD_TURN_OFF_EC_AND_NR,     
//     TUYA_BT_BREDR_HFP_CMD_AG_ACTIVATE_VOICE_RECOGNITION,                  // 37
//     TUYA_BT_BREDR_HFP_CMD_HF_ACTIVATE_VOICE_RECOGNITION,
//     TUYA_BT_BREDR_HFP_CMD_HF_REQUEST_PHONE_NUMBER,
//     TUYA_BT_BREDR_HFP_CMD_AG_SENT_PHONE_NUMBER,
//     TUYA_BT_BREDR_HFP_CMD_TRANSMIT_DTMF_CODES,
//     TUYA_BT_BREDR_HFP_CMD_SET_MICROPHONE_GAIN,
//     TUYA_BT_BREDR_HFP_CMD_SET_SPEAKER_GAIN,
//     TUYA_BT_BREDR_HFP_CMD_GET_SUBSCRIBER_NUMBER_INFORMATION,
//     TUYA_BT_BREDR_HFP_CMD_LIST_CURRENT_CALLS,
//     TUYA_BT_BREDR_HFP_CMD_RESPONSE_AND_HOLD_QUERY,
//     TUYA_BT_BREDR_HFP_CMD_RESPONSE_AND_HOLD_COMMAND,
//     TUYA_BT_BREDR_HFP_CMD_RESPONSE_AND_HOLD_STATUS,
//     TUYA_BT_BREDR_HFP_CMD_HF_INDICATOR_STATUS,
//     TUYA_BT_BREDR_HFP_CMD_CUSTOM_MESSAGE
// } TUYA_BT_BREDR_HFP_COMMAND_E;

typedef struct {
    UINT8_T    number[32];           /* Phone number, as a double-quoted string*/
    UINT16_T type;                   /* Phone number format integer; -1 if N/A */
} TUYA_BT_BREDR_HFP_RESULT_CCWA_E;

typedef enum {
    TUYA_BT_BREDR_HFP_CODEC_CVSD  = 0x01,
    TUYA_BT_BREDR_HFP_CODEC_MSBC  = 0x02
}TUYA_BT_BREDR_HFP_RESULT_CODEC_E;

typedef enum {
    TUYA_BT_BREDR_HFP_NO_INBAND_RING_TONE = 0x00,
    TUYA_BT_BREDR_HFP_INBAND_RING_TON   = 0x01
}TUYA_BT_BREDR_HFP_RESULT_BAND_RING_TONE_E;

typedef enum {
    TUYA_BT_BREDR_HFP_BRTH_INCOMING_HOLD     = 0x00,
    TUYA_BT_BREDR_HFP_BRTH_INCOMING_ACCEPT   = 0x01,
    TUYA_BT_BREDR_HFP_BRTH_INCOMING_REJECT   = 0x02
}TUYA_BT_BREDR_HFP_RESULT_BRTH_E;



typedef enum {
    TUYA_BT_BREDR_HFP_CHLD_0      =  (1 << 0),  // Releases all held calls or sets UDUB for a waiting call
    TUYA_BT_BREDR_HFP_CHLD_1      =  (1 << 1),     // Releases all active calls (if any exist) and accepts the other (held or waiting) call
    TUYA_BT_BREDR_HFP_CHLD_1x     =  (1 << 2),      // Releases call with specified index (<idx>)
    TUYA_BT_BREDR_HFP_CHLD_2      =  (1 << 3), // Places all active calls (if any exist) on hold and accepts the other (held or waiting) call
    TUYA_BT_BREDR_HFP_CHLD_2x     =  (1 << 4), // Request private consultation mode with specified call (<idx>).
    TUYA_BT_BREDR_HFP_CHLD_3      =  (1 << 5), // Adds a held call to the conversation.
    TUYA_BT_BREDR_HFP_CHLD_4      =  (1 << 6), // ects the two calls and disconnects the subscriber from both calls 
}TUYA_BT_BREDR_HFP_RESULT_CHLD_E;


typedef enum {
    HFP_CALL_STATUS_NO_HELD_OR_ACTIVE_CALLS = 0,
    HFP_CALL_STATUS_ACTIVE_OR_HELD_CALL_IS_PRESENT
} TUYA_BT_BREDR_HFP_CALL_STATE_E;

typedef enum {  
    HFP_CALLSETUP_STATUS_NO_CALL_SETUP_IN_PROGRESS = 0, 
    HFP_CALLSETUP_STATUS_INCOMING_CALL_SETUP_IN_PROGRESS,
    HFP_CALLSETUP_STATUS_OUTGOING_CALL_SETUP_IN_DIALING_STATE,
    HFP_CALLSETUP_STATUS_OUTGOING_CALL_SETUP_IN_ALERTING_STATE
} TUYA_BT_BREDR_HFP_CALLSETUP_STATE_E;

typedef enum {
    HFP_CALLHELD_STATUS_NO_CALLS_HELD = 0,
    HFP_CALLHELD_STATUS_CALL_ON_HOLD_OR_SWAPPED,
    HFP_CALLHELD_STATUS_CALL_ON_HOLD_AND_NO_ACTIVE_CALLS 
} TUYA_BT_BREDR_HFP_CALLHELD_STATE_E;


/* AG Call-Indicator Codes */
/* The comments list the status values supported for each indicator. */
typedef enum {
    TUYA_BT_BREDR_HFP_CIND_SERVICE = 0,       /* Service availability indicator:
                               0 - Implies no home/roam network service
                               1 - Implies available home/roam network service
                             */
    TUYA_BT_BREDR_HFP_CIND_CALL,              /* Standard call status indicator:
                               0 - There are no calls in progress.
                               1 - There is at least one call in progress.
                             */
    TUYA_BT_BREDR_HFP_CIND_CALLSETUP,         /* Bluetooth propriety call-setup status indicator:
                               0 - Not currently in call set-up
                               1 - Ongoing incoming call process
                               2 - Ongoing outgoing call set-up
                               3 - Remote party being alerted in outgoing call
                             */
    TUYA_BT_BREDR_HFP_CIND_CALLHELD,          /* Bluetooth process call-hold status indicator:
                               0 - No held calls
                               1 - Call is placed on hold, or active/held calls
                                   are swapped; the AG has both an active and a
                                   held call.
                               2 - Call on hold, no active call
                             */
    TUYA_BT_BREDR_HFP_CIND_SIGNAL,            /* Signal strength indicator: 0--5 */
    TUYA_BT_BREDR_HFP_CIND_ROAM,              /* Roaming status indicator:
                               0 - Roaming is not active
                               1 - Roaming is active
                             */
    TUYA_BT_BREDR_HFP_CIND_BATTCHG,           /* Battery charge indicator: 0--5 */
    /* The following codes are not covered by the HFP specification: */
    TUYA_BT_BREDR_HFP_CIND_MESSAGE,           /* Message indicator:
                               0 - No message received
                               1 - Message received
                             */
    TUYA_BT_BREDR_HFP_CIND_BATTERYWARNING,    /* Battery warning indicator:
                               0 - No battery warning
                               1 - Battery warning
                            */
    TUYA_BT_BREDR_HFP_CIND_CHARGERCONNECTED,  /* Charger connection indicator:
                               0 - Charger is not connected
                               1 - Charger is connected
                             */
    TUYA_BT_BREDR_HFP_CIND_LAST
} TUYA_BT_BREDR_HFP_CIND_CODE_E;

typedef struct {
    TUYA_BT_BREDR_HFP_CIND_CODE_E  cind_code;  /* AG indicator code */
    INT8_T                         val;        /* Current status of the AG indicator */
} TUYA_BT_BREDR_HFP_RESULT_CIEV_T;

/* AT_RESULT_CIND_READ (+CIND: 1,1,2,0) Result Data -- AG Indicators Status */
typedef struct {
    INT8_T      data[TUYA_BT_BREDR_HFP_CIND_LAST];    /* Array of the current states of the AG
                                       indicators; -1 if N/A (no indicator)  */
} TUYA_BT_BREDR_HFP_RESULT_CIND_T;

typedef struct {
    UINT8_T     index;  /* Indicator index */
    INT8_T      min;    /* Minimum indicator status value; -1 if N/A */
    INT8_T      max;    /* Maximum indicator status value; -1 if N/A */
} RESULT_CIND_MAP_T;

/* AT_RESULT_CIND_TEST (+CIND Test) Result Data -- AG-Supported Indicators
 * Mapping */
typedef struct {
    RESULT_CIND_MAP_T  data[TUYA_BT_BREDR_HFP_CIND_LAST];
} TUYA_BT_BREDR_HFP_RESULT_CIND_MAP_T;

typedef struct {
    INT8_T      call_id;    /* Call identification number (one-based) */
    INT8_T      dir;        /* Call direction:
                               0 - Outgoing call (mobile originated - MO)
                               1 - Incoming call (mobile terminated -MT)
                             */
    INT8_T      stat;       /* Call status:
                               0 - Active
                               1 - Held
                               2 - Dialing (MO call)
                               3 - Alerting (MO call)
                               4 - Incoming (MT call)
                               5 - Waiting (MT call)
                               6 - Call held by Response and Hold
                             */
    INT8_T      mode;       /* Call mode:
                               0 - Voice
                               1 - Data
                               2 - Fax
                               The following modes are defined in the 3GPP TS
                               spec, and are not part of the HFP Spec:
                               3 - Voice followed by data, voice mode
                               4 - Alternating voice/data, voice mode
                               5 - Alternating voice/fax, voice mode
                               6 - Voice followed by data, data mode
                               7 - Alternating voice/data, data mode
                               8 - Alternating voice/fax, fax mode
                               9 - Unknown
                             */
    INT8_T      mpty;       /* 0 - Call is not a member of a multi-party
                                   (conference) call
                               1 - Call is a member of multi-party (conference)
                                   call
                             */
    UINT8_T        number[32]; /* Optional phone number, as a
                                         * double-quoted string */
    INT16_T     type;       /* Optional address integer format; -1 if N/A */
} TUYA_BT_BREDR_HFP_RESULT_CLCC_T;

/* AT_RESULT_CLIP (+CLIP) Result Data --
 * Calling Line Identification (CLI) Notification Data */
typedef struct {
    UINT8_T    number[32]; /* Phone number, as a double-quoted string
                                     */
    INT16_T type;                   /* Phone number format integer; -1 if N/A */
    UINT8_T    subaddr[32];/* Subaddress string */
    INT8_T  satype;                 /* Subaddress integer format; -1 if N/A */
    UINT8_T    alpha[32];   /* Alphanumeric representation of the
                                       phone number ("number"), corresponding
                                       to the phone-book entry */
    INT8_T  cli_validity;           /* CLI validity (-1 if N/A):
                                       0 - CLI is valid.
                                       1 - CLI has been withheld by the
                                           originator.
                                       2 - CLI is not available due to
                                           interworking problems or limitations
                                           of the originating network.
                                     */
} TUYA_BT_BREDR_HFP_RESULT_CLIP_T;


typedef enum {
    HFP_CME_ERROR_NO_CONNECTION_TO_PHONE             = 1,
    HFP_CME_ERROR_OPERATION_NOT_ALLOWED              = 3,
    HFP_CME_ERROR_AG_FAILURE                         = 0,
    HFP_CME_ERROR_OPERATION_NOT_SUPPORTED            = 4,
    HFP_CME_ERROR_PH_SIM_PIN_REQUIRED                = 5,
    HFP_CME_ERROR_SIM_NOT_INSERTED                   = 10,
    HFP_CME_ERROR_SIM_PIN_REQUIRED                   = 11,
    HFP_CME_ERROR_SIM_PUK_REQUIRED                   = 12,
    HFP_CME_ERROR_SIM_FAILURE                        = 13,
    HFP_CME_ERROR_SIM_BUSY                           = 14,
    HFP_CME_ERROR_INCORRECT_PASSWORD                 = 16,
    HFP_CME_ERROR_SIM_PIN2_REQUIRED                  = 17,
    HFP_CME_ERROR_SIM_PUK2_REQUIRED                  = 18,
    HFP_CME_ERROR_MEMORY_FULL                        = 20,
    HFP_CME_ERROR_INVALID_INDEX                      = 21,
    HFP_CME_ERROR_MEMORY_FAILURE                     = 23,
    HFP_CME_ERROR_TEXT_STRING_TOO_LONG               = 24,
    HFP_CME_ERROR_INVALID_CHARS_IN_TEXT_STR          = 25,
    HFP_CME_ERROR_DIAL_STRING_TOO_LONG               = 26,
    HFP_CME_ERROR_INVALID_CHARS_IN_DIAL_STR          = 27,
    HFP_CME_ERROR_NO_NETWORK_SERVICE                 = 30,
    HFP_CME_ERROR_EMERGENCY_CALLS_ONLY               = 32,
} TUYA_BT_BREDR_HFP_CME_ERROR_CODE_E;

typedef struct {
    UINT8_T    number[32]; /* Phone number, as a double-quoted string
                                    */
    INT16_T type;                   /* Phone number format integer; -1 if N/A */
} TUYA_BT_BREDR_HFP_CNUM_T;

typedef struct {
    UINT8_T name[16];   /* Network operator name, as a quoted
                                       alphanumeric string */
} TUYA_BT_BREDR_HFP_COPS_T;

typedef struct {
    UINT8_T    name[32]; /* Phone name, as a string                                   */
    INT16_T para;                   /* Phone parameter format integer; -1 if N/A */

} TUYA_BT_BREDR_HFP_XAPL_T;

// typedef enum {
//     TUYA_BT_BREDR_HFP_VOICE_RECOGNITION_DISABLE_IN_AG,
//     TUYA_BT_BREDR_HFP_VOICE_RECOGNITION_ENABLE_IN_AG,
// } TUYA_BT_BREDR_HFP_RESULT_BRVA_E;

/* AT_RESULT_CNUM (+CNUM) Result Data -- Subscriber Number Information */
// typedef struct {
//     UINT8_T    number[32]; /* Phone number, as a double-quoted string
//                                     */
//     INT16_T type;                   /* Phone number format integer; -1 if N/A */
// } TUYA_BT_BREDR_HFP_CNUM_T;

typedef union {

        UINT8_T         vgm;            /* AT_RESULT_VGM   Valid range: [0,15] */
        UINT8_T         vgs;            /* AT_RESULT_VGS   Valid range: [0,15]*/
        TUYA_BT_BREDR_HFP_RESULT_CODEC_E         bcs;            /* codec */

        /* HFP-Specific AT Result Data */
        UINT16_T        brsf;           /* AT_RESULT_BRSF */
        TUYA_BT_BREDR_HFP_RESULT_BAND_RING_TONE_E  bsir; 
        TUYA_BT_BREDR_HFP_RESULT_BRTH_E        btrh;           /* AT_RESULT_BTRH */
        TUYA_BT_BREDR_HFP_RESULT_BVRA_E        bvra;           /* AT_RESULT_BVRA */
        TUYA_BT_BREDR_HFP_RESULT_CCWA_E        ccwa;           /* AT_RESULT_CCWA */
        TUYA_BT_BREDR_HFP_RESULT_CHLD_E        chld;           /* AT_RESULT_CHLD_TEST */
        TUYA_BT_BREDR_HFP_RESULT_CIEV_T        ciev;           /* AT_RESULT_CIEV */
        TUYA_BT_BREDR_HFP_RESULT_CIND_T        cind;           /* AT_RESULT_CIND_READ */
        TUYA_BT_BREDR_HFP_RESULT_CIND_MAP_T    cind_map;       /* AT_RESULT_CIND_TEST */
        TUYA_BT_BREDR_HFP_RESULT_CLCC_T        clcc;           /* AT_RESULT_CLCC */
        TUYA_BT_BREDR_HFP_RESULT_CLIP_T        clip;           /* AT_RESULT_CLIP */
        TUYA_BT_BREDR_HFP_CME_ERROR_CODE_E   cme_error;      /* AT_RESULT_CME_ERROR */
        TUYA_BT_BREDR_HFP_CNUM_T        cnum;           /* AT_RESULT_CNUM */
        TUYA_BT_BREDR_HFP_COPS_T        cops;           /* AT_RESULT_COPS */
        TUYA_BT_BREDR_HFP_XAPL_T        xapl;           /* AT_RESULT_XAPL */

} tuya_bt_bredr_hfp_result_u;



/* AT Result Codes */
typedef enum {
    TUYA_BT_BREDR_HFP_EVENT_HFP_CONNECTED,
    TUYA_BT_BREDR_HFP_EVENT_HFP_DISCONNECTED,
    TUYA_BT_BREDR_HFP_EVENT_HFP_SCO_CONNECTED,
    TUYA_BT_BREDR_HFP_EVENT_HFP_SCO_DISCONNECTED,

    TUYA_BT_BREDR_HFP_EVENT_RING,             /* Incoming call indication */
    TUYA_BT_BREDR_HFP_EVENT_VGM,              /* Gain of microphone */
    TUYA_BT_BREDR_HFP_EVENT_VGS,              /* Gain of speaker */

    /* HFP-Specific AT Result Codes */
    TUYA_BT_BREDR_HFP_EVENT_BRSF,             /* AG supported features */
    TUYA_BT_BREDR_HFP_EVENT_BSIR,             /* In-band ring tone setting */
    TUYA_BT_BREDR_HFP_EVENT_BTRH,             /* Bluetooth response and hold state */
    TUYA_BT_BREDR_HFP_EVENT_BVRA,             /* Voice recognition activation */
    TUYA_BT_BREDR_HFP_EVENT_CCWA,             /* Call Waiting notification */ // 11
    TUYA_BT_BREDR_HFP_EVENT_CHLD_TEST,        /* Call-hold & multi-party handling services */
    TUYA_BT_BREDR_HFP_EVENT_CIEV,             /* AG indicator events reporting (status) */
    TUYA_BT_BREDR_HFP_EVENT_CIND_READ,        /* AG indicators status */
    TUYA_BT_BREDR_HFP_EVENT_CIND_TEST,        /* Supported AG indicators mapping */
    TUYA_BT_BREDR_HFP_EVENT_CLCC,             /* List of current calls */  // 16
    TUYA_BT_BREDR_HFP_EVENT_CLIP,             /* Calling Line ID (CLI) notification */
    TUYA_BT_BREDR_HFP_EVENT_CME_ERROR,        /* Extended AG audio error */
    TUYA_BT_BREDR_HFP_EVENT_CNUM,             /* Subscriber number information */
    TUYA_BT_BREDR_HFP_EVENT_COPS,              /* Network operator string */
    TUYA_BT_BREDR_HFP_EVENT_XAPL,
    TUYA_BT_BREDR_HFP_EVENT_BCS,

    TUYA_BT_BREDR_HFP_EVENT_END

}TUYA_BT_BREDR_HFP_EVENT_E;

#define TUYA_BT_BREDR_HFP_HFSF_EC_NR_FUNCTION                       (1<<0)
#define TUYA_BT_BREDR_HFP_HFSF_THREE_WAY_CALLING                    (1<<1)
#define TUYA_BT_BREDR_HFP_HFSF_CLI_PRESENTATION_CAPABILITY          (1<<2)
#define TUYA_BT_BREDR_HFP_HFSF_VOICE_RECOGNITION_FUNCTION           (1<<3)
#define TUYA_BT_BREDR_HFP_HFSF_REMOTE_VOLUME_CONTROL                (1<<4)
#define TUYA_BT_BREDR_HFP_HFSF_ENHANCED_CALL_STATUS                 (1<<5)
#define TUYA_BT_BREDR_HFP_HFSF_ENHANCED_CALL_CONTROL                (1<<6)
#define TUYA_BT_BREDR_HFP_HFSF_CODEC_NEGOTIATION                    (1<<7)
#define TUYA_BT_BREDR_HFP_HFSF_HF_INDICATORS                        (1<<8)
#define TUYA_BT_BREDR_HFP_HFSF_ESCO_S4                              (1<<9)
#define TUYA_BT_BREDR_HFP_HFSF_ENHANCED_VOICE_RECOGNITION_STATUS    (1<<10)
#define TUYA_BT_BREDR_HFP_HFSF_VOICE_RECOGNITION_TEXT               (1<<11)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/

// typedef OPERATE_RET (*tkl_bt_bredr_hfp_receive_cb)(TUYA_BT_BREDR_HFP_COMMAND_E pdu_id, UINT8_T* payload,  UINT16_T len);
typedef OPERATE_RET (*tkl_bt_bredr_hfp_event_cb)(TUYA_BT_BREDR_HFP_EVENT_E event_id, tuya_bt_bredr_hfp_result_u*  result );



typedef struct {
    TUYA_BT_BREDR_HFP_ROLE_E role;
    UINT32_T hf_support_feature;
    TUYA_BT_BREDR_HFP_RESULT_CODEC_E hf_codec;
    // tkl_bt_bredr_hfp_receive_cb     reveive_response_handler;
    tkl_bt_bredr_hfp_event_cb       event_handler;
} TUYA_BT_BREDR_HFP_CONTEXT_T;




/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/
extern tkl_bt_bredr_hfp_event_cb       tkl_bt_bredr_hfp_event_func;


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/


OPERATE_RET tkl_bt_bredr_hfp_init(TUYA_BT_BREDR_HFP_CONTEXT_T*  context);
OPERATE_RET tkl_bluetooth_bredr_hfp_init(TUYA_BT_BREDR_HFP_CONTEXT_T*  context);


OPERATE_RET tkl_bt_bredr_hfp_uninit();

OPERATE_RET tkl_bt_bredr_hfp_connect(TUYA_BT_GAP_ADDR_T *addr);
OPERATE_RET tkl_bt_bredr_hfp_disconnect();

BOOL_T tkl_bt_bredr_hfp_has_connection();

OPERATE_RET tkl_bt_bredr_hfp_sco_connect();
OPERATE_RET tkl_bt_bredr_hfp_sco_disconnect();

BOOL_T tkl_bt_bredr_has_hfp_sco_connection();

OPERATE_RET tkl_bluetooth_bredr_hfp_send_at_command(TUYA_BT_BREDR_HFP_AT_CMD_E cmd, tuya_bt_avrcp_hfp_cmd_u* at_cmd);


/********************************************* HID ********************************************/


typedef enum
{
    TKL_BT_BREDR_HID_INT_CHANNEL = 0,
    TKL_BT_BREDR_HID_CTRL_CHANNEL = 1
} TKL_BT_BREDR_HID_CHANNEL_E;


/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

typedef struct {
    void (*connected_cb)(TUYA_BT_GAP_ADDR_T* addr);
    void (*disconnected_cb)(UINT8_T reason);
} tuya_bt_bredr_hid_conn_cbs_t;


typedef struct {

    tuya_bt_bredr_hid_conn_cbs_t  conn_cbs;
} TUYA_BT_BREDR_HID_CONTEXT_T;

extern tuya_bt_bredr_hid_conn_cbs_t  tuya_bt_bredr_hid_conn_func;

OPERATE_RET tkl_bt_bredr_hid_init(TUYA_BT_BREDR_HID_CONTEXT_T* context);

OPERATE_RET tkl_bt_bredr_hid_uninit();

OPERATE_RET tkl_bt_bredr_hid_connect(TUYA_BT_GAP_ADDR_T *addr);

OPERATE_RET tkl_bt_bredr_hid_disconnect();

OPERATE_RET tkl_bt_bredr_has_hid_connect(TUYA_BT_GAP_ADDR_T *addr);

OPERATE_RET tkl_bt_bredr_hid_send_data(UINT8_T* data, UINT32_T length, TKL_BT_BREDR_HID_CHANNEL_E channel);


/*********************************************** CHARGE  **************************************************/



typedef enum {
    TKL_BLUETOOTH_BATTERY_CHARGE_PREPARE,
    TKL_BLUETOOTH_BATTERY_CHARGE_ACTIVE,
    TKL_BLUETOOTH_BATTERY_CHARGING,

    TKL_BLUETOOTH_BATTERY_CHARGE_FULL,
    TKL_BLUETOOTH_BATTERY_CHARGE_FINISHED,
    TKL_BLUETOOTH_BATTERY_CHARGE_ABORTED,
    TKL_BLUETOOTH_BATTERY_CHARGE_FULL_POWER
} TKL_BT_CHARGE_E;


typedef OPERATE_RET (*tkl_bt_charge_state_cb)(TKL_BT_CHARGE_E charge_state);


typedef struct {
    UINT8_T charger_current;
    UINT16_T percent_map[10];

    tkl_bt_charge_state_cb  charge_state_cb;

} TUYA_BT_CHARGE_T;

extern tkl_bt_charge_state_cb tkl_bt_charge_state_callback;

OPERATE_RET tkl_bluetooth_charge_init(TUYA_BT_CHARGE_T* context);

OPERATE_RET tkl_bluetooth_charge_uninit();


UINT8_T tkl_bluetooth_charge_vusb_is_charging();

UINT8_T tkl_bluetooth_charge_vusb_is_plug_in();

UINT16_T tkl_bluetooth_charge_vbat_get();


/************************************************************ AUDIO ***************************************/
typedef enum
{
    TKL_BT_AUDIO_AUDIO_DAC_MODE_DIFFERENCE,
    TKL_BT_AUDIO_AUDIO_DAC_MODE_SINGLE_END,
    TKL_BT_AUDIO_AUDIO_DAC_MODE_CLASSD,
    TKL_BT_AUDIO_AUDIO_DAC_MODE_VCOM,
}TKL_BT_AUDIO_DAC_MODE_E;

typedef enum
{
    TKL_BT_AUDIO_ADC_CHANNEL_0 = (1 << 0),
    TKL_BT_AUDIO_ADC_CHANNEL_1 = (1 << 1),
    TKL_BT_AUDIO_ADC_CHANNEL_2 = (1 << 2),
    TKL_BT_AUDIO_ADC_CHANNEL_3 = (1 << 3),
    TKL_BT_AUDIO_ADC_CHANNEL_4 = (1 << 4),
    TKL_BT_AUDIO_ADC_CHANNEL_5 = (1 << 5),
    TKL_BT_AUDIO_ADC_CHANNEL_6 = (1 << 6),
}TKL_BT_AUDIO_ADC_ANC_CHANNEL;


typedef struct {
    TKL_BT_AUDIO_ADC_ANC_CHANNEL channels;
    TKL_BT_AUDIO_DAC_MODE_E      mode;
    UINT32_T                     samplerate;
    UINT32_T                      dig_gain;
    UINT32_T                      ana_gain;
} tkl_bt_audio_dac_cfg_t;


/**
 * @brief audio ADC mode definition
 */
typedef enum
{
    TKL_BT_AUDIO_ADC_MODE_DIFFERENCE,
    TKL_BT_AUDIO_ADC_MODE_SINGLE_END,
    TKL_BT_AUDIO_ADC_MODE_AUTO_DC_CALIB,
}TKL_BT_AUDIO_ADC_MODE_E;


typedef enum
{
    TKL_BT_AUDIO_ADC_WIDTH_20,
    TKL_BT_AUDIO_ADC_WIDTH_24,
    TKL_BT_AUDIO_ADC_WIDTH_16,
}TKL_BT_AUDIO_ADC_WIDTH_E;


typedef struct {
    TKL_BT_AUDIO_ADC_ANC_CHANNEL channels;
    TKL_BT_AUDIO_ADC_WIDTH_E     wide;
    TKL_BT_AUDIO_ADC_MODE_E      mode;
    UINT32_T                     samplerate;
    UINT8_T                      dig_gain;
    UINT8_T                      ana_gain;
} tkl_bt_audio_adc_cfg_t;

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

/**
 * @brief Init the Bluetooth Audio Interface.
 *
 * @param none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_init(tkl_bt_audio_dac_cfg_t* tkl_bt_audio_dac_cfg, tkl_bt_audio_adc_cfg_t* tkl_bt_audio_adc_cfg);


/**
 * @brief Uninit the Bluetooth Audio Interface.
 *
 * @param none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_uninit();


/**
 * @brief   Init the Bluetooth Audio dac Interface, Generally dac is speaker.
 *
 * @param [in] channels: DAC TX channels
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 * */
OPERATE_RET tkl_bt_audio_dac_init(tkl_bt_audio_dac_cfg_t* tkl_bt_audio_dac_cfg);


/**
 * @brief Uninit the Bluetooth Audio dac Interface
 *
 * @param [in] channels: DAC TX channels
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_uninit(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels);

/**
 * @brief Config audio dac
 *
 * @param [in] channels: DAC TX channels
 * @param [in] samp_freq: DAC TX samplerate
 * @param [in] bits_per_sample: DAC bit width
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_config(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, uint32_t samp_freq, UINT8_T bits_per_sample);


/**
 * @brief enable or disable audio dac
 *
 * @param [in] channels: DAC TX channels
 * @param [in] enable:   1: open dac, 0: close dac
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_enable(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels,  BOOL_T enable);

/**
 * @brief Set audio dac volume.
 *
 * @param [in] channels: DAC TX channels
 * @param [in] volume:   set speaker volume. range[0 - 16]
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_set_volume(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, UINT8_T volume);

/**
 * @brief Get audio dac volume.
 *
 * @param [in] channels: DAC TX channels
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tkl_bt_audio_dac_get_volume(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels);

/**
 * @brief Set audio dac volume mute.
 *
 * @param [in] channels: DAC TX channels
 * @param [in] enable: enable or disable the mute mode
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_set_volume_mute(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, BOOL_T enable);

/**
 * @brief Set audio adc ana gain.
 *
 * @param [in] channels: DAC TX channels
 * @param [in] gain: analog gain to be set, value range from 0 ~ 7 which means 0dB ~ 7dB
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_set_ana_gain(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, UINT8_T gain);

/**
 * @brief Get audio adc ana gain.
 *
 * @param [in] channels: DAC TX channels
 *
 * @return return the ana gain
 */
UINT8_T tkl_bt_audio_dac_get_ana_gain(TKL_BT_AUDIO_ADC_ANC_CHANNEL channel);

/**
 * @brief Set audio adc dig gain.
 *
 * @param [in] channels: DAC TX channels
 * @param [in] gain: digital gain to be set
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_dac_set_dig_gain(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, UINT8_T gain);

/**
 * @brief Get audio adc dig gain.
 *
 * @param [in] channels: DAC TX channels
 *
 * @return return the dig gain
 */
UINT8_T tkl_bt_audio_dac_get_dig_gain(TKL_BT_AUDIO_ADC_ANC_CHANNEL channel);



// /**
//  * @brief aud_dac_config
//  *
//  * @param channels:  通道
//  * @param freq:  采样频率
//  * @param freq:  位宽
//  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
//  */
// OPERATE_RET tkl_bt_audio_adc_config(uint32_t channels, uint32_t samp_freq, uint32_t bits_per_sample);


/**
 * @brief   Init the Bluetooth Audio adc Interface, Generally adc is mic.
 *
 * @param [in] channels: ADC  channels
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 * */
OPERATE_RET tkl_bt_audio_adc_init(tkl_bt_audio_adc_cfg_t* tkl_bt_audio_adc_cfg);

/**
 * @brief   Uninit the Bluetooth Audio dac Interface.
 *
 * @param [in] channels: ADC  channels
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 * */
OPERATE_RET tkl_bt_audio_adc_uninit(tkl_bt_audio_adc_cfg_t* tkl_bt_audio_adc_cfg);

/**
 * @brief Config audio ADC
 *
 * @param [in] channels: ADC  channels
 * @param [in] samp_freq: ADC  samplerate
 * @param [in] bits_per_sample: ADC bit width
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_adc_config(uint32_t channels, uint32_t samp_freq, UINT8_T bits_per_sample);   // 配置mic

/**
 * @brief enable or disable audio ADC
 *
 * @param [in] channels: ADC channels
 * @param [in] enable:   1: open ADC, 0: close ADC
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_adc_enable(uint32_t channels, uint32_t enable);    // 使能mic


/**
 * @brief Set audio adc ana gain.
 *
 * @param [in] channels: ADC channels
 * @param [in] gain： anglog gain to be set, value range from 0 ~ 12 which means 0dB ~ 24dB, step is 2dB
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_adc_set_ana_gain(uint32_t channels, uint32_t gain);        // 设置mic模拟增益

/**
 * @brief Get audio adc ana gain.
 *
 * @param [in] channel: ADC channels
 *
 * @return return ana gain
 */
UINT8_T tkl_bt_audio_adc_get_ana_gain(uint32_t channel);    // 获取mic模拟增益


/**
 * @brief Get audio adc ana gain.
 *
 * @param [in] channels: ADC channels
 *
 * @return return ana gain
 */
OPERATE_RET tkl_bt_audio_adc_set_dig_gain(uint32_t gain);    // 设置mic数字增益

/**
 * @brief Get audio adc ana gain.
 *
 * @param [in] channel: ADC channels
 *
 * @return return ana gain
 */
UINT8_T tkl_bt_audio_adc_get_dig_gain(uint32_t channel);    // 获取mic数字增益


/**
 * @brief Set audio adc volume.
 *
 * @param [in] channels: adc TX channels
 * @param [in] volume:   set speaker volume. range[0 - 16]
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_adc_set_volume(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, UINT8_T volume);  // 设置mic音量

/**
 * @brief Get audio adc volume.
 *
 * @param [in] channels: adc TX channels
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT8_T tkl_bt_audio_adc_get_volume(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels);  // 获取mic音量

/**
 * @brief Set audio adc volume mute.
 *
 * @param [in] channels: adc TX channels
 * @param [in] enable: enable or disable the mute mode
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_bt_audio_adc_set_volume_mute(TKL_BT_AUDIO_ADC_ANC_CHANNEL channels, BOOL_T enable);  // MIC静音


OPERATE_RET tkl_bt_bredr_wave_file_play_start( UINT32_T file_id );           // 播放音频文件

OPERATE_RET tkl_bt_bredr_wave_file_play_stop();  // 停止播放音频文件

BOOL_T tkl_bt_bredr_wave_file_is_playing(); //  是否正在播放


#ifdef __cplusplus
}
#endif

#endif /* __TKL_BLUETOOTH_BREDR_H__ */
