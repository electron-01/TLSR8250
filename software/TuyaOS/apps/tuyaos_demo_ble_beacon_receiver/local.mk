# 当前文件所在目录
LOCAL_PATH := $(call my-dir)

#---------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

# 当前模块名
LOCAL_MODULE := $(notdir $(LOCAL_PATH))

# 模块对外头文件（只能是目录）
# 加载至CFLAGS中提供给其他组件使用；打包进SDK产物中；
LOCAL_TUYA_SDK_INC := $(LOCAL_PATH)/include
ifneq ($(APP_PACK_FLAG), 1) 
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)
endif

# 模块对外CFLAGS：其他组件编译时可感知到
LOCAL_TUYA_SDK_CFLAGS := -DUSER_SW_VER=\"$(APP_VER)\" -DAPP_BIN_NAME=\"$(APP_NAME)\"

# 模块源代码
LOCAL_SRC_FILES := $(foreach dir, $(LOCAL_PATH)/src, $(wildcard $(dir)/*.c))     #<-- 千万不能用 shell find 去查，不然 windows 不兼容
LOCAL_SRC_FILES += $(foreach dir, $(LOCAL_PATH)/application_components, $(wildcard $(dir)/*.c))
LOCAL_SRC_FILES += $(foreach dir, $(LOCAL_PATH)/application_drivers, $(wildcard $(dir)/*.c))

# 模块内部CFLAGS：仅供本组件使用
LOCAL_CFLAGS :=

# 全局变量赋值
TUYA_SDK_INC += $(LOCAL_TUYA_SDK_INC)  # 此行勿修改
TUYA_SDK_CFLAGS += $(LOCAL_TUYA_SDK_CFLAGS)  # 此行勿修改

# 生成静态库
include $(BUILD_STATIC_LIBRARY)

# 生成动态库
include $(BUILD_SHARED_LIBRARY)

# 导出编译详情
include $(OUT_COMPILE_INFO)

#---------------------------------------

