# tuyaos_demo_ble_thermohygrometer

# BLE 温湿度品类Demo 涉及能力

- 低功耗的使用
- BLE-Beacon 能力与对应组件的使用
- 历史数据上报
- 温湿度产品逻辑框架
  
  

## 1. 低功耗的使用

设备烧录完成后有授权的当作 ，所以设备上电后的第一秒不允许进入休眠，以保证授权功能的完整。

通过调用 tal_cpu_allow_sleep 接口允许设备进入休眠状态。

休眠分两种：

- 一、深度休眠（deep sleep）休眠后仅仅保持SRAM的工作，其余外设全部停止，唤醒后需要重新初始化外设（如TLSR8250 需要用户手动添加硬件初始化代码， 其他平台不需要）

- 二、普通休眠（sleep）仅MCU 暂停工作， 外设保持原先的工作状态，唤醒后不需要再重新初始化。

这两种休眠模式默认一般由SDK内部自行管理，用户只要关注唤醒后的硬件初始化。需要用户自行处理的平台则在 OPERATE_RET tuya_init_third(VOID_T)这里添加硬件初始化的代码，该函数会在每次从deepsleep状态下唤醒的时候被执行。

## 2. tal_ble_beacon组件的使用

通过 ble-beacon 上报数据实际上就是将DP数据放进广播包中进行上报。首先需要先将宏定义  TAL_BLE_BEACON_INCLUDE_DP_DATA  置成 1（该宏在文件IoTSconfig 中设置好了，用户只需要在应用文件夹下右击选择 config即可配置。）

这种上报方式有两个缺点：

- 一、不可靠性，因为不能保证一次就上报成功，所以需要持续、密集地上报一段时间 ，推荐值是以100ms的广播间隔持续发1s，用户可以根据自身需求和功耗要求来制定上报策略。

- 二、低效性：因为广播包还有其他的数据需要填充， 留给DP 的空间就只有几个字节，所以一般情况下一个ble-beacon广播包不会超过2个DP，如果同时有超过2个DP需要上报则需要分多次填充数据。

当然优点也是很鲜明的，就是可以搭配网关或者其他可接收、解析 ble-beacon的设备实现非连接下的数据交互。

demo中的文件app_dp_reporter.c 中用宏 ENABLE_BEACON_REPORT 框起来的代码都是对该组件的使用，应用代码可以参考这部分的代码。

## 3. 历史数据上报

温湿度历史数据的上报用到了SDK中的 bulkdata 能力，使用该能力的优势在于可以实现短时间内大量数据的批量上报。

首先需要先将宏定义 TUYA_BLE_FEATURE_BULKDATA_ENABLE  置成1（该宏在文件IoTSconfig 中设置好了，用户只需要在应用文件夹下右击选择 config即可配置。）

历史记录的使用见文件 app_history_data.c 在文件的内的宏定义中定义了几个重要的数值

- 一、 数据格式定义

```
// do not change the 2 define below
#define TUYA_BLE_BULKDATA_BLOCK_SIZE 512
#define TLD_SIZE    32
```

这两个宏分别是告知APP每次请求数据的最大size（512） 和 每个数据点的大小（32字节），这两个值用户没有特殊需求最好不要修改

再往下就是定义历史数据

- 二、 数据存储地址
  
  这里是以TLSR8250为开发平台定义的FLASH空间地址。用户在使用的时候要确保地址是能够使用，并最好是连续的，如果不连续则需要自行编写兼容程序。

```
// data storage address.
#define APP_BULKDATA_START_ADDR 0x2C000
#define APP_BULKDATA_END_ADDR   0x3F000
#define APP_BULKDATA_PAGE_NUM ((APP_BULKDATA_END_ADDR - APP_BULKDATA_START_ADDR) / 0
```

可以通过这个网址来查询不同平台开给用户使用的Flash分布情况： [开发平台-TuyaOS-涂鸦开发者](https://developer.tuya.com/cn/docs/iot-device-dev/bluetooth_platform?id=Kc6fd92l28kkm) 

- 三、历史数据存储间隔
  
  历史数据存储以秒为单位， 这里每隔300s记录一条历史数据。用户可以根据需求自定义存储间隔。同时需要留意的是 当设备处于历史数据上报的时候不能进行历史数据记录，因为这样可能会导致APP对总数据的CRC校验的计算值和设备上报的CRC校验值不同，从而导致本轮上报失败。

```
// 300 seconds (5 minute) record one history data
#define APP_HISTORY_RECORD_INTERVAL (300)
```

- 快速测试
  
  将该宏置为1， 可以让设备在上电的时候检测历史记录存储区域的数据，如果没有数据，则自动生成历史记录。

```
// automatic generate history data when device power on and history data empty
#define HISTORY_TEST 0
```

历史数据上报交互流程：

- 温湿度设备 与 app 完成 BLE 连接

- 用户进入设备面板，APP 主动发起大数据请求

- 温湿度设备请求响应，响应包中包括 即将上报的历史数据总长度（字节）和总数据的CRC32

- APP 告知温湿度设备要读取的数据的Block 索引

- 温湿度设备通过索引换算出要读取的Flash地址，并将该地址的数据读出一个block的大小上报。

- APP不断重复 步骤4 和 步骤5，直到上报完成

- APP计算收到的数据的CRC32值并于温湿度设备上报的CRC32比对，如果正确则上报云端。 并在上报云端完成后下发 reset命令，告知设备数据上报完成，可以删除保存的数据了。



## 5. 温湿度数据处理框架

具体代码实现见文件app_sensor.c。该demo中的温湿度数据均为模拟，真实的使用需要用户根据自己的硬件实现 如下三个函数：

```
VOID_T app_sensor_hw_init(VOID_T)//即外设接口的初始化
STATIC OPERATE_RET app_sensor_trigger(VOID_T)//触发温湿度数据读取
STATIC OPERATE_RET app_sensor_data_read(VOID_T)// 读取温湿度数据

```

- 数据更新
  
  大部分温湿度传感器在促发温湿度采集后一般都要等待百毫秒的时间。所以为了兼顾低功耗设计，我们开了一个每2s超时一次的定时器，通过这个定时器数据更新做成2个阶段， 第一个阶段触发温湿度采集，第二个阶段读取采集到的数据并将数据用作相关逻辑的计算。所以从用户的角度来看 设备每4s更新一次数据。当然也可以将这个周期缩小，但功耗也会相应提高。

- 灵敏度设置
  
  温湿度灵敏度的设置是为了解决在两次上报周期中间温湿度变化过于激烈时的数据上报的问题。灵敏度比对需要两个数据，一个是本次采集到数据 ，另一个是最近一次上报的数据。