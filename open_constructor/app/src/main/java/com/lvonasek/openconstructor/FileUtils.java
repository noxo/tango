package com.lvonasek.openconstructor;

import android.os.Environment;

import java.io.File;

public class FileUtils
{
  private static final String MODEL_DIRECTORY = "/Models/";

  public static String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    new File(dir).mkdir();
    return dir;
  }
}
