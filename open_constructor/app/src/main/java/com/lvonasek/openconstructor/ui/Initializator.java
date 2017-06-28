package com.lvonasek.openconstructor.ui;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import com.lvonasek.openconstructor.R;

public class Initializator extends AbstractActivity
{
  private static boolean closeOnResume = false;
  private static int index = -1;
  private static Notification.Builder builder;
  private static Initializator instance;
  private static NotificationManager nm;

  @Override
  protected void onResume()
  {
    super.onResume();
    instance = this;
    nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

    if (closeOnResume) {
      closeOnResume = false;
      if (Service.getRunning(this) > Service.SERVICE_NOT_RUNNING) {
        moveTaskToBack(true);
      } else
        finish();
    } else {
      closeOnResume = true;
      if (Service.getRunning(this) < Service.SERVICE_NOT_RUNNING)
        hideNotification();
      startActivity(new Intent(this, FileManager.class));
    }
  }

  @Override
  public void onDestroy()
  {
    Initializator.hideNotification();
    super.onDestroy();
  }

  public static void letMeGo()
  {
    closeOnResume = true;
  }

  public static void hideNotification()
  {
    try {
      nm.cancel(index);
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  public static void showNotification(String message)
  {
    try {
      //build notification
      builder = new Notification.Builder(instance)
              .setSmallIcon(R.drawable.ic_launcher)
              .setContentTitle(instance.getString(R.string.app_name))
              .setContentText(message)
              .setAutoCancel(false)
              .setOngoing(true);
      index++;

      //show notification
      Intent intent = new Intent(instance, FileManager.class);
      PendingIntent pi = PendingIntent.getActivity(instance, 0, intent, 0);
      builder.setContentIntent(pi);
      nm.notify(index, builder.build());
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  public static void updateNotification()
  {
    try {
      builder.setContentText(instance.getString(R.string.finished));
      builder.setAutoCancel(true);
      builder.setOngoing(false);
      nm.notify(index, builder.build());
    } catch(Exception e) {
      e.printStackTrace();
    }
  }
}
