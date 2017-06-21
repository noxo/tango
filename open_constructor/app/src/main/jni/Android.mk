LOCAL_PATH := $(call my-dir)
PROJECT_ROOT_FROM_JNI:= ../../../../..
PROJECT_ROOT:= $(call my-dir)/../../../../..

include $(CLEAR_VARS)
LOCAL_MODULE           := libopenconstructor
LOCAL_SHARED_LIBRARIES := tango_client_api tango_3d_reconstruction tango_support
LOCAL_STATIC_LIBRARIES := jpeg-turbo png
LOCAL_CFLAGS           := -std=c++11

LOCAL_C_INCLUDES := $(PROJECT_ROOT)/third_party/glm/ \
                    $(PROJECT_ROOT)/third_party/libjpeg-turbo/include/ \
                    $(PROJECT_ROOT)/third_party/libpng/include/

LOCAL_SRC_FILES := app.cc \
                   scene.cc \
                   data/file3d.cc \
                   data/image.cc \
                   data/mesh.cc \
                   editor/effector.cc \
                   editor/rasterizer.cc \
                   editor/selector.cc \
                   gl/camera.cc \
                   gl/glsl.cc \
                   gl/renderer.cc \
                   tango/scan.cc \
                   tango/service.cc \
                   tango/texturize.cc

LOCAL_LDLIBS    := -llog -lGLESv2 -L$(SYSROOT)/usr/lib -lz -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT))
$(call import-add-path, $(PROJECT_ROOT)/third_party)
$(call import-module,libjpeg-turbo)
$(call import-module,libpng)
$(call import-module,tango_client_api)
$(call import-module,tango_3d_reconstruction)
$(call import-module,tango_support)
