# ble_mdev_test_demo

BLE mdev test demo

初始化后打开一个2000ms的定时器（ app_mdev_timer ），在定时期间，统计扫到的 广播名为 mdev 的广播，并进行统计， 当平均RSSI  > -40 且 < 0 时认为满足 需求，此时允许进入信标产测模式（相关逻辑由用户自行补充）。