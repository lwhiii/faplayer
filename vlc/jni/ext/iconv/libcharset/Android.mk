
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := libcharset

LOCAL_CFLAGS += \
    -DHAVE_CONFIG_H \
    -DBUILDING_LIBCHARSET \
    -DIN_LIBRARY \
    -DLIBDIR=\"\"

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    lib/localcharset.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

