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

#define GLM_FORCE_RADIANS

#include <jni.h>
#include "mesh_builder/mesh_builder_app.h"

static mesh_builder::MeshBuilderApp app;

#ifdef __cplusplus
extern "C" {
#endif

std::string jstring2string(JNIEnv* env, jstring name)
{
  const char *s = env->GetStringUTFChars(name,NULL);
  std::string str( s );
  env->ReleaseStringUTFChars(name,s);
  return str;
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_activityCtor(jboolean t3dr_running) {
  app.ActivityCtor(t3dr_running);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onCreate(
JNIEnv* env, jobject, jobject activity) {
  app.OnCreate(env, activity);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onTangoServiceConnected(
    JNIEnv* env, jobject, jobject iBinder) {
  app.OnTangoServiceConnected(env, iBinder);
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
Java_com_lvonasek_openconstructor_TangoJNINative_set3D(
        JNIEnv*, jobject, jdouble res, jdouble min, jdouble max) {
  app.TangoSetup3DR(res, min, max);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_load(JNIEnv* env, jobject, jstring name) {
  app.Load(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_save(JNIEnv* env, jobject, jstring name) {
  app.Save(jstring2string(env, name));
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
Java_com_lvonasek_openconstructor_TangoJNINative_centerOfStaticModel(JNIEnv*, jobject, jboolean horizontal) {
  return app.CenterOfStaticModel(horizontal);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_filter(JNIEnv* env, jobject, jstring oldname,
                                                        jstring newname, jint passes) {
  mesh_builder::MeshBuilderApp::Filter(jstring2string(env, oldname),
                                       jstring2string(env, newname), passes);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_setLandscape(JNIEnv*, jobject, jboolean on) {
  return app.SetLandscape(on);
}

#ifdef __cplusplus
}
#endif
