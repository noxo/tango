package com.lvonasek.openconstructor.ui;

import android.content.Intent;

public class Initializator extends AbstractActivity
{
  private static boolean closeOnResume = false;

  @Override
  protected void onResume()
  {
    super.onResume();
    if (closeOnResume) {
      closeOnResume = false;
      if (Service.getRunning(this) > Service.SERVICE_NOT_RUNNING) {
        moveTaskToBack(true);
      } else
        finish();
    } else {
      closeOnResume = true;
      Intent intent = new Intent(this, FileManager.class);
      intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
      startActivity(intent);
    }
  }

  public static void letMeGo()
  {
    closeOnResume = true;
  }
}
