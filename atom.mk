
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libaudio-raw
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := Raw audio library
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -DARAW_API_EXPORTS -fvisibility=hidden -std=gnu99
LOCAL_SRC_FILES := \
	src/araw.c \
	src/araw_reader.c \
	src/araw_writer.c

LOCAL_LIBRARIES := \
	libaudio-defs \
	libulog

include $(BUILD_LIBRARY)
