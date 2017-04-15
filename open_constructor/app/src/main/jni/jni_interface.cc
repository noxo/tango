/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include "mesh_builder_app.h"

static oc::MeshBuilderApp app;

std::string jstring2string(JNIEnv* env, jstring name)
{
  const char *s = env->GetStringUTFChars(name,NULL);
  std::string str( s );
  env->ReleaseStringUTFChars(name,s);
  return str;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onCreate(
JNIEnv* env, jobject, jobject activity) {
  app.OnCreate(env, activity);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onTangoServiceConnected(JNIEnv* env, jobject,
          jobject iBinder, jdouble res, jdouble dmin, jdouble dmax, jint noise, jboolean land,
                                             jboolean photo, bool textures, jstring dataset,
                                             jfloat deviceMatrixRotation) {
  app.OnTangoServiceConnected(env, iBinder, res, dmin, dmax, noise, land, photo, textures,
  jstring2string(env, dataset), deviceMatrixRotation);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onPause(JNIEnv*, jobject) {
  app.OnPause();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onGlSurfaceCreated(JNIEnv*, jobject) {
  app.OnSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onGlSurfaceChanged(
    JNIEnv*, jobject, jint width, jint height) {
  app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onGlSurfaceDrawFrame(JNIEnv*, jobject) {
  app.OnDrawFrame();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onToggleButtonClicked(
    JNIEnv*, jobject, jboolean t3dr_is_running) {
  app.OnToggleButtonClicked(t3dr_is_running);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onClearButtonClicked(JNIEnv*, jobject) {
  app.OnClearButtonClicked();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_load(JNIEnv* env, jobject, jstring name) {
  app.Load(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_save(JNIEnv* env, jobject, jstring name, jstring d) {
  app.Save(jstring2string(env, name), jstring2string(env, d));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_setView(JNIEnv*, jobject, jfloat pitch, jfloat yaw,
                                                         jfloat x, jfloat y, jboolean gyro) {
  app.SetView(pitch, yaw, x, y, gyro);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_setZoom(JNIEnv*, jobject, jfloat value) {
  app.SetZoom(value);
}

JNIEXPORT jfloat JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_centerOfStaticModel(JNIEnv*, jobject, jboolean h) {
  return app.CenterOfStaticModel(h);
}

JNIEXPORT jboolean JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_isPhotoFinished(JNIEnv*, jobject) {
  return (jboolean) app.IsPhotoFinished();
}

#ifndef NDEBUG
JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_clientSecret(JNIEnv* env, jobject) {
  std::string message = "NO SECRET";
  int byteCount = message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}
#else
#include "secret.h"
JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_clientSecret(JNIEnv* env, jobject) {
  std::string message = secret();
  int byteCount = message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}
#endif

#ifdef __cplusplus
}
#endif
