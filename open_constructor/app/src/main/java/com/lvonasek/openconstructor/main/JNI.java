package com.lvonasek.openconstructor.main;

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

  // Save current 3D model with textures (editor usage)
  public static native void saveWithTextures(String name);

  // Texturize 3D model
  public static native void texturize(String name);

  // Set view on 3D view
  public static native void setView(float pitch, float yaw, float x, float y, float z, boolean gyro);

  // Detect floor level for position
  public static native float getFloorLevel(float x, float y, float z);

  // Get Tango event
  public static native byte[] getEvent();

  // Apply effect on model
  public static native void applyEffect(int effect, float value, int axis);

  // Preview effect on model
  public static native void previewEffect(int effect, float value, int axis);

  // Apply select on model
  public static native void applySelect(float x, float y, boolean triangle);

  // Select or deselect all
  public static native void completeSelection(boolean inverse);

  // Increase or decrease selection
  public static native void multSelection(boolean increase);

  // Select object by rect
  public static native void rectSelection(float x1, float y1, float x2, float y2);

  public static native byte[] clientSecret();
}
