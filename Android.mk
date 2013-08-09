LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
	LOCAL_MODULE_TAGS := optional
	LOCAL_MODULE := ecore

	LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc \
						$(LOCAL_PATH)/external/pcl_android \
						$(LOCAL_PATH)/external/ \
						$(LOCAL_PATH)/external/laslib/inc
						

	LOCAL_SRC_FILES := $(call all-cpp-files-under,src) $(call all-c-files-under,src)  
	
	LOCAL_CFLAGS += -fexceptions -DANDROID_OS -DE_PRINT -D__ANDROID__ -DANDROID
	LOCAL_CPPFLAGS += -fexceptions -frtti
	
	LOCAL_LDFLAGS += $(LOCAL_PATH)/libs/arm/std/libstdc++.a -Wl,-v
 	LOCAL_LDLIBS :=	 -L$(LOCAL_PATH)/libs/arm/pcl -lpcl_common -lpcl_io \
			 		 -L$(LOCAL_PATH)/libs/arm/gif -lgif \
			 		 -L$(LOCAL_PATH)/libs/arm/boost \
			 		 -lboost_thread -lboost_system -lboost_filesystem -llog
 
	LOCAL_STATIC_LIBRARIES := liblas libminini libjpeg

	LOCAL_PRELINK_MODULE := false
#include $(BUILD_EXECUTABLE)
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/external/Android.mk