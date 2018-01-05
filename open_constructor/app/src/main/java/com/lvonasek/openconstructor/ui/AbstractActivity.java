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
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Scanner;

public abstract class AbstractActivity extends Activity
{
  protected static final String FILE_KEY = "FILE2OPEN";
  protected static final String MODEL_DIRECTORY = "/Models/";
  protected static final String RESOLUTION_KEY = "RESOLUTION";
  protected static final String TEMP_DIRECTORY = "dataset";
  protected static final String URL_KEY = "URL2OPEN";
  protected static final String USER_AGENT = "Mozilla/5.0 Google";
  public static final String[] FILE_EXT = {".obj"};
  public static final String TAG = "tango_app";

  public static boolean isCardboardEnabled(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_cardboard);
    return pref.getBoolean(key, false);
  }

  public static boolean isPortrait(Context context) {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_landscape);
    return !pref.getBoolean(key, false);
  }

  public boolean isNoiseFilterOn()
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
    String key = getString(R.string.pref_noisefilter);
    return pref.getBoolean(key, false);
  }

  public boolean isSharpPhotosOn()
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
    String key = getString(R.string.pref_sharpphotos);
    return pref.getBoolean(key, false);
  }

  public static int getModelType(String filename) {
    for(int i = 0; i < FILE_EXT.length; i++) {
      int begin = filename.length() - FILE_EXT[i].length();
      if (begin >= 0)
        if (filename.substring(begin).contains(FILE_EXT[i]))
          return i;
    }
    return -1;
  }

  public static String getMtlResource(String obj)
  {
    try
    {
      Scanner sc = new Scanner(new FileInputStream(obj));
      while(sc.hasNext()) {
        String line = sc.nextLine();
        if (line.startsWith("mtllib")) {
          return line.substring(7);
        }
      }
      sc.close();
    } catch (Exception e)
    {
      e.printStackTrace();
    }
    return null;
  }

  public static ArrayList<String> getObjResources(File file)
  {
    HashSet<String> files = new HashSet<>();
    ArrayList<String> output = new ArrayList<>();
    String mtlLib = getMtlResource(file.getAbsolutePath());
    if (mtlLib != null) {
      output.add(mtlLib);
      output.add(mtlLib + ".png");
      mtlLib = file.getParent() + "/" + mtlLib;
      try
      {
        Scanner sc = new Scanner(new FileInputStream(mtlLib));
        while(sc.hasNext()) {
          String line = sc.nextLine();
          if (line.startsWith("map_Kd")) {
            String filename = line.substring(7);
            if (!files.contains(filename))
            {
              files.add(filename);
              output.add(filename);
            }
          }
        }
        sc.close();
      } catch (Exception e)
      {
        e.printStackTrace();
      }
    }
    return output;
  }

  public static void setOrientation(boolean portrait, Activity activity) {
    int value = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    if (!portrait)
      value = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    activity.setRequestedOrientation(value);
  }

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    setOrientation(isPortrait(this), this);
  }

  public void deleteRecursive(File fileOrDirectory) {
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
