# 模板平台

## 目录结构

![nrf52832_ble](https://images.tuyacn.com/rms-static/db66cc80-4088-11ec-bf56-238df7ae6cb1-1636371708232.png?tyName=nrf52832_ble.png "nrf52832_ble")

| 一级目录 | 二级目录 | 说明 |
| :------------ | :------------ | :------------ |
| sdk | nRF5_SDK_15.3.0_59ac345 | 芯片原厂SDK |
|  | port | 芯片原厂SDK对应的port接口，**main函数放在这里** |
| toolchain | templates | 编译工具链/开发环境 |
| tuyaos | bluetooth | 蓝牙 - TKL接口实现 |
|  | drivers | 驱动 - TKL接口实现，**详见下文** |
|  | include | TKL层必要的头文件，可能没有 |
|  | system | 系统 - TKL接口实现，**详见下文** |
|  | utilities | 工具 - TKL接口实现<br>此处的.c文件从.\adapter\utilities\src拷贝过来，具备跨平台特性，严禁改动 |
| prepare.sh | / | vendor的预处理脚本，比如拉取原厂SDK等 |
| README.md | / | myself |


## drivers 接口介绍

| 函数 | 说明 |
| :------------ | :------------- |
| `tkl_adc.c` | adc - TKL接口实现 |
| `tkl_display.c` | display - TKL接口实现，暂未使用 |
| `tkl_flash.c` | flash - TKL接口实现 |
| `tkl_gpio.c` | gpio - TKL接口实现 |
| `tkl_i2c.c` | i2c - TKL接口实现 |
| `tkl_pwm.c` | pwm - TKL接口实现 |
| `tkl_rtc.c` | rtc - TKL接口实现 |
| `tkl_spi.c` | spi - TKL接口实现 |
| `tkl_uart.c` | uart - TKL接口实现 |
| `tkl_watchdog.c` | watchdog - TKL接口实现 |


## system 接口介绍

| 函数 | 说明 |
| :------------ | :------------ |
| `tkl_memory.c` | memory - TKL接口实现，通过调用“utilities\tuya_mem_heap”实现，具备跨平台特性，无需更改 |
| `tkl_mutex.c` | mutex - TKL接口实现，通过调用“tkl_system”实现，具备跨平台特性，无需更改 |
| `tkl_ota.c` | ota - TKL接口实现 |
| `tkl_output.c` | output - TKL接口实现，暂未使用 |
| `tkl_sleep.c` | sleep - TKL接口实现 |
| `tkl_sw_timer.c` | sw_timer - TKL接口实现，通过调用“tkl_system”实现，具备跨平台特性，无需更改 |
| `tkl_system.c` | system - TKL接口实现 |



test 



