package com.lvonasek.nightvision;

import android.os.IBinder;
import android.util.Log;

/**
 * Interfaces between native C++ code and Java code.
 */
public class JNI
{
  static {
    if (!TangoInitHelper.loadTangoSharedLibrary())
      Log.e("JNI", "ERROR! Unable to load libtango_client_api.so!");
    System.loadLibrary("openconstructor");
  }

  // Called when the Tango service is connected successfully.
  public static native void onTangoServiceConnected(IBinder binder, double res, double dmin, double dmax, int noise, boolean updown);

  // Setup the view port width and height.
  public static native void onGlSurfaceChanged(int width, int height);

  // Main render loop.
  public static native void onGlSurfaceDrawFrame();
}
