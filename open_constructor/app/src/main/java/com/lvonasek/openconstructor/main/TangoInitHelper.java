package com.lvonasek.openconstructor.main;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.util.Log;
import java.io.File;

public class TangoInitHelper {

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
  public static final boolean loadTangoSharedLibrary() {
    String basePath = "/data/data/com.google.tango/libfiles/";
    if (!(new File(basePath).exists())) {
      basePath = "/data/data/com.projecttango.tango/libfiles/";
    }
    Log.i("TangoInitHelper", "basePath: " + basePath);

    boolean loaded = false;
    try {
      System.load(basePath + "default/libtango_client_api.so");
      loaded = true;
      Log.i("TangoInitHelper", "Success! Using default/libtango_client_api.");
    } catch (UnsatisfiedLinkError e) {
    }
    if (!loaded) {
      try {
        System.loadLibrary("tango_client_api");
        loaded = true;
        Log.i("TangoInitHelper", "Falling back to libtango_client_api.so symlink.");
      } catch (UnsatisfiedLinkError e) {
      }
    }
    return loaded;
  }
}