package com.lvonasek.openconstructor.ui;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;

import com.lvonasek.openconstructor.R;

public class Service extends android.app.Service
{
  private static Runnable action;
  private static Notification.Builder builder;
  private static AbstractActivity parent;
  private static NotificationManager nm;
  private static boolean isRunning = false;

  public static void finish(Runnable onFinish)
  {
    isRunning = false;

    //update notification that operation was finished
    FileManager.setOnResume(onFinish);
    builder.setContentText(parent.getString(R.string.finished));
    builder.setAutoCancel(true);
    builder.setOngoing(false);
    nm.notify(0, builder.build());
  }

  public static void process(AbstractActivity activity, String message, Runnable runnable)
  {
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

    action = runnable;
    parent = activity;
    isRunning = true;
    activity.startService(new Intent(activity, Service.class));
  }

  public static boolean isRunning()
  {
    return isRunning;
  }

  @Override
  public IBinder onBind(Intent intent) {
    return null;
  }

  @Override
  public void onCreate() {
    super.onCreate();

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
  public void onDestroy()
  {
    try {
      nm.cancel(0);
      stopService(new Intent(parent, Service.class));
    } catch(Exception e) {
      e.printStackTrace();
    }
    super.onDestroy();
  }
}
