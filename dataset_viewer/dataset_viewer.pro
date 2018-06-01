LIBS += -lglut \
    -lGL \
    -lGLU \
    -lX11 \
    -lturbojpeg \
    -lpng \
    -lzip
TARGET = dataset_viewer
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += \
../common/data/file3d.cc \
../common/data/image.cc \
../common/data/mesh.cc \
app.cpp
HEADERS += \
../common/data/file3d.h \
../common/data/image.h \
../common/data/mesh.h \
../common/gl/opengl.h
INCLUDEPATH += ../common
