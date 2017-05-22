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
  public static native void onTangoServiceConnected(IBinder nativeTangoServiceBinder, double res,
   double dmin, double dmax, int noise, boolean land, String temp);

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

  // Load 3D model from file
  public static native void load(String name);

  // Save current 3D model
  public static native void save(String name, String dataset);

  // Texturize 3D model
  public static native void texturize(String name, String dataset);

  // Set view on 3D view
  public static native void setView(float pitch, float yaw, float x, float y, float z, boolean gyro);

  // Detect floor level for position
  public static native float getFloorLevel(float x, float y, float z);

  // Get Tango event
  public static native byte[] getEvent();

  public static native byte[] clientSecret();
}
