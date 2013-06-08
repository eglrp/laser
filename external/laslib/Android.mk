LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
	LOCAL_MODULE := liblas
	LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc \
					    external/stlport/stlport \
					    external/protobuf/src \
					    bionic
	LOCAL_SRC_FILES := 	$(call all-subdir-cpp-files)
	LOCAL_CFLAGS 	+= -DUNORDERED
	LOCAL_CPPFLAGS += -fexceptions -frtti
include $(BUILD_STATIC_LIBRARY)

