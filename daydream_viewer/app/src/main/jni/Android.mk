LOCAL_PATH := $(call my-dir)
PROJECT_ROOT_FROM_JNI:= ../../../../..
PROJECT_ROOT:= $(call my-dir)/../../../../..

include $(CLEAR_VARS)
LOCAL_MODULE    := libdaydream
LOCAL_CFLAGS    := -std=c++11
LOCAL_SHARED_LIBRARIES := gvr
LOCAL_STATIC_LIBRARIES := png

LOCAL_C_INCLUDES += $(PROJECT_ROOT)/third_party/glm/ \
                    $(PROJECT_ROOT)/third_party/libpng/include/ \
                    $(LOCAL_PATH)/../../../../../open_constructor/app/src/main/jni/

LOCAL_CFLAGS    += -DNOTANGO
LOCAL_SRC_FILES := renderer_jni.cc \
                   renderer.cc \
                   ../../../../../open_constructor/app/src/main/jni/data/file3d.cc \
                   ../../../../../open_constructor/app/src/main/jni/data/image.cc \
                   ../../../../../open_constructor/app/src/main/jni/data/mesh.cc

LOCAL_LDLIBS    := -llog -lGLESv2 -L$(SYSROOT)/usr/lib -lz -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT))
$(call import-add-path, $(PROJECT_ROOT)/third_party)
$(call import-module,gvr)
$(call import-module,libpng)