package com.lvonasek.openconstructor.main;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.util.Log;

import com.lvonasek.openconstructor.ui.AbstractActivity;

import java.util.ArrayList;

class TangoInitHelper
{
  static boolean bindTangoService(final Context context, ServiceConnection connection)
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

  static void loadLibrary(String pkg, String name, String libraryShort)
  {
    ArrayList<String> base = new ArrayList<>();
    base.add("/system/app/" + name + "/lib/");
    base.add("/data/data/" + pkg + "/libfiles/");
    for (String basePath : base)
    {
      String library = "lib" + libraryShort + ".so";
      ArrayList<String> lib = new ArrayList<>();
      lib.add(basePath + "arm64-v8a/" + library);
      lib.add(basePath + "armeabi/" + library);
      lib.add(basePath + "default/" + library);
      lib.add(basePath + "arm64/" + library);
      lib.add(basePath + "arm32/" + library);
      lib.add(basePath + "arm/" + library);
      for (String s : lib)
      {
        try {
          System.load(s);
          Log.d(AbstractActivity.TAG, s + " loaded");
          return;
        } catch (UnsatisfiedLinkError e) {
        }
      }
    }
    try {
      System.loadLibrary(libraryShort);
      Log.d(AbstractActivity.TAG, libraryShort + " loaded");
    } catch (UnsatisfiedLinkError e) {
    }
  }
}
