LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
	LOCAL_MODULE := liblas
	LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
	LOCAL_SRC_FILES := 	$(call all-subdir-cpp-files)
	LOCAL_CFLAGS 	+= -DUNORDERED
include $(BUILD_SHARED_LIBRARY)

