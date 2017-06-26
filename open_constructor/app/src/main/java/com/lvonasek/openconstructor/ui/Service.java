package com.lvonasek.openconstructor.ui;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.preference.PreferenceManager;

import com.lvonasek.openconstructor.R;

public class Service extends android.app.Service
{
  private static final String SERVICE_RUNNING = "service_running";

  public static final int SERVICE_FINISHED = -1;
  public static final int SERVICE_NOT_RUNNING = 0;
  public static final int SERVICE_SKETCHFAB = 1;
  public static final int SERVICE_POSTPROCESS = 2;

  private static Runnable action;
  private static Notification.Builder builder;
  private static AbstractActivity parent;
  private static Service service;
  private static NotificationManager nm;

  public static void finish(Intent onFinish)
  {
    //update notification that operation was finished
    PendingIntent pi = PendingIntent.getActivity(parent, 0, onFinish, 0);
    builder.setContentIntent(pi);
    builder.setContentText(parent.getString(R.string.finished));
    builder.setAutoCancel(true);
    builder.setOngoing(false);
    nm.notify(0, builder.build());

    //mark activity as inactive
    service.stopService(new Intent(parent, Service.class));
    AbstractActivity activity = parent;
    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(activity).edit();
    e.putInt(SERVICE_RUNNING, SERVICE_FINISHED);
    e.commit();

    System.exit(0);
  }

  public static void process(String message, int serviceId, AbstractActivity activity, Runnable runnable)
  {
    action = runnable;
    parent = activity;

    //build notification
    builder = new Notification.Builder(activity)
            .setSmallIcon(R.drawable.ic_launcher)
            .setContentTitle(activity.getString(R.string.app_name))
            .setContentText(message)
            .setAutoCancel(false)
            .setOngoing(true);

    //show notification
    Intent intent = new Intent(activity, FileManager.class);
    PendingIntent pi = PendingIntent.getActivity(activity, 0, intent, 0);
    builder.setContentIntent(pi);
    nm = (NotificationManager) activity.getSystemService(Context.NOTIFICATION_SERVICE);
    nm.notify(0, builder.build());

    SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(activity).edit();
    e.putInt(SERVICE_RUNNING, serviceId);
    e.commit();
    activity.startService(new Intent(activity, Service.class));
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
    e.commit();
  }

  @Override
  public void onCreate() {
    super.onCreate();
    service = this;

    //process
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
  public void onDestroy()
  {
    try {
      nm.cancel(0);
    } catch(Exception e) {
      e.printStackTrace();
    }
    super.onDestroy();
  }

  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }
}
