LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE   := AudioReceivePlayerNative
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := -O3 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
###

LOCAL_SRC_FILES := opensl_io.c \
java_interface_wrap.cpp 

LOCAL_LDLIBS := -llog -lOpenSLES

include $(BUILD_SHARED_LIBRARY)


