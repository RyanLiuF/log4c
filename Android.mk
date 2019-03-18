LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE 	:= log4c

LOCAL_MODULE_TAGS := eng

LOCAL_CPP_EXTENSION := .hpp .cpp .h

LOCAL_SRC_FILES :=  module.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					$(LOCAL_PATH)/../Public \
					$(LOCAL_PATH)/../Public/include \

LOCAL_CFLAGS  := -w -O2 -g -W -Wall -DLOG_DEBUG -DANDROID -fPIC -std=c++11
LOCAL_CPP_FEATURES += rtti exceptions
include $(BUILD_SHARED_LIBRARY)
