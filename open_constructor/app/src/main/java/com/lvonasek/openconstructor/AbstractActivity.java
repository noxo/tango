package com.lvonasek.openconstructor;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.view.WindowManager;

import java.io.File;

public abstract class AbstractActivity extends Activity
{
  protected static final String FILE_EXT = ".ply";
  protected static final String FILE_KEY = "FILE2OPEN";
  protected static final String MODEL_DIRECTORY = "/Models/";
  protected static final int REQUEST_CODE_PERMISSION_CAMERA = 1987;
  protected static final int REQUEST_CODE_PERMISSION_READ_STORAGE = 1988;
  protected static final int REQUEST_CODE_PERMISSION_WRITE_STORAGE = 1989;
  protected static final int REQUEST_CODE_PERMISSION_INTERNET = 1990;

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  public Uri filename2Uri(String filename) {
    if(filename == null)
      return null;
    return Uri.fromFile(new File(getPath(), filename));
  }

  public String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    new File(dir).mkdir();
    return dir;
  }

  protected void setupPermission(String permission, int requestCode) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
        requestPermissions(new String[]{permission}, requestCode);
      else
        onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
    } else
      onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
  }

  public abstract void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults);
}
