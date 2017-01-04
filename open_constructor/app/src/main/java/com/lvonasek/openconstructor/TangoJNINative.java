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

package com.lvonasek.openconstructor;

import android.app.Activity;
import android.os.IBinder;
import android.util.Log;

/**
 * Interfaces between native C++ code and Java code.
 */
public class TangoJNINative {
  static {
    // This project depends on tango_client_api, so we need to make sure we load
    // the correct library first.
    if (TangoInitHelper.loadTangoSharedLibrary() ==
        TangoInitHelper.ARCH_ERROR) {
      Log.e("TangoJNINative", "ERROR! Unable to load library!");
    }
    System.loadLibrary("openconstructor");
  }

  /**
   * Interfaces to native ActivityCtor function.
   *
   * @param reconstructionRunning the initial state of reconstruction.
   */
  public static native void activityCtor(boolean reconstructionRunning);

  /**
   * Interfaces to native OnCreate function.
   *
   * @param callerActivity the caller activity of this function.
   */
  public static native void onCreate(Activity callerActivity);

  /**
   * Called when the Tango service is connected successfully.
   *
   * @param nativeTangoServiceBinder The native binder object.
   */
  public static native void onTangoServiceConnected(IBinder nativeTangoServiceBinder);

  /**
   * Interfaces to native OnPause function.
   */
  public static native void onPause();

  // Allocate OpenGL resources for rendering.
  public static native void onGlSurfaceCreated();

  // Setup the view port width and height.
  public static native void onGlSurfaceChanged(int width, int height);

  // Main render loop.
  public static native void onGlSurfaceDrawFrame();

  // Called when the toggle button is clicked
  public static native void onToggleButtonClicked(boolean reconstructionRunning);

  // Called when the clear button is clicked
  public static native void onClearButtonClicked();

  // Set 3D scanning parameters
  public static native void set3D(double res, double min, double max);

  // Load 3D model from file
  public static native void load(String name);

  // Save current 3D model
  public static native void save(String name);

  // Set view on 3D view
  public static native void setView(float pitch, float yaw, float x, float y, boolean gyro);

  // Set zoom of view
  public static native void setZoom(float value);
}
