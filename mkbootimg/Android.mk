
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),aries)
#LOCAL_CFLAGS := -DUSE_DROID_RAMFS
LOCAL_CFLAGS := -DUSE_INIT_RAMFS
endif

LOCAL_SRC_FILES := mkbootimg.c
LOCAL_STATIC_LIBRARIES := libmincrypt

LOCAL_MODULE := mkbootimg

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,dist_files,$(LOCAL_BUILT_MODULE))
