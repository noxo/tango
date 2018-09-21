# -------------------------------------------------
# Project created by QtCreator 2011-11-11T16:55:36
# -------------------------------------------------
LIBS += -lglut \
    -lGL \
    -lGLU \
    -lX11 \
    -lturbojpeg \
    -lpng \
    -lzip
TARGET = glut_viewer
CONFIG += console
CONFIG -= app_bundle
QMAKE_CXXFLAGS += -std=c++11
TEMPLATE = app
SOURCES += \
../common/data/file3d.cc \
../common/data/image.cc \
../common/data/mesh.cc \
../common/editor/effector.cc \
../common/editor/rasterizer.cc \
../common/editor/selector.cc \
../common/gl/camera.cc \
../common/gl/glsl.cc \
../common/gl/renderer.cc \
../common/gl/scene.cc \
app.cpp
HEADERS += \
../common/data/file3d.h \
../common/data/image.h \
../common/data/mesh.h \
../common/editor/effector.h \
../common/editor/rasterizer.h \
../common/editor/selector.h \
../common/gl/camera.h \
../common/gl/glsl.h \
../common/gl/renderer.h \
../common/gl/scene.h
INCLUDEPATH += ../common
