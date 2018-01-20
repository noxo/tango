package com.lvonasek.openconstructor.main;

import android.content.res.Resources;
import android.os.IBinder;
import android.util.Log;

import com.lvonasek.openconstructor.R;

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
  public static native void onTangoServiceConnected(IBinder binder, double res, double dmin,
                                                    double dmax, int noise, boolean land,
                                                    boolean sharp, String temp);

  // Setup the view port width and height.
  public static native void onGlSurfaceChanged(int width, int height);

  // Main render loop.
  public static native void onGlSurfaceDrawFrame();

  // Called when the toggle button is clicked
  public static native void onToggleButtonClicked(boolean reconstructionRunning);

  // Called when the clear button is clicked
  public static native void onClearButtonClicked();

  // Resume scanning
  public static native void onResumeScanning();

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
  private static native byte[] getEvent();

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

  // Indicate that last setview was applied
  public static native boolean animFinished();

  public static native byte[] clientSecret();

  public static String getEvent(Resources r)
  {
    String event = new String(getEvent());
    event = event.replace("TOO_BRIGHT", r.getString(R.string.event_bright));
    event = event.replace("TOO_DARK", r.getString(R.string.event_dark));
    event = event.replace("FEW_FEATURES", r.getString(R.string.event_features));
    event = event.replace("CONVERT", r.getString(R.string.event_convert));
    event = event.replace("IMAGE", r.getString(R.string.event_image));
    event = event.replace("MERGE", r.getString(R.string.event_merge));
    event = event.replace("PROCESS", r.getString(R.string.event_process));
    event = event.replace("SIMPLIFY", r.getString(R.string.event_simplify));
    event = event.replace("UNWRAP", r.getString(R.string.event_unwrap));
    return event;
  }
}
