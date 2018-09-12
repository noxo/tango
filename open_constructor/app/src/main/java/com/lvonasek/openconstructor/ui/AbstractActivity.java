package com.lvonasek.openconstructor.ui;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Environment;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.WindowManager;

import com.google.atap.tangoservice.Tango;
import com.google.atap.tangoservice.TangoConfig;
import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.main.TangoInitHelper;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Scanner;

public abstract class AbstractActivity extends Activity
{
  protected static final String FILE_KEY = "FILE2OPEN";
  protected static final String MODEL_DIRECTORY = "/Models/";
  protected static final String RESOLUTION_KEY = "RESOLUTION";
  protected static final String TEMP_DIRECTORY = "dataset";
  protected static final String URL_KEY = "URL2OPEN";
  protected static final String USER_AGENT = "Mozilla/5.0 Google";
  public static final String TAG = "tango_app";
  private Tango mTango = null;

  protected void defaultSettings()
  {
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(this).edit();
    e.putBoolean(getString(R.string.pref_cardboard), isCardboardEnabled(this));
    e.putBoolean(getString(R.string.pref_landscape), isLandscape(this));
    e.putBoolean(getString(R.string.pref_noisefilter), isNoiseFilterOn(this));
    e.putBoolean(getString(R.string.pref_poisson), isPoissonReconstructionOn(this));
    e.putBoolean(getString(R.string.pref_sharpphotos), isSharpPhotosOn(this));
    e.putBoolean(getString(R.string.pref_spaceclearing), isSpaceClearingOn(this));
    e.apply();
  }

  public static boolean isCardboardEnabled(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    String key = context.getString(R.string.pref_cardboard);
    return pref.getBoolean(key, false);
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
  public void cleanADF() {
    final ArrayList<String> uuids = new ArrayList<>();
    File file = new File("/data/data/com.lvonasek.openconstructor/files/todelete");
    if (file.exists()) {
      try {
        FileInputStream fis = new FileInputStream(file.getAbsolutePath());
        Scanner sc = new Scanner(fis);
        while (sc.hasNext()) {
          uuids.add(sc.nextLine());
        }
        sc.close();
        fis.close();
      } catch (Exception e) {
        e.printStackTrace();
      }
      file.delete();
    } else {
      return;
    }

    //export ADF
    if (mTango != null)
      mTango.disconnect();
    mTango = new Tango(AbstractActivity.this, new Runnable() {
      @Override
      public void run() {
        TangoInitHelper.bindTangoService(AbstractActivity.this, new ServiceConnection() {
          @Override
          public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
            for (String uuid : mTango.listAreaDescriptions()) {
              if (uuids.contains(uuid)) {
                mTango.deleteAreaDescription(uuid);
              }
            }
            mTango.disconnect();
          }

          @Override
          public void onServiceDisconnected(ComponentName componentName) {
          }
        });
        mTango.connect(mTango.getConfig(TangoConfig.CONFIG_TYPE_DEFAULT));
      }
    });
  }

  public void exportADF(final Runnable onFinish) {
    try {
      //get UUID
      FileInputStream fis = new FileInputStream(new File(getTempPath(), "uuid.txt").getAbsolutePath());
      Scanner sc = new Scanner(fis);
      final String uuid = sc.nextLine();
      sc.close();
      fis.close();

      //get ADF path
      final File adf = new File(getTempPath(), uuid);
      if (adf.exists())
        adf.delete();

      //export ADF
      if (mTango != null)
        mTango.disconnect();
      mTango = new Tango(AbstractActivity.this, new Runnable() {
        @Override
        public void run() {
          TangoInitHelper.bindTangoService(AbstractActivity.this, new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName componentName, IBinder iBinder) {
              mTango.exportAreaDescriptionFile(uuid, getTempPath().getAbsolutePath());
              while (!adf.exists()) {
                try {
                  Thread.sleep(50);
                } catch (Exception e) {
                  e.printStackTrace();
                }
              }
              mTango.disconnect();
              if (adf.exists())
                onFinish.run();
            }

            @Override
            public void onServiceDisconnected(ComponentName componentName) {
            }
          });
          mTango.connect(mTango.getConfig(TangoConfig.CONFIG_TYPE_DEFAULT));
        }
      });
    } catch (Exception e) {
      e.printStackTrace();
    }
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
