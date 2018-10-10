package com.lvonasek.openconstructor.ui;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.WindowManager;

import com.lvonasek.openconstructor.R;

import java.io.File;

public abstract class AbstractActivity extends Activity
{
  protected static final String FILE_KEY = "FILE2OPEN";
  protected static final String MODEL_DIRECTORY = "/Models/";
  protected static final String RESOLUTION_KEY = "RESOLUTION";
  protected static final String TEMP_DIRECTORY = "dataset";
  protected static final String URL_KEY = "URL2OPEN";
  protected static final String USER_AGENT = "Mozilla/5.0 Google";
  public static final String TAG = "tango_app";

  protected void defaultSettings()
  {
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(this).edit();
    e.putBoolean(getString(R.string.pref_correction), isPoseCorrectionOn(this));
    e.putBoolean(getString(R.string.pref_landscape), isLandscape(this));
    e.putBoolean(getString(R.string.pref_noisefilter), isNoiseFilterOn(this));
    e.putBoolean(getString(R.string.pref_poisson), isPoissonReconstructionOn(this));
    e.putBoolean(getString(R.string.pref_sharpphotos), isSharpPhotosOn(this));
    e.putBoolean(getString(R.string.pref_spaceclearing), isSpaceClearingOn(this));
    e.apply();
  }

  public static boolean isLandscape(Context context) {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_landscape);
    return pref.getBoolean(key, false);
  }

  public static boolean isNoiseFilterOn(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_noisefilter);
    return pref.getBoolean(key, false);
  }

  public static boolean isPoseCorrectionOn(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_correction);
    return pref.getBoolean(key, false);
  }

  public static boolean isPoissonReconstructionOn(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_poisson);
    return pref.getBoolean(key, false);
  }

  public static boolean isSharpPhotosOn(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_sharpphotos);
    return pref.getBoolean(key, false);
  }
  public static boolean isSpaceClearingOn(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_spaceclearing);
    return pref.getBoolean(key, true);
  }

  public static void setOrientation(boolean land, Activity activity) {
    int value = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    if (!land)
      value = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    activity.setRequestedOrientation(value);
  }

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    setOrientation(isLandscape(this), this);
  }

  public static void deleteRecursive(File fileOrDirectory) {
    if (fileOrDirectory.isDirectory())
      for (File child : fileOrDirectory.listFiles())
        deleteRecursive(child);

    if (fileOrDirectory.delete())
      Log.d(TAG, fileOrDirectory + " deleted");
  }

  public Uri filename2Uri(String filename) {
    if(filename == null)
      return null;
    return Uri.fromFile(new File(getPath(), filename));
  }

  public static File getModel(String name) {
    //get the model file
    File obj = new File(getPath(), name);
    for (File f : obj.listFiles())
      if (f.getAbsolutePath().toLowerCase().endsWith(".obj"))
        return f;
    return obj;
  }

  public static String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    if (new File(dir).mkdir())
      Log.d(TAG, "Directory " + dir + " created");
    try
    {
      if (new File(new File(dir), ".nomedia").createNewFile())
        Log.d(TAG, ".nomedia in  " + dir + " created");
    } catch (Exception e)
    {
      e.printStackTrace();
    }
    return dir;
  }

  public static File getTempPath() {
    File dir = new File(getPath(), TEMP_DIRECTORY);
    if (dir.mkdir())
      Log.d(TAG, "Directory " + dir + " created");
    return dir;
  }
}
