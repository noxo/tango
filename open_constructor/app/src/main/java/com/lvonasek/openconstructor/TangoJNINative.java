package com.lvonasek.openconstructor;

import android.os.IBinder;
import android.util.Log;

import com.lvonasek.openconstructor.main.TangoInitHelper;

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

  // Called when the Tango service is connected successfully.
  public static native void onTangoServiceConnected(IBinder binder, double res,  double dmin,
                                                    double dmax, int noise, boolean land, String temp);

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
  public static native void save(String name);

  // Texturize 3D model
  public static native void texturize(String name);

  // Set view on 3D view
  public static native void setView(float pitch, float yaw, float x, float y, float z, boolean gyro);

  // Detect floor level for position
  public static native float getFloorLevel(float x, float y, float z);

  // Get Tango event
  public static native byte[] getEvent();

  // Apply effect on model
  public static native void applyEffect(int effect, float value);

  // Preview effect on model
  public static native void previewEffect(int effect, float value);

  // Apply select on model
  public static native void applySelect(float x, float y);

  // Select or deselect all
  public static native void completeSelection(boolean inverse);

  // Increase or decrease selection
  public static native void multSelection(boolean increase);

  public static native byte[] clientSecret();
}
