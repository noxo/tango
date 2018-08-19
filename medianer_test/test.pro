LIBS += -lglut \
    -lGL \
    -lGLU \
    -lX11 \
    -lturbojpeg \
    -lpng \
    -lzip \
    -lopencv_calib3d \
    -lopencv_core \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_imgcodecs \
    -lopencv_xfeatures2d \
    -lglog \
    -lceres \
    -fopenmp
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
../third_party/libmv/correspondence/feature_matching.cc \
../third_party/libmv/correspondence/matches.cc \
../third_party/libmv/correspondence/nRobustViewMatching.cc \
../third_party/libmv/multiview/conditioning.cc \
../third_party/libmv/multiview/euclidean_resection.cc \
../third_party/libmv/multiview/fundamental.cc \
../third_party/libmv/multiview/fundamental_kernel.cc \
../third_party/libmv/multiview/homography.cc \
../third_party/libmv/multiview/panography.cc \
../third_party/libmv/multiview/panography_kernel.cc \
../third_party/libmv/multiview/projection.cc \
../third_party/libmv/multiview/robust_estimation.cc \
../third_party/libmv/multiview/robust_fundamental.cc \
../third_party/libmv/multiview/robust_resection.cc \
../third_party/libmv/multiview/triangulation.cc \
../third_party/libmv/multiview/twoviewtriangulation.cc \
../third_party/libmv/numeric/numeric.cc \
../third_party/libmv/numeric/poly.cc \
../third_party/libmv/simple_pipeline/bundle.cc \
../third_party/libmv/simple_pipeline/camera_intrinsics.cc \
../third_party/libmv/simple_pipeline/distortion_models.cc \
../third_party/libmv/simple_pipeline/initialize_reconstruction.cc \
../third_party/libmv/simple_pipeline/intersect.cc \
../third_party/libmv/simple_pipeline/keyframe_selection.cc \
../third_party/libmv/simple_pipeline/pipeline.cc \
../third_party/libmv/simple_pipeline/reconstruction.cc \
../third_party/libmv/simple_pipeline/reconstruction_scale.cc \
../third_party/libmv/simple_pipeline/resect.cc \
../third_party/libmv/simple_pipeline/tracks.cc \
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
../third_party \
/usr/include/eigen3
