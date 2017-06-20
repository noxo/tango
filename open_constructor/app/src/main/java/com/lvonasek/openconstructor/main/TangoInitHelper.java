package com.lvonasek.openconstructor.main;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.util.Log;

import java.io.File;

/**
 * Functions for simplifying the process of initializing TangoService, and function
 * handles loading correct libtango_client_api.so.
 */
public class TangoInitHelper
{
  private static final int ARCH_ERROR = -2;
  private static final int ARCH_FALLBACK = -1;
  private static final int ARCH_DEFAULT = 0;
  private static final int ARCH_ARM64 = 1;
  private static final int ARCH_ARM32 = 2;

  /**
   * Only for apps using the C API:
   * Initializes the underlying TangoService for native apps.
   *
   * @return returns false if the device doesn't have the Tango running as Android Service.
   *     Otherwise ture.
   */
  public static final boolean bindTangoService(final Context context,
                                               ServiceConnection connection) {
    Intent intent = new Intent();
    intent.setClassName("com.google.tango", "com.google.atap.tango.TangoService");

    boolean hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);

    // User doesn't have the latest packagename for TangoCore, fallback to the previous name.
    if (!hasJavaService) {
      intent = new Intent();
      intent.setClassName("com.projecttango.tango", "com.google.atap.tango.TangoService");
      hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);
    }

    // User doesn't have a Java-fied TangoCore at all; fallback to the deprecated approach
    // of doing nothing and letting the native side auto-init to the system-service version
    // of Tango.
    if (!hasJavaService) {
      return false;
    }

    return context.bindService(intent, connection, Context.BIND_AUTO_CREATE);
  }

    
  /**
   * Load the libtango_client_api.so library based on different Tango device setup.
   *
   * @return returns the loaded architecture id.
   */
  public static final int loadTangoSharedLibrary() {
    int loadedSoId = ARCH_ERROR;
    String basePath = "/data/data/com.google.tango/libfiles/";
    if (!(new File(basePath).exists())) {
      basePath = "/data/data/com.projecttango.tango/libfiles/";
    }
    Log.i("TangoInitHelper", "basePath: " + basePath);

    try {
      System.load(basePath + "arm64-v8a/libtango_client_api.so");
      loadedSoId = ARCH_ARM64;
      Log.i("TangoInitHelper", "Success! Using arm64-v8a/libtango_client_api.");
    } catch (UnsatisfiedLinkError e) {
    }
    if (loadedSoId < ARCH_DEFAULT) {
      try {
        System.load(basePath + "armeabi-v7a/libtango_client_api.so");
        loadedSoId = ARCH_ARM32;
        Log.i("TangoInitHelper", "Success! Using armeabi-v7a/libtango_client_api.");
      } catch (UnsatisfiedLinkError e) {
      }
    }
    if (loadedSoId < ARCH_DEFAULT) {
      try {
        System.load(basePath + "default/libtango_client_api.so");
        loadedSoId = ARCH_DEFAULT;
        Log.i("TangoInitHelper", "Success! Using default/libtango_client_api.");
      } catch (UnsatisfiedLinkError e) {
      }
    }
    if (loadedSoId < ARCH_DEFAULT) {
      try {
        System.loadLibrary("tango_client_api");
        loadedSoId = ARCH_FALLBACK;
        Log.i("TangoInitHelper", "Falling back to libtango_client_api.so symlink.");
      } catch (UnsatisfiedLinkError e) {
      }
    }
    return loadedSoId;
  }

  public static void loadLibrary(String libraryShort, int arch)
  {
    String basePath = "/data/data/com.lvonasek.openconstructor/libfiles/";
    String library = "lib" + libraryShort + ".so";
    if (arch == ARCH_ARM64)
      System.load(basePath + "arm64-v8a/" + library);
    else if (arch == ARCH_ARM32)
      System.load(basePath + "armeabi/" + library);
    else
      System.loadLibrary(libraryShort);
  }
}
