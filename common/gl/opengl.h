#ifndef GL_OPENGL_H
#define GL_OPENGL_H

#ifdef ANDROID
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <algorithm>
#include <vector>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#ifdef ANDROID
#include <android/log.h>
#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "tango_app", __VA_ARGS__)
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, "tango_app", __VA_ARGS__)
#else
#define LOGI(...); \
  printf(__VA_ARGS__); printf("\n")
#define LOGE(...); \
  printf(__VA_ARGS__); printf("\n")
#endif


#endif
