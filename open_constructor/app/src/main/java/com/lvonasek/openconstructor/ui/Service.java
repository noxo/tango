package com.lvonasek.openconstructor.ui;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.preference.PreferenceManager;

import com.lvonasek.openconstructor.main.JNI;
import com.lvonasek.openconstructor.main.OpenConstructor;
import com.lvonasek.openconstructor.sketchfab.Home;

public class Service extends android.app.Service
{
  private static final String SERVICE_LINK = "service_link";
  private static final String SERVICE_RUNNING = "service_running";

  public static final int SERVICE_NOT_RUNNING = 0;
  public static final int SERVICE_SKETCHFAB = 1;
  public static final int SERVICE_POSTPROCESS = 2;

  private static Runnable action;
  private static String message;
  private static String messageNotification;
  private static AbstractActivity parent;
  private static boolean running;
  private static Service service;

  @Override
  public void onCreate() {
    super.onCreate();
    service = this;
    if (getRunning(parent) == SERVICE_POSTPROCESS) {
      message = "";
      running = true;
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          while(running) {
            message = new String(JNI.getEvent());
            try
            {
              Thread.sleep(100);
            } catch (Exception e)
            {
              e.printStackTrace();
            }
          }
          message = "";
        }
      }).start();
    }
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        action.run();
      }
    }).start();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId)
  {
    return START_STICKY;
  }

  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  public static void finish(String link)
  {
    running = false;
    service.stopService(new Intent(parent, Service.class));
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(parent).edit();
    e.putInt(SERVICE_RUNNING, -Math.abs(getRunning(parent)));
    e.putString(SERVICE_LINK, link);
    e.commit();
    Initializator.updateNotification(getIntent(parent));
    System.exit(0);
  }

  public static void process(String message, int serviceId, AbstractActivity activity, Runnable runnable)
  {
    action = runnable;
    parent = activity;
    messageNotification = message;
    Initializator.showNotification(message);

    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(activity).edit();
    e.putInt(SERVICE_RUNNING, serviceId);
    e.putString(SERVICE_LINK, "");
    e.commit();
    activity.startService(new Intent(activity, Service.class));
  }

  public static Intent getIntent(AbstractActivity activity) {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(parent);
    String link = pref.getString(SERVICE_LINK, "");
    switch(Math.abs(getRunning(activity))) {
      case SERVICE_SKETCHFAB:
        Intent i = new Intent(activity, Home.class);
        i.putExtra(AbstractActivity.URL_KEY, link);
        return i;
      case SERVICE_POSTPROCESS:
        Intent intent = new Intent(activity, OpenConstructor.class);
        intent.putExtra(AbstractActivity.FILE_KEY, link);
        return intent;
      default:
        return new Intent(activity, FileManager.class);
    }
  }

  public static String getMessage()
  {
    return messageNotification + "\n" + message;
  }

  public static int getRunning(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    return pref.getInt(SERVICE_RUNNING, SERVICE_NOT_RUNNING);
  }

  public static void reset(Context context)
  {
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(context).edit();
    e.putInt(SERVICE_RUNNING, SERVICE_NOT_RUNNING);
    e.putString(SERVICE_LINK, "");
    e.commit();
  }
}
