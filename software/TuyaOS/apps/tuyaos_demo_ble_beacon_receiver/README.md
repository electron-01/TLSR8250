# tuyaos_ble_beacon_receiver_demo

# BLE-BEACON Receiver Demo

## Vendor TLSR8250：

在 文件 IotOsconfig  中添加新的条目：

config ENABLE_SCAN

    bool "ENABLE_SCAN"

    default y

    config ENABLE_LOG
        bool "ENABLE_LOG"
        default y
    
    config BOARD_ENABLE_LOG
        bool "BOARD_ENABLE_LOG"
        default y
    
    config TUYA_SDK_TEST
        bool "TUYA_SDK_TEST"
        default y
    
    config BOARD_HEAP_SIZE
        int "BOARD_HEAP_SIZE"
        default 5120 
    
    config CLOCK_SYS_CLOCK_HZ
        int "CLOCK_SYS_CLOCK_HZ"
        default 48000000 
    
    config ENABLE_SCAN
        bool "ENABLE_SCAN"
        default y

同时为了执行效率， 建议把主频提升到 48000000，即 CLOCK_SYS_CLOCK_HZ 的值

并且通过 config project 使其生效。

## Vendor PHY62222：

  工程编译前需要将 HOST_CONFIG 的值设置为6， 否则无法开启扫描功能。

### 设置方法一：

    打开keil，找到菜单栏的魔法棒（Options for Target）然后选择 C/C++ tag ，在Preprocessor Symbles 里面的Define 中找到HOST_CONFIG，将其值设置为6即可

### 设置方法二：

    在工程跟目录中找到.log 文件夹，用记事本打开 Demo.uvprojx ，找到 HOST_CONFIG 直接改成6

## 

## 关键代码说明

涉及组件：tal_ble_beacon_remoter

事件流程：

    扫描到的广播数据在回调事件： TAL_BLE_EVT_ADV_REPORT 中。

        在该事件中完成对数据的打包，并抛给用户事件：APP_EVT_0

    APP_EVT_0中进行数据的进一步打包，并将打包好的数据抛给应用层，应用层函数： app_receiver_data_handler 。在app_receiver_data_handler 中数据解析完成后，会将解析好的数据抛到函数  “app_receiver_cmd_parser” 中进行具体的逻辑处理。

代码中 ，通过 宏 TUYA_SDK_TEST 包围起来的部分都是SDK测试代码， 用户使用时无需关注 。也可以通过 IotOsconfig 将测试代码屏蔽（会省出不少内存）。