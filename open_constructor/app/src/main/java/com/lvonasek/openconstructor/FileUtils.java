package com.lvonasek.openconstructor;

import android.net.Uri;
import android.os.Environment;
import java.io.File;

public class FileUtils
{
  public static final String FILE_EXT = ".ply";
  public static final String FILE_KEY = "FILE2OPEN";
  public static final String MODEL_DIRECTORY = "/Models/";
  public static final int REQUEST_CODE_PERMISSION_CAMERA = 1987;
  public static final int REQUEST_CODE_PERMISSION_READ_STORAGE = 1988;
  public static final int REQUEST_CODE_PERMISSION_WRITE_STORAGE = 1989;
  public static final int REQUEST_CODE_PERMISSION_INTERNET = 1990;


  public static Uri filename2Uri(String filename) {
    return Uri.fromFile(new File(getPath(), filename));
  }

  public static String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    new File(dir).mkdir();
    return dir;
  }
}
