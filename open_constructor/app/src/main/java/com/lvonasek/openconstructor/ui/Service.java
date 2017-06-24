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
  private static Runnable actionFinish;
  private static boolean cancelable;
  private static int index = -1;
  private static String msg;
  private static AbstractActivity parent;
  private static NotificationManager nm;
  private static boolean isRunning = false;

  public static void process(AbstractActivity activity, String message, boolean cancel, Runnable runnable, Runnable onFinish)
  {
    if (isRunning)
      return;
    action = runnable;
    actionFinish = onFinish;
    cancelable = cancel;
    msg = message;
    parent = activity;
    isRunning = true;
    index++;
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

    //build notification
    Notification.Builder mBuilder = new Notification.Builder(this)
            .setSmallIcon(R.drawable.ic_launcher)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(msg)
            .setAutoCancel(cancelable)
            .setOngoing(!cancelable);

    //show notification
    Intent intent = new Intent(this, FileManager.class);
    if ((action == null) && (actionFinish != null))
      FileManager.setOnResume(actionFinish);
    PendingIntent pi = PendingIntent.getActivity(this, 0, intent, 0);
    mBuilder.setContentIntent(pi);
    nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    nm.notify(index, mBuilder.build());

    //process
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        if (action != null)
          action.run();
        isRunning = false;
        if (action != null)
        {
          nm.cancel(index);
          stopService(new Intent(parent, Service.class));

          //new notification that operation was finished
          new Thread(new Runnable()
          {
            @Override
            public void run()
            {
              try
              {
                Thread.sleep(1000);
              } catch (InterruptedException e)
              {
                e.printStackTrace();
              }
              process(parent, getString(R.string.finished), true, null, actionFinish);
            }
          }).start();
        }
      }
    }).start();
  }

  @Override
  public void onDestroy()
  {
    nm.cancel(index);
    stopService(new Intent(parent, Service.class));
    super.onDestroy();
  }
}
