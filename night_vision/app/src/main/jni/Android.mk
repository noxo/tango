LOCAL_PATH := $(call my-dir)
PROJECT_ROOT_FROM_JNI:= ../../../../..
PROJECT_ROOT:= $(call my-dir)/../../../../..

include $(CLEAR_VARS)
LOCAL_MODULE           := libopenconstructor
LOCAL_SHARED_LIBRARIES := tango_client_api tango_3d_reconstruction tango_support

LOCAL_C_INCLUDES := $(PROJECT_ROOT)/third_party/glm/ \
                    $(PROJECT_ROOT)/third_party/libjpeg-turbo/include/ \
                    $(PROJECT_ROOT)/third_party/libpng/include/ \
                    $(PROJECT_ROOT)/common/

LOCAL_SRC_FILES := ../../../../../common/data/dataset.cc \
                   ../../../../../common/gl/glsl.cc \
                   ../../../../../common/tango/service.cc \
                   app.cc

LOCAL_LDLIBS    := -llog -lGLESv3 -L$(SYSROOT)/usr/lib -lz -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT))
$(call import-add-path, $(PROJECT_ROOT)/third_party)
$(call import-module,tango_client_api)
$(call import-module,tango_3d_reconstruction)
$(call import-module,tango_support)
