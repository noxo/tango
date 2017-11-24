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
                    $(PROJECT_ROOT)/third_party/libpng/include/ \
                    $(PROJECT_ROOT)/common/ \
                    $(PROJECT_ROOT)/common/poisson

LOCAL_SRC_FILES := ../../../../../common/data/file3d.cc \
                   ../../../../../common/data/image.cc \
                   ../../../../../common/data/mesh.cc \
                   ../../../../../common/editor/effector.cc \
                   ../../../../../common/editor/rasterizer.cc \
                   ../../../../../common/editor/selector.cc \
                   ../../../../../common/gl/camera.cc \
                   ../../../../../common/gl/glsl.cc \
                   ../../../../../common/gl/renderer.cc \
                   ../../../../../common/poisson/CmdLineParser.cpp \
                   ../../../../../common/poisson/Factor.cpp \
                   ../../../../../common/poisson/Geometry.cpp \
                   ../../../../../common/poisson/MarchingCubes.cpp \
                   ../../../../../common/poisson/PlyFile.cpp \
                   ../../../../../common/poisson/PoissonRecon.cpp \
                   ../../../../../common/tango/scan.cc \
                   ../../../../../common/tango/service.cc \
                   ../../../../../common/tango/texturize.cc \
                   app.cc \
                   scene.cc

LOCAL_LDLIBS    := -llog -lGLESv3 -L$(SYSROOT)/usr/lib -lz -landroid
include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(PROJECT_ROOT))
$(call import-add-path, $(PROJECT_ROOT)/third_party)
$(call import-module,libjpeg-turbo)
$(call import-module,libpng)
$(call import-module,tango_client_api)
$(call import-module,tango_3d_reconstruction)
$(call import-module,tango_support)
