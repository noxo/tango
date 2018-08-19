LIBS += -lglut \
    -lGL \
    -lGLU \
    -lX11 \
    -lturbojpeg \
    -lpng \
    -lzip
TARGET = test
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += \
../common/data/dataset.cc \
../common/data/file3d.cc \
../common/data/image.cc \
../common/data/mesh.cc \
../common/editor/rasterizer.cc \
../common/gl/camera.cc \
../common/gl/glsl.cc \
../common/gl/renderer.cc \
../common/postproc/medianer.cc \
app.cpp
HEADERS += \
../common/data/dataset.h \
../common/data/file3d.h \
../common/data/image.h \
../common/data/mesh.h \
../common/editor/rasterizer.h \
../common/gl/opengl.h \
../common/postproc/medianer.cc
INCLUDEPATH += ../common \
../third_party
