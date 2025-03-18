# TuyaOS Demo BLE LOCK Use Guide

更新时间：2023-12-15  作者：shelton

## 概述

在阅读本文档之前，建议您先阅读以下几篇文章，以便更好的熟悉理解开发框架、提供能力以及芯片平台资料，包括：

1. 阅读位于 **tuyaos_demo_ble_peripheral 文件夹下的 README.md 文档**，以了解 TuyaOS 蓝牙开发包的基本开发流程；
2. TuyaOS 蓝牙开发框架，框架以及提供的能力详见[能力地图](https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_software_map_bt?id=Kc6i34kejqmni) ；
3. 阅读对应[芯片平台开发资料](https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_platform?id=Kc6fd92l28kkm) ，熟悉该平台硬件环境的基础操作。

本文档将着重介绍蓝牙门锁品类相关的基本操作与概念，以帮助您理解并掌握如何使用 TuyaOS 蓝牙开发包开发一款带有基础功能的蓝牙门锁产品。



## 创建产品

### 创建PID

**Demo提供测试用pid**：3yglvz27，该pid专为蓝牙门锁Pro方案演示demo而设计。

开发者可以在 [涂鸦 IoT 平台](https://iot.tuya.com/) 按照[创建产品](https://developer.tuya.com/cn/docs/iot/create-product?id=K914jp1ijtsfe)的流程来创建新的PID，也可以直接 [复制该 PID](https://pbt.tuya.com/s?p=3125bbb8af3aff3ef4f5ce928e5a8425&u=60e9cf653473f084aa6b01a8820f5ddd&t=1) （在新的 IoT 平台账号上创建相同功能的 PID）。

您创建新的产品PID后，请将位于`tuya_ble_protocol_callback.h`下的TY_DEVICE_PID替换成您新建的pid。

**创建蓝牙门锁产品**：

![](https://images.tuyacn.com/fe-static/docs/img/1119128a-9cba-438e-8c6e-1e76f78408b2.png)

### 测试PID(3yglvz27) DP 功能点

| DP ID | 功能点名称           | 标识符 dp_code       | 数据传输类型       | 数据类型 |
| ----- | -------------------- | -------------------- | ------------------ | -------- |
| 8     | 剩余电量             | residual_electricity | 只上报（ro）       | value    |
| 19    | 蓝牙开锁             | unlock_ble           | 只上报（ro）       | value    |
| 20    | 关锁记录             | lock_record          | 可下发可上报（rw） | raw      |
| 33    | 自动关锁             | automatic_lock       | 可下发可上报（rw） | bool     |
| 36    | 自动关锁延迟         | auto_lock_time       | 可下发可上报（rw） | value    |
| 44    | 本地RTC时钟          | rtc_lock             | 可下发可上报（rw） | bool     |
| 46    | 手动关锁             | manual_lock          | 可下发可上报（rw） | bool     |
| 47    | 锁开合状态           | lock_motor_state     | 只上报（ro）       | bool     |
| 70    | 配置蓝牙开锁校验码   | check_code_set       | 可下发可上报（rw） | raw      |
| 71    | 蓝牙开锁（带校验码） | ble_unlock_check     | 可下发可上报（rw） | raw      |

Demo仅选取了部分DP点，实际可根据产品需求增减其他DP点，完整的DP协议详见[《蓝牙门锁DP参考》](https://developer.tuya.com/cn/docs/iot/ble?id=K9ow3vcpn71ua)。



## Demo说明

### 目录结构

此处展示的 Demo 是基于TuyaOS BLE 开发包开发的**蓝牙门锁最基础功能示例demo**，开发者可在体验理解后扩充功能点开发自有产品。

![](https://images.tuyacn.com/fe-static/docs/img/b6774955-73ae-452d-8c3f-9afaef2ed514.png)

| 文件                         | 说明                                                         |
| ---------------------------- | ------------------------------------------------------------ |
| app_dp_parser.c              | DP 点解析例程，包含 DP 点接收与发送处理                      |
| app_lock.c                   | 门锁逻辑，如自动落锁功能等                                   |
| tuya_ble_protocol_callback.c | 主要负责处理涂鸦蓝牙通信协议的回调事件处理以及相关的接口，是**SDK与应用开发的桥梁，重点理解** |
| tuya_sdk_callback.c          | 主要负责 TuyaOS SDK 的事件回调处理、各级初始化以及大循环处理 |
| app_config.h                 | 应用配置文件，包含固件标识名、固件版本、硬件版本、SDK版本等信息 |

### 常用 API

```c
// 解析APP/网关下发的DP指令
OPERATE_RET app_dp_parser(UINT8_T* buf, UINT32_T size);

// 设备上报DP状态
OPERATE_RET app_dp_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size);

// 设备上报记录，如开/关锁操作记录等
OPERATE_RET app_dp_record_report(UINT8_T dp_id, UINT8_T* buf, UINT32_T size);
```

### 常用 回调事件

```c
case TUYA_BLE_CB_EVT_CONNECT_STATUS: {	     
    // 收到蓝牙连接状态
    if (event->connect_status == BONDING_CONN) {
        TAL_PR_INFO("bonding and connecting");
        tuya_ble_update_conn_param_timer_start();
        
        // YOUR JOBS
        //1. 判断如果门锁设备处于未绑定状态，更新设备为已绑定状态
        //2. 创建并启动定时器，用于设备超时主动断开蓝牙连接
    }
} break;

case TUYA_BLE_CB_EVT_DP_DATA_RECEIVED: {
    // 接收到DP数据, 解析APP下发的dp指令
    app_dp_parser(event->dp_received_data.p_data, event->dp_received_data.data_len);
} break;

case TUYA_BLE_CB_EVT_TIME_STAMP: {
    // 时间同步, 设置本地时间戳和时区
    tal_rtc_time_set(timestamp_s);
    tal_utc_set_time_zone(event->timestamp_data.time_zone);

} break;

case TUYA_BLE_CB_EVT_DP_QUERY: {
    // 收到APP请求获取设备DP状态
    TAL_PR_HEXDUMP_INFO("TUYA_BLE_CB_EVT_DP_QUERY", event->dp_query_data.p_data, event->dp_query_data.data_len);
    
    // YOUR JOBS：您可在此处上报所有门锁状态和设置信息，如：电量、门锁状态、音量等等
    UINT8_T vbat_value[4] = {0};
    vbat_value[3] = 100;
    app_dp_report(OR_DP_STATUS_BATTERY_PERCENT, (UINT8_T*)&vbat_value, SIZEOF(UINT32_T));
} break;

case TUYA_BLE_CB_EVT_UNBOUND: {				
    // 收到解绑消息
    TAL_PR_INFO("TUYA_BLE_CB_EVT_UNBOUND");
    
    // YOUR JOBS：对产品做恢复出厂设置
} break;

case TUYA_BLE_CB_EVT_ANOMALY_UNBOUND: {		
    // 收到异常解绑消息(如离线时从app移除设备，再与设备连接时）
    TAL_PR_INFO("TUYA_BLE_CB_EVT_ANOMALY_UNBOUND");
    
    // YOUR JOBS：对产品做恢复出厂设置
} break;

case TUYA_BLE_CB_EVT_DEVICE_RESET: {		
    // 收到重置消息
    TAL_PR_INFO("TUYA_BLE_CB_EVT_DEVICE_RESET");
    
    // YOUR JOBS：对产品做恢复出厂设置
} break;
```

### 开/关锁操作

```c
case WR_SBP_DP_MASTER_OPEN_LOCK: {      //!< new ble open lock [dev with ble parts function]
    TAL_PR_INFO("dp ble open lock");
    doorlock_opened_status = TRUE;
	
    // 1. 上报门锁状态-锁已开，APP上将会实时显示
    UINT8_T lock_status = 0x01;         //!< lock status opened
    app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1); 

    // 2. 上报蓝牙开锁记录
    UINT8_T data[4] = {0};              //!< report lock opened record
    data[3] = g_cmd.dp_data[18];        //!< operate user
    app_dp_record_report(OR_DP_RECORD_BLE_OPEN_LOCK, data, 4);
	
    // 3. 如果打开了自动落锁功能，启动自动落锁定时器
    if (autolock_stu) {
        app_lock_autolock_timer_start(autolock_period);
    }
} break;

case WR_DP_MANUAL_CLOSE_DOORLOCK: {     //!< manual close doorlock
    TAL_PR_INFO("dp manual close lock");
    doorlock_opened_status = FALSE;

    app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);

    // 1. 上报门锁状态-锁已关，APP上将会实时显示
    UINT8_T lock_status = 0x00;         //!< lock status closed
    app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);

    // 2. 上报关锁记录
    UINT8_T data[5] = {0};              //!< report lock closed record
    data[0] = APP_CLOSE_LOCK;
    data[4] = 0xFF;
    app_dp_record_report(OR_DP_RECORD_CLOSE_LOCK_COMMON, data, 5);
	
    // 3. 如果打开了自动落锁功能，关闭自动落锁定时器
    if (autolock_stu) {
        app_lock_autolock_timer_stop();
    }
} break;
```

### 自动落锁设置

```c
case WR_DP_SETTING_AUTOLOCK_STU: {      //!< setting autolock state
    autolock_stu = g_cmd.dp_data[0];
    
    // 1. DP回复
    TAL_PR_INFO("dp set autolock stu: %d", autolock_stu);
    app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);
	
    // 2. YOUR JOBS：存储设置参数 
    
    // 3. 如果自动落锁打开并且当前处于锁开状态，则执行上锁操作并上报锁关状态
    if (autolock_stu && doorlock_opened_status) {
        app_lock_autolock_timer_stop();
		
        // TODO .. YOUR JOBS: operate close doorlock
        
        UINT8_T lock_status = 0x00;     //!< close lock immediately, sync lock status closed
        app_dp_report(OR_DP_STATUS_LOCK_STU, &lock_status, 1);
    }
} break;

case WR_DP_SETTING_AUTOLOCK_PERIOD: { 		//!< setting autolock period
    // 1. 解析自动落锁时长，并做DP回复
    autolock_period  = g_cmd.dp_data[0]<<24;
    autolock_period += g_cmd.dp_data[1]<<16;
    autolock_period += g_cmd.dp_data[2]<<8;
    autolock_period += g_cmd.dp_data[3];
    autolock_period *= 1000;
    TAL_PR_INFO("dp set autolock period: %d(ms)", autolock_period);
	
    app_dp_report(g_cmd.dp_id, g_cmd.dp_data, g_cmd.dp_data_len);
    
    // 2. YOUR JOBS：存储设置参数
} break;
```

## 授权

**授权调试模式**

前期暂无生产需要，为快速体验可通过**使能TUYA_SDK_DEBUG_MODE宏配置为1**进行临时授权（**仅用于调试，生产时请改回原状**）。

**生产授权模式**

必须将**TUYA_SDK_DEBUG_MODE宏配置为0**，然后使用涂鸦授权工具操作写入授权码，详见《[TuyaOS BLE SDK Product Test](https://registry.code.tuya-inc.top/document/platform/-/blob/main/_%E6%B1%87%E6%80%BB/04_%E4%BA%A7%E6%B5%8B/TuyaOS_BLE_SDK_Product_Test.md)》。

**免费领取调试授权码**

如下图所示，每个pid可免费领取2个调试授权码。领取成功后可替换`tuya_ble_protocol_callback.h`下的**三元组TY_DEVICE_MAC、TY_DEVICE_DID、TY_DEVICE_AUTH_KEY**。

![](https://images.tuyacn.com/fe-static/docs/img/3f8a02e8-6a6e-4e9c-b57f-f7231468f699.png)



## 烧录固件

烧录固件跟芯片平台完全相关，请参阅对应[芯片平台开发资料](https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_platform?id=Kc6fd92l28kkm)文档内的**烧录固件**章节；设备log请参阅**硬件外设UART**章节，以了解日志打印对应引脚。

## 测试

替换完PID以及授权三元组后，编译烧录固件至目标板，即成功激活涂鸦BLE设备。

此时可在App Store下载 `涂鸦智能` App，登录后 `添加设备` / 点击 `右上角` → `添加设备` 

<img src="https://images.tuyacn.com/fe-static/docs/img/38402ccd-c0a4-4e44-8623-cc7728300da7.jpg" style="zoom:20%;" />

设备添加完成后如下图所示：

<img src="https://images.tuyacn.com/fe-static/docs/img/837b312a-c5e5-4f91-b8a3-5241a5eeef79.jpg" style="zoom:20%;" />

此时可通过 `涂鸦智能` App 控制 BLE 设备。如开/关锁，设置等

<img src="https://images.tuyacn.com/fe-static/docs/img/98ecf77c-4da8-4a57-a8e1-c8f12630fa24.jpg" style="zoom:20%;" />



## 支持与帮助

1. 我们提供**蓝牙门锁产品开发包**，支持丰富的门锁组件与应用。开发者可按需申请，详见[产品开发包下载](https://developer.tuya.com/cn/docs/iot-device-dev/tuyaos-wind-ide?id=Kbfy6kfuuqqu3#title-12-%E4%BA%A7%E5%93%81%E5%BC%80%E5%8F%91%E5%8C%85%E4%B8%8B%E8%BD%BD)。
2. 在开发过程遇到问题，您可以登录 TuyaOS 开发者论坛 [子设备开发版块](https://www.tuyaos.com/viewtopic.php?t=12&sid=9c9cadc78b70eddb4707bb77f7339971) 进行沟通咨询。