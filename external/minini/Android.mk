LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
	LOCAL_MODULE := libminini
	LOCAL_SRC_FILES := 	$(call all-subdir-c-files)
	LOCAL_CFLAGS 	+=  -DANDROID_OS
include $(BUILD_SHARED_LIBRARY)

