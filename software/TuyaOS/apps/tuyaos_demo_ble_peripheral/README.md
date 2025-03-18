# TuyaOS BLE Developer Guide

更新时间：2022-06-20  作者：逻辑

## 概述

完成一件事情需要有限个步骤，嵌入式开发也不例外。

我们需要完成的事情是：基于 BLE 技术，将设备接入涂鸦 IoT 平台。

![image-20220406171632450](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220406171632450.png)

**设备**：由开发者提供，涂鸦提供教程（本文档及文中提及的其他文档）

**BLE**：Bluetooth Low Energy，低功耗蓝牙

**涂鸦智能 App**：由涂鸦提供，开发者可在 App Store 搜索安装

**网络**：公共网络

**涂鸦 IoT 平台**：由涂鸦提供，访问地址：https://iot.tuya.com/



根据上图可知：设备可以通过 BLE 接入涂鸦智能 App，涂鸦智能 App 通过网络连接涂鸦 IoT 平台，即实现了设备接入涂鸦 IoT 平台的目标。

因为涂鸦智能 App 和涂鸦 IoT 平台都是由涂鸦提供的，所以作为开发者，接下来的重点内容就是完成设备的嵌入式开发。

完成设备的嵌入式开发这件事情可以分解为有限个步骤：

（1）理解概念

（2）搭建环境

（3）开发软件

（4）烧录固件

（5）授权

（6）测试

（7）OTA

考虑到设备使用的 BLE 芯片各不相同，搭建环境和烧录固件等芯片相关的部分内容会在《TuyaOS_BLE_Platform_xxxx》文档中进行介绍。

## 理解概念

为了开发工作的顺利进行，对一些常见的概念进行说明，更多说明见 [名词解释](https://developer.tuya.com/cn/docs/iot/terms?id=K914joq6tegj4)。

### 蓝牙配网

考虑到兼容性和安全性，涂鸦设计并开发了一整套 涂鸦 BLE 配网协议，相关概念介绍如下。

#### 角色

主机：涂鸦智能APP（手机）、蓝牙网关

从机：涂鸦智能设备（蓝牙单点：门锁、灯、防丢器，体脂称、手环……）

主机通过 **蓝牙广播** 识别从机，并对已授权的从机发起配对请求，完成配对的主从机处于 **绑定状态 ** 。处于绑定状态的主从机之间存在一条符合蓝牙规范的安全通道，所有的业务类通信（包括DP数据）都在该安全通道中进行。日常交流中，主机多称为手机 / App，从机多称为设备。

#### 配网

安全通道的建立过程（配网）可以简述如下：**未绑定 > 连接 > 配对 > 绑定 > 安全通信** > 断开 > 重连 > 绑定 > 安全通信 > …… > **解配对 > 未绑定**

- 连接：表示蓝牙设备链路层的状态为连接状态。

- 配对：视为一种过程，一系列的密钥交换过程。

- 绑定：视为一种状态，配对完成的状态称之为“绑定状态”，处于绑定状态的两个设备可以进行安全通信。

- 重连：视为一种过程，依然是一系列的密钥交换过程，可以认为是简化的“配对” ，该过程的前提是此前已经进入过一次绑定状态。

- 解配对：视为一种过程，秘钥删除和绑定状态等信息的清除，也称作“移除”、“解绑”。

#### 解绑

解绑

- 触发条件：手机面板上点击“解除绑定”按钮

- 触发事件：TUYA_BLE_CB_EVT_UNBOUND

解绑并清除数据

- 触发条件：手机面板上点击“解绑并清除数据”按钮

- 触发事件：TUYA_BLE_CB_EVT_DEVICE_RESET

异常解绑

- [触发条件]()：手机面板上点击“解除绑定”按钮

- 触发事件：TUYA_BLE_CB_EVT_ANOMALY_UNBOUND

#### 异常解绑

异常解绑又称作**离线移除**。

指的是涂鸦智能 App 在未连接蓝牙设备的情况下，在面板上对该设备进行“解除绑定/解绑并清除数据”，操作成功并同步至云端，此时设备端还处于绑定状态。

此后，设备进行蓝牙广播（绑定状态），如果任一涂鸦智能 App 扫描到该设备，发现其广播是绑定状态但是该设备在云端的状态为未绑定状态，则会主动对其进行蓝牙连接，连接成功后发送异常解绑指令（该过程APP界面无任何变化），此时蓝牙设备就能收到异常解绑事件，并进行异常解绑操作。

#### 相关文档

《Tuya BLE Communication Protocol》

获取方式：搜索企业微信群“蓝牙通信协议同步”可获取最新版本，若不在群内请联系高永会获取。

#### 其他名词

| **名词**           | **说明**                                                     |
| :----------------- | :----------------------------------------------------------- |
| MAC                | MAC地址一般采用6字节（48比特），48比特都有其规定的意义，前24位是由生产网卡的厂商向IEEE申请的厂商地址，目前的价格是1000美元买一个地址块，后24位由厂商自行分配，这样的分配使得世界上任意一个拥有48位MAC 地址的网卡都有唯一的标识。所以要求客供MAC的客户要有自己购买的MAC地址块。 |
| UUID               | UUID是设备入涂鸦云的授权凭证。模组订单生成时，便在Tuya云根据模组订单信息生成授权码（包含需要烧录的固件信息、UUID等），在模组烧录授权过程，上位机工具录入授权码，在“授权”环节，将UUID与模组MAC地址进行绑定，保证一机一码。 |
| Device  ID         | Device ID也叫设备ID、虚拟ID等，是设备配网激活时，根据设备UUID在云端随机生成的设备凭证。 |
| 蓝牙绑定           | 蓝牙绑定是指蓝牙设备通过 Tuya BLE Communication Protocol 与 App 经过一系列秘钥交换、鉴权建立起的一种设备与APP账号的绑定关系，以及表示设备在云端的绑定状态。 |
| 蓝牙解绑           | 蓝牙解绑是指设备解除与 App  账号的绑定关系，进入未绑定未连接状态。设备存储的相关秘钥、绑定状态等信息也会被清除。 |
| 蓝牙重置           | 蓝牙重置是指在 App  上操作解绑并清除数据，与蓝牙解绑的区别在于用户数据是否清除。 |
| 蓝牙连接           | 蓝牙连接仅表示蓝牙设备链路层的状态为连接状态。               |
| 蓝牙广播           | 蓝牙广播仅表示蓝牙设备链路层的状态为广播状态。               |
| 蓝牙配网状态       | 该名词中的 **配网** 不同于 Wi-Fi 设备配路由器的过程，指的是未绑定未连接的蓝牙设备处于广播状态的一种状态，此时 App  可通过广播发现设备。 |
| 未绑定未连接       | 表示设备当前既未注册到涂鸦云，也没有处于蓝牙连接状态。若当前设备还处于蓝牙广播状态，设备处于可配网状态。 |
| 未绑定已连接       | 表示未绑定的设备处于蓝牙连接状态。                           |
| 绑定未连接         | 通常也叫设备 **离线**，表示设备与 App 账号建立了绑定关系，但链路层未连接，不处于安全通讯状态。 |
| 绑定已连接         | 通常也叫设备 **上线**，蓝牙绑定已连接是指蓝牙设备通过 **涂鸦蓝牙通讯协议** 与 App 建立的安全通讯状态。 |
| 绑定未鉴权已连接   | 这个状态是配对或重连中的一个中间状态，通常表示已绑定的设备刚刚建立蓝牙连接。 |
| 未绑定未鉴权已连接 | 与未绑定未连接的区别是该状态表示已处于蓝牙连接状态，暂时不可被发现。 |

### 产品

[产品的定义](https://developer.tuya.com/cn/docs/iot/development-overview?id=Ka3redtxl6g4k#title-0-%E4%BB%80%E4%B9%88%E6%98%AF%E4%BA%A7%E5%93%81%EF%BC%9F)

[创建产品的流程](https://developer.tuya.com/cn/docs/iot/create-product?id=K914jp1ijtsfe)

#### PID

PID，Product ID，产品 ID，描述一类产品功能（DP）的集合。

在 [涂鸦 IoT 平台](https://iot.tuya.com/) 创建的每一个产品都会产生一个唯一的产品编号，关联了产品具体的功能点、App 控制面板、出货信息等所有跟这个产品相关的信息。

##### 测试用 PID

3aubjk7p

该 PID 作为 nRF52832 芯片平台用于测试的 PID，开发者可以按照创建产品的流程创建新的 PID，也可以直接 [复制该 PID](https://pbt.tuya.com/s?p=3125bbb8af3aff3ef4f5ce928e5a8425&u=60e9cf653473f084aa6b01a8820f5ddd&t=1) （在新的 IoT 平台账号上创建相同功能的 PID）。

注意：复制的 PID 仅用于测试/调试，不可用作生产。

##### PID 3aubjk7p 绑定的固件Key

key7ccsa

固件最新版本：一般来说，版本说明为 **OTA测试专用** 的版本为最新版本的 OTA 测试固件，所以最新版本为其下面的第一个固件，例如，下图最新版本为V4.0。

![image-20211229165043767](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229165043767.png)

##### PID 3aubjk7p 的绑定模式

弱绑定

#### DP点

DP，Data Point，数据点，又称为 DP 点或功能点，表示设备所具备的功能集合。

**DP数据格式：**

| **字段**      | **长度（byte）** | **说明**       |
| ------------- | ---------------- | -------------- |
| dp_id         | 1                | DP点的指令码   |
| dp_type       | 1                | DP点的数据类型 |
| dp_data_len   | 2                | DP点的数据长度 |
| dp_data_value | dp_data_len      | DP点的数据     |

**dp_type的取值范围及含义（云端定义）如下：**

| **dp_type** | **取值** | **长度（byte）** | **说明** |
| ----------- | -------- | ---------------- | -------- |
| raw         | 0        | 1~N              | 透传型   |
| bool        | 1        | 1                | 布尔型   |
| value       | 2        | 4                | 数值型   |
| string      | 3        | 0~255            | 字符型   |
| enum        | 4        | 1                | 枚举型   |
| bitmap      | 5        | 4                | 故障型   |

##### PID 3aubjk7p 的 DP 点

![image-20211229162859864](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229162859864.png)

## 搭建环境

### 搭建硬件环境

硬件环境跟芯片平台完全相关，请参阅《TuyaOS_BLE_Platform_xxxx》文档

### 搭建软件环境

部分软件环境跟芯片平台无关，在该文档中统一介绍，其他软件环境跟芯片平台相关，请参阅《TuyaOS_BLE_Platform_xxxx》文档

#### [Tuya Wind IDE](https://developer.tuya.com/cn/docs/iot-device-dev/tuyaos-wind-ide?id=Kbfy6kfuuqqu3)

安装Python

前往 [Python 官网](https://www.python.org/downloads/) 下载 3.6~3.8 的版本进行默认安装。

注意：windows 下安装完 python 后在安装路径下默认是 python.exe，需要复制一份并重命名为 python3.exe（如果 python3.exe 已存在请忽略该步骤）。

![image-20211229171340283](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229171340283.png)

验证

![image-20211229171351181](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229171351181.png)

前往 [Vscode 官网](https://code.visualstudio.com/download) 下载最新的版本，并进行安装。

非常建议安装以下Vscode插件：

![image-20211231113746244](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231113746244.png)

![image-20211231113912246](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231113912246.png)

安装完成 Vscode 后在插件栏搜索 `Tuya Wind IDE` 并进行安装，安装完成后进行账号登录（同涂鸦 IoT 平台账号），如果没有账号，请前往 [涂鸦 IoT 平台](https://iot.tuya.com/) 进行账号注册。

![image-20220411150042599](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220411150042599.png)

登录完成后，请参考 `开发框架` 进行接下来的步骤。

#### 开发框架

开发框架即开发包

##### 获取

在 Vscode 下登录 `Tuya Wind IDE` 账号，然后选择 `创建新框架` 进行开发包选择

![image-20220411150243137](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220411150243137.png)

选择到合适的开发包后点击确认并开始拉取，拉取完成即可进行相关功能的开发。

![image-20220411150522556](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220411150522556.png)

等待开发框架拉取完成

![image-20220411150607013](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220411150607013.png)

##### 编译

找到 Demo 所在目录，在该目录上右键选择 `Build Project` 

注意：编译之前，务必按照本文档和《TuyaOS_BLE_Platform_xxxx》完成所有 “搭建软件环境” 的步骤

![image-20211229171508492](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229171508492.png)

手动输入版本号（该版本号即为编译生成的固件版本号，用于授权和OTA）

![image-20211229171519740](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229171519740.png)

等待编译成功

![image-20211231114535344](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231114535344.png)

#### 使用第三方开发工具

使用 Keil 等第三方开发工具编译前，必须要使用 Vscode 进行一次编译。

打开 Keil 等第三方开发工具前，首先关闭Vscode，防止两个软件发生冲突。

在以下目录找到 Keil  等第三方开发工具的工程文件，打开后可以进行正常的编译和调试。

![image-20211231144043789](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231144043789.png)

#### 添加文件

`切勿使用Keil进行源文件和头文件的添加操作`

在如下目录下添加源文件

![image-20211231152446129](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231152446129.png)

在如下目录下添加头文件

![image-20211231152805277](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231152805277.png)

源文件/头文件添加完成后，至少使用 Vscode 进行一次代码编译，之后可继续使用 Vscode 或者切换到 Keil 等第三方开发工具进行代码调试工作。

#### 注意

1. 第一次编译必须要使用 Vscode 进行编译，脚本会自动构建编译环境，之后你可以打开 IAR 或者 Keil 等第三方开发工具进行调试；

2. Vscode 下的脚本会递归遍历 tuyaos_demo_ble_peripheral 文件夹，自动添加该文件下所有的源文件和头文件到编译环境；

3. Vscode 在每次编译前都会重新构建编译环境，以下操作要特别注意：

- 在使用 IDE 编译的时请关闭 IAR 或者是 Keil 等第三方开发工具，否则可能会导致文件占用的问题；

- 在使用 IAR 或者 Keil 等第三方开发工具进行调试的时候，禁止手动添加头文件或者是源文件到工程项目中；

## 开发软件

### 软件架构

![image-20220418183453214](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418183453214.png)

从图中可知，TuyaOS BLE 开发包主要分为两层，第一层称作涂鸦抽象层（tuya abstract layer），该层由各种组件组成，是开发包的主体部分，主要负责涂鸦蓝牙配网、通信、授权、OTA 等功能，其中大部分组件提供源代码，但是安全相关的部分组件仅根据芯片平台提供相应的库文件；第二层称作开发平台，主体由涂鸦提供标准的涂鸦核心层（tuya kernel layer，主要是各种标准头文件），这些头文件对上提供统一的接口，对下适配不同的芯片平台，适配工作可能由涂鸦开发人员完成，也可能由芯片原厂的开发人员完成。

基于 TuyaOS 开发包，应用开发人员可以进行各种业务开发，例如照明产品、家电产品等。为了方便应用开发人员快速开展工作，涂鸦提供了标准 Demo 工程，标准 Demo 工程无需任何改动，即可直接编译烧录，为应用开发人员提供跨平台的应用开发体验，详见测试章节。同时，应用开发人员可以基于标准 Demo 工程开发自己的产品。

#### 软件运行流程

![image-20220418200829687](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418200829687.png)

### 开发包目录

![image-20220418193704561](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418193704561.png)



#### Demo 目录

![image-20220418200017207](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418200017207.png)

此处展示的 Demo 是 TuyaOS BLE 开发包最基本的 Demo，开发者可以通过该 Demo 体验开发包支持的几乎所有功能，当然也可以基于该 Demo 开发任何产品。

##### app_config

应用信息配置文件，主要包括固件标识名、固件版本、硬件版本、SDK版本等信息。

##### app_dp_parser

DP 点解析例程，包含 DP 点接收函数和发送函数，开发人员可以在接收和发送之间添加业务逻辑。

##### app_key

按键例程，短按进入低功耗，长按恢复出厂设置。

##### app_led

LED 例程。

##### tuya_ble_protocol_callback

主要负责处理涂鸦蓝牙通信协议的事件回调处理以及相关的接口。

##### tuya_sdk_callback

主要负责 TuyaOS SDK 的事件回调处理、各级初始化以及大循环处理。

#### 组件（含库）目录

![image-20220418200918641](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418200918641.png)

![image-20220418200945813](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220418200945813.png)

组件是 TuyaOS BLE 开发包的主体部分，TuyaOS的目标就是实现代码的组件化，熟练地掌握常用组件的基本原理和实现方法对于产品开发可以达到事半功倍的效果。

##### tal_ble_bulkdata

实现大数据传输功能。

##### tal_ble_ota

实现OTA功能。

##### tal_ble_product_test

实现授权（产测）功能。

##### tal_ble_protocol

实现涂鸦 BLE 配网协议，内部开发人员请参考《Tuya BLE Communication Protocol》，其他使用人员无需关心实现细节，直接调用相关接口实现业务功能即可。接口介绍详见《[涂鸦ble sdk说明](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/tuyaos/%E6%B6%82%E9%B8%A6_ble_sdk_sdk%E8%AF%B4%E6%98%8E_v2_beta1.pdf)》的 `API介绍` 和 `CALL BACK EVENT 介绍` 章节，其他内容请忽略 。

##### tal_ble_uart_common

实现串口通用对接功能。

##### tal_ble_weather

实现查询天气功能。

##### tal_bluetooth

实现蓝牙相关接口封装。

##### tal_driver

实现驱动相关接口封装。

##### tal_key

实现按键功能。

##### tal_oled

实现 OLED 屏幕驱动功能。

##### tal_sdk_test

实现测试功能。

##### tal_system

实现系统相关接口封装。

##### tal_utc

实现 UTC 相关接口。

##### tal_util

实现通用工具接口。

##### tal_ble_mbedtls

实现 AES 和 MD5 等加密接口。

##### tal_ble_secure

实现 涂鸦 BLE 配网协议相关的安全接口。

#### Vender 目录

![image-20220419101107254](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220419101107254.png)

Vender 是开发环境所在目录，包含芯片原厂 SDK、各类适配层以及通用头文件，由涂鸦和芯片原厂共同维护。

##### 芯片原厂 SDK

基于芯片原厂的公开例程开发或由芯片原厂开发人员修改而来。

##### 各类适配层

涉及蓝牙、外设驱动（GPIO、PWM、ADC、IIC、SPI……）、系统驱动（Memory、OTA、Sleep……）、工具接口的适配。

##### 通用头文件

为保证 TKL 层以上能够达到一套代码适用于多个芯片平台的目标，Flash 地址、外设引脚等平台相关的因素都通过统一的宏定义放置于 board.h中。

#### 头文件目录

![image-20220419111533203](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220419111533203.png)

##### tuya_error_code.h

涂鸦对错误类型的定义。

##### tuya_cloud_types.h

涂鸦对数据类型、枚举、宏、结构体的定义。

##### tuya_iot_config

涂鸦对系统配置、组件使能/配置的定义。

### 常用 API

#### 初始化

`tuya_init_first()`

一般用于基础外设、配置信息、内存处理相关的初始化。

`tuya_init_second()`

一般用于 Log、软定时、蓝牙基础协议相关的初始化。

`tuya_init_third()`

一般用于复杂外设、外设组件相关的初始化。

`tuya_init_last()`

一般用于初始化的收尾工作，包含蓝牙配网协议的初始化、测试代码的初始化、开启广播等动作。

在执行完该 API 之后，一般会进入主循环。

`tuya_ble_protocol_init`

蓝牙配网协议的初始化。

#### 主循环

`tuya_main_loop()`

TuyaOS BLE 开发包基于前后台软件框架，提供了主循环内的回调接口 `tuya_main_loop()` ，开发者可依据需求自定义相关操作注入该接口。

注意：

（1）该接口的返回值会影响低功耗功能，请勿随意修改；

（2）该接口主要用于开发者添加调试、验证性的操作，需**谨慎使用**。该接口占用过多时间片会影响整个系统框架的稳定性！

#### 事件回调

`tuya_ble_evt_callback`

蓝牙基础协议事件回调处理函数，包含协议栈初始化完成、连接、断开、连接参数更新、接收数据等蓝牙基础事件。

`tuya_ble_protocol_callback`

蓝牙配网协议事件回调处理函数，包含配网成功、时间戳更新成功、接收应用数据、解绑成功、接收OTA数据等应用层常用功能。

#### 数据收发

`TUYA_BLE_CB_EVT_DP_DATA_RECEIVED`

应用层数据接收回调，接收的数据符合 DP 数据格式，请参考 `DP 点` 小节

`tuya_ble_dp_data_send`

应用层数据发送函数

`tuya_ble_dp_data_with_time_send`

应用层数据发送函数（带时间戳）

`TAL_BLE_EVT_WRITE_REQ`

蓝牙基础数据接收回调

`tal_ble_server_common_send`

蓝牙基础数据发送函数

#### 状态查询

tuya_ble_connect_status_get()

```c
typedef enum {
    UNBONDING_UNCONN = 0,  //未绑定未连接    
    UNBONDING_CONN,        //未绑定已连接已认证    
    BONDING_UNCONN,        //已绑定未连接    
    BONDING_CONN,          //已绑定已连接已认证    
    BONDING_UNAUTH_CONN,   //已绑定已连接未认证     
    UNBONDING_UNAUTH_CONN, //未绑定已连接未认证     
    UNKNOW_STATUS 
} tuya_ble_connect_status_t;
```

各状态之间的转换关系如下图所示：

![image-20220419141240911](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220419141240911.png)

### 常用功能

#### 蓝牙数据流

下图演示了 BLE 数据在 SDK 中的流转过程，其他过程类似。

![image-20220419152551810](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220419152551810.png)

表格形式，方便复制

| 手机发送数据                                                 | 手机接收数据                                                 |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| TKL：芯片原厂 BLE 数据接收回调 + tkl_ble_gatt_evt_func       | tkl_ble_gatts_value_notify + 芯片原厂 BLE 数据发送函数       |
| TAL：TKL_BLE_GATT_EVT_WRITE_REQ + tal_ble_event_callback     | tal_ble_server_common_send                                   |
| TUYA：TAL_BLE_EVT_WRITE_REQ + tuya_ble_gatt_receive_data     | tuya_ble_gatt_send_data                                      |
| 接收单包：TUYA_BLE_EVT_MTU_DATA_RECEIVE + tuya_ble_handle_ble_data_evt | tuya_ble_gatt_send_data_enqueue + tuya_ble_gatt_send_data_handle |
| tuya_ble_commonData_rx_proc，组包，解密                      | tuya_ble_commData_send，组包，加密                           |
| 接收指令：TUYA_BLE_EVT_BLE_CMD + tuya_ble_handle_ble_cmd_evt | TUYA_BLE_EVT_DP_DATA_SEND - tuya_ble_handle_dp_data_send_evt |
| tuya_ble_evt_process，指令分发                               | tuya_ble_dp_data_send                                        |
| FRM_DP_DATA_WRITE_REQ + tuya_ble_handle_dp_data_write_req    | app_dp_report                                                |
| TUYA_BLE_CB_EVT_DP_DATA_RECEIVED + app_dp_parser             | ↑                                                            |
| →                                                            | →                                                            |

#### 测试代码

开启测试代码 - 设置宏 TUYA_SDK_TEST 的值为 1。

关闭测试代码 - 设置宏 TUYA_SDK_TEST 的值为 0。

测试代码结合测试上位机（Logic）可实现大部分配网、通信、外设等功能的测试，可帮助开发者更好的开发产品，**但是生产固件请务必关闭测试功能。**

#### Log 管理

![image-20220624164104619](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220624164104619.png)

[上图链接](https://docs.qq.com/sheet/DWlJwc2VqdEtkdGxk?u=f2ae378df6814bb9b6be54fc69d58391&tab=BB08J2)    方便复制

ENABLE_LOG

BOARD_LOG_ENABLE

##### Log 输出接口

tuya_log_output_cb

#### 动态内存

动态内存的大小：HEAP_SIZE

#### Flash 管理

#### 计算固件占用 Flash 和 RAM

计算固件占用的 Flash 和 RAM 之前，请首先完成以下几件事：

（1）关闭测试功能；

（2）关闭 Log 相关的所有宏；

（3）关闭不需要的功能，例如扫描等；

#### 关于驱动

涂鸦通过 TKL 层提供了最小功能集所需的驱动接口，TKL 只是涂鸦标准化的接口，并非所有驱动都有 TKL，为保证开发效率，没有 TKL 的驱动，开发者可以按照实际需求实现。

## 烧录固件

烧录固件跟芯片平台完全相关，请参阅《TuyaOS_BLE_Platform_xxxx》文档

## 授权

详见《[TuyaOS BLE SDK Product Test](https://registry.code.tuya-inc.top/document/platform/-/blob/main/_%E6%B1%87%E6%80%BB/04_%E4%BA%A7%E6%B5%8B/TuyaOS_BLE_SDK_Product_Test.md)》，该文档位于doc文件夹下。

若暂无生产需要，也可通过以下方式进行临时授权（仅用于调试，生产时请改回原状）：

（1）将

![image-20211231144126460](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231144126460.png)

临时修改为

```c++
STATIC tuya_ble_device_param_t tuya_ble_protocol_param = {
    .use_ext_license_key = 1, //1-info in tuya_ble_sdk_demo.h, 0-auth info
    .device_id_len       = DEVICE_ID_LEN, //DEVICE_ID_LEN,
    .p_type              = TUYA_BLE_PRODUCT_ID_TYPE_PID,
    .product_id_len      = 8,
    .adv_local_name_len  = 4,
};
```

## 测试

#### 体验

授权结束后，即成功激活涂鸦BLE设备。

此时可在App Store下载 `涂鸦智能` App，登录后 `添加设备` / 点击 `右上角` → `添加设备` 

![image-20211231145241481](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231145241481.png)

设备添加完成后如下图所示：

![image-20211231145506556](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211231145506556.png)

此时可通过 `涂鸦智能` App 控制 BLE 设备。

### 测试上位机

#### 使用范围

TuyaOS BLE SDK，用于蓝牙协议和外设功能的验证和测试。

#### 获取方式

[下载地址](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/logic/logic.exe)

最新版本请单击 `帮助 → 最新版本` 获取。

#### 基本信息

通信方式：串口

波特率：默认 9600

#### 使用步骤

绿色软件，无需安装，解压至一个**单独文件夹**中双击打开即可。

##### 默认界面

![image-20211229172046966](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172046966.png)

##### 串口设置

可关闭打开串口设置

关闭

![image-20211229172059627](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172059627.png)

打开

![image-20211229172109380](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172109380.png)

##### 调试 - 调试信息

![image-20220620142233705](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220620142233705.png)

可关闭打开调试信息（串口指令）

关闭

![image-20211229172129374](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172129374.png)

打开

![image-20211229172138937](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172138937.png)

##### 设置

![image-20211229172147784](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172147784.png)

恢复出厂设置：对设备发送恢复出厂设置的命令

打开声音：打开声音，进行必要的声音提示

显示行数：前台Log的最大显示行数

定时发送时间：指令定时发送时间，单位ms

##### 帮助

![image-20211229172158480](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172158480.png)

TuyaOS@无线二：了解TuyaOS

最新版本：获取最新版本

### 重点功能测试说明

#### 大数据

第一步：设置PID

当前设置 PID 的功能仅用于大数据测试，设置 PID 之前，如果设备处于绑定状态，需要先解除绑定，清空 App 缓存，并重启 App 进行设备发现。

![image-20211229172225639](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172225639.png)

 

第二步：设置大数据

通过修改下图中的十六进制数据可以修改上报的步数

![image-20211229172234923](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172234923.png)

设置大数据成功后，在 App 产品面板首页下拉刷新，等待大数据同步成功，即可在产品面板第二页 “步数” 一栏看到上报的步数。

#### 天气数据

位置： 

1-配网位置

2-当前位置（手机）



天气参数： 

(1 << 0), /**< temperature. */ 

(1 << 1), /**< high temperature. */ 

(1 << 2), /**< low temperature. */  

(1 << 3), /**< humidity. */ 

(1 << 4), /**< weather condition. */ 

(1 << 5), /**< pressure. */ 

(1 << 6), /**< sendible temperature. */ 

(1 << 7), /**< uvi. */ 

(1 << 8), /**< sunrise. */ 

(1 << 9), /**< sunset. */ 

(1 << 10), /**< unix time, Use with sunrise and sunset. */ 

(1 << 11), /**< local time, Use with sunrise and sunset. */ 

(1 << 12), /**< wind speed. */ 

(1 << 13), /**< wind direction. */ 

(1 << 14), /**< wind speed scale/level. */ 

(1 << 15), /**< aqi. */  

(1 << 16), /**< tips. */ 

(1 << 17), /**< Detailed AQI status and national ranking. */ 

(1 << 18), /**< pm10. */  

(1 << 19), /**< pm2.5. */ 

(1 << 20), /**< o3. */ 

(1 << 21), /**< no2. */ 

(1 << 22), /**< co. */ 

(1 << 23), /**< so2. */ 

(1 << 24), /**< weather condition mapping id. */ 



某天： 

1-7

#### 出厂设置

App端解绑并清除数据——虚拟ID改变

App仅解绑——虚拟ID不改变

本地解绑并清除数据——虚拟ID不改变

本地仅解绑——虚拟ID不改变

#### 扫描功能

如果设备支持扫描功能，可以使用上位机测试，测试方法如下：

![image-20220620114908663](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20220620114908663.png)

开启扫描功能后，要多测试是否会对其他功能产生影响

## OTA

### 协议

参考《Tuya BLE Communication Protocol》中 8.13~8.18 小节。

### 流程

![image-20211229172319849](https://myphotos-1257188211.cos.ap-shanghai.myqcloud.com/img/image-20211229172319849.png)

### 特点

支持双分区

支持断点续传

### 相关代码

#### API

tal_ble_protocol

tal_ble_ota

#### Event

TUYA_BLE_CB_EVT_OTA_DATA

#### MACRO

\#define BOARD_FLASH_OTA_START_ADDR       (0x46000)

\#define BOARD_FLASH_OTA_END_ADDR           (0x66000)

## 默认参数

### 串口参数

串口号：UART0（此处指代TAL层的UART0，具体引脚请参考硬件说明）

波特率：9600

最大数据长度：≥ 200字节

### 蓝牙参数

默认广播间隔：100ms

默认连接间隔：180 ~ 200ms，

OTA连接间隔：15ms

默认连接超时：6000ms

默认MTU ：247

### 通信距离

室内无遮挡无干扰：≥15m

室外无遮挡无干扰：≥30m

### OTA时间

不低于60s/100kB

## 注意事项

外设引脚复用之前务必恢复初始状态

