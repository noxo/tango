package com.lvonasek.openconstructor.ui;

import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;

public class Initializator extends Activity
{
  private boolean closeOnResume = false;

  @Override
  protected void onPause()
  {
    super.onPause();
    closeOnResume = true;
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (Service.getRunning(this) == Service.SERVICE_FINISHED) {
      Service.reset(this);
      NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
      try {
        nm.cancel(0);
      } catch(Exception e) {
        e.printStackTrace();
      }
      startActivity(new Intent(this, FileManager.class));
    } else if (closeOnResume)
      finish();
    else
      startActivity(new Intent(this, FileManager.class));
  }
}
