
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := amem_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"amem\" \
    -DMODULE_NAME=amem

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    amem.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := audiotrack_android_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"audiotrack_android\" \
    -DMODULE_NAME=audiotrack_android

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src

LOCAL_SRC_FILES := \
    androidaudiotrack.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_ARM),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_MODULE := opensles_android_plugin

LOCAL_CFLAGS += \
    -std=gnu99 \
    -DHAVE_CONFIG_H \
    -D__PLUGIN__ \
    -DMODULE_STRING=\"opensles_android\" \
    -DMODULE_NAME=opensles_android

LOCAL_C_INCLUDES += \
    $(VLCROOT) \
    $(VLCROOT)/include \
    $(VLCROOT)/src \
    $(DEPROOT)/include

LOCAL_SRC_FILES := \
    opensles_android.c

LOCAL_SHARED_LIBRARIES += libvlccore

include $(BUILD_SHARED_LIBRARY)

