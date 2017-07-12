package com.lvonasek.openconstructor.main;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.util.Log;

import com.lvonasek.openconstructor.ui.AbstractActivity;

import java.util.ArrayList;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import java.io.File;

public class TangoInitHelper {
  public static final int ARCH_ERROR = -2;
  public static final int ARCH_FALLBACK = -1;
  public static final int ARCH_DEFAULT = 0;
  public static final int ARCH_ARM64 = 1;
  public static final int ARCH_ARM32 = 2;
  public static final int ARCH_X86_64 = 3;
  public static final int ARCH_X86 = 4;

  public static final boolean bindTangoService(final Context context, ServiceConnection connection)
  {
    Intent intent = new Intent();
    intent.setClassName("com.google.tango", "com.google.atap.tango.TangoService");
    boolean hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);
    if (!hasJavaService)
    {
      intent = new Intent();
      intent.setClassName("com.projecttango.tango", "com.google.atap.tango.TangoService");
      hasJavaService = (context.getPackageManager().resolveService(intent, 0) != null);
    }
    return hasJavaService && context.bindService(intent, connection, Context.BIND_AUTO_CREATE);

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
        System.load(basePath + "x86_64/libtango_client_api.so");
        loadedSoId = ARCH_X86_64;
        Log.i("TangoInitHelper", "Success! Using x86_64/libtango_client_api.");
      } catch (UnsatisfiedLinkError e) {
      }
    }
    if (loadedSoId < ARCH_DEFAULT) {
      try {
        System.load(basePath + "x86/libtango_client_api.so");
        loadedSoId = ARCH_X86;
        Log.i("TangoInitHelper", "Success! Using x86/libtango_client_api.");
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
}