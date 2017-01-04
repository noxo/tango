package com.lvonasek.openconstructor;

import android.os.Environment;

import java.io.File;

public class FileUtils
{
  public static final String FILE_KEY = "FILE2OPEN";
  private static final String MODEL_DIRECTORY = "/Models/";

  public static String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    new File(dir).mkdir();
    return dir;
  }
}
