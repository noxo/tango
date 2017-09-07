#include <android/log.h>
#include <jni.h>

#include <memory>

#include "renderer.h"  // NOLINT
#include "vr/gvr/capi/include/gvr.h"

std::string jstring2string(JNIEnv* env, jstring name)
{
  const char *s = env->GetStringUTFChars(name,NULL);
  std::string str( s );
  env->ReleaseStringUTFChars(name,s);
  return str;
}

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_lvonasek_daydreamOBJ_MainActivity_##method_name

namespace {

inline jlong jptr(Renderer *renderer_instance) {
  return reinterpret_cast<intptr_t>(renderer_instance);
}

inline Renderer *native(jlong ptr) {
  return reinterpret_cast<Renderer *>(ptr);
}
}  // anonymous namespace

extern "C" {

JNI_METHOD(jlong, nativeCreateRenderer)
(JNIEnv *env, jclass clazz, jobject class_loader, jobject android_context,
 jlong native_gvr_api, jstring filename, jint w, jint h) {
  return jptr(new Renderer(reinterpret_cast<gvr_context *>(native_gvr_api),
  jstring2string(env, filename), w, h));
}

JNI_METHOD(void, nativeDestroyRenderer)
(JNIEnv *env, jclass clazz, jlong renderer_instance) {
  delete native(renderer_instance);
}

JNI_METHOD(void, nativeInitializeGl)
(JNIEnv *env, jobject obj, jlong renderer_instance) {
  native(renderer_instance)->InitializeGl();
}

JNI_METHOD(void, nativeDrawFrame)
(JNIEnv *env, jobject obj, jlong renderer_instance) {
  native(renderer_instance)->DrawFrame();
}

JNI_METHOD(void, nativeOnTriggerEvent)
(JNIEnv *env, jobject obj, jlong renderer_instance, jfloat value) {
  native(renderer_instance)->OnTriggerEvent(value);
}

JNI_METHOD(void, nativeOnPause)
(JNIEnv *env, jobject obj, jlong renderer_instance) {
  native(renderer_instance)->OnPause();
}

JNI_METHOD(void, nativeOnResume)
(JNIEnv *env, jobject obj, jlong renderer_instance) {
  native(renderer_instance)->OnResume();
}

}  // extern "C"
