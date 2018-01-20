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
  public static final String SERVICE_LINK = "service_link";
  public static final String SERVICE_RUNNING = "service_running";

  public static final int SERVICE_NOT_RUNNING = 0;
  public static final int SERVICE_SKETCHFAB = 1;
  public static final int SERVICE_POSTPROCESS = 2;
  public static final int SERVICE_SAVE = 3;

  private static Runnable action;
  private static String message;
  private static String messageNotification;
  private static AbstractActivity parent;
  private static boolean running;
  private static Service service;

  @Override
  public synchronized void onCreate() {
    super.onCreate();
    service = this;
    message = "";
    if (parent == null)
      return;
    if ((getRunning(parent) == SERVICE_POSTPROCESS) || (getRunning(parent) == SERVICE_SAVE)) {
      running = true;
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          while(running) {
            setMessage(JNI.getEvent(Service.this.getResources()));
            try
            {
              Thread.sleep(1000);
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

  public static synchronized void finish(String link)
  {
    running = false;
    service.stopService(new Intent(parent, Service.class));
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(parent).edit();
    e.putInt(SERVICE_RUNNING, -Math.abs(getRunning(parent)));
    e.putString(SERVICE_LINK, link);
    e.commit();
    System.exit(0);
  }

  public static synchronized void process(String message, int serviceId, AbstractActivity activity, Runnable runnable)
  {
    action = runnable;
    parent = activity;
    messageNotification = message;

    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(activity).edit();
    e.putInt(SERVICE_RUNNING, serviceId);
    e.putString(SERVICE_LINK, "");
    e.commit();
    activity.startService(new Intent(activity, Service.class));
  }

  public static synchronized Intent getIntent(AbstractActivity activity) {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(activity);
    String link = pref.getString(SERVICE_LINK, "");
    switch(Math.abs(getRunning(activity))) {
      case SERVICE_SKETCHFAB:
        Intent i = new Intent(activity, Home.class);
        i.putExtra(AbstractActivity.URL_KEY, link);
        return i;
      case SERVICE_POSTPROCESS:
      case SERVICE_SAVE:
        Intent intent = new Intent(activity, OpenConstructor.class);
        intent.putExtra(AbstractActivity.FILE_KEY, link);
        return intent;
      default:
        return new Intent(activity, FileManager.class);
    }
  }

  public static synchronized String getLink(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    return pref.getString(SERVICE_LINK, "");
  }

  public static synchronized String getMessage()
  {
    if (messageNotification == null)
      return null;
    if (message == null)
      return null;
    return messageNotification + "\n" + message;
  }

  public static synchronized int getRunning(Context context)
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
    return pref.getInt(SERVICE_RUNNING, SERVICE_NOT_RUNNING);
  }

  private static synchronized void setMessage(String msg)
  {
    message = msg;
  }

  public static synchronized void reset(Context context)
  {
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(context).edit();
    e.putInt(SERVICE_RUNNING, SERVICE_NOT_RUNNING);
    e.putString(SERVICE_LINK, "");
    e.commit();
  }
}
