package com.lvonasek.openconstructor;

import android.app.Activity;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
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
  protected static final String PREF_ORIENTATION = "PREF_ORIENTATION";
  protected static final String PREF_TAG = "OPENCONSTRUCTOR";
  protected static final int REQUEST_CODE_PERMISSION_CAMERA = 1987;
  protected static final int REQUEST_CODE_PERMISSION_READ_STORAGE = 1988;
  protected static final int REQUEST_CODE_PERMISSION_WRITE_STORAGE = 1989;
  protected static final int REQUEST_CODE_PERMISSION_INTERNET = 1990;

  public boolean isPortrait() {
    SharedPreferences pref = getSharedPreferences(PREF_TAG, MODE_PRIVATE);
    int value = pref.getInt(PREF_ORIENTATION, ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    return value == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
  }

  public void setOrientation(boolean portrait) {
    SharedPreferences.Editor edit = getSharedPreferences(PREF_TAG, MODE_PRIVATE).edit();
    int value = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    if (!portrait)
      value = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    edit.putInt(PREF_ORIENTATION, value);
    edit.apply();
    setRequestedOrientation(value);
  }

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    setOrientation(isPortrait());
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
