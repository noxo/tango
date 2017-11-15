package com.lvonasek.daydreamOBJ;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;

import com.google.vr.ndk.base.DaydreamApi;

import java.net.URLDecoder;

public class EntryActivity extends Activity {

  public static String[] permissions = { Manifest.permission.READ_EXTERNAL_STORAGE };

  public static String filename = null;
  private DaydreamApi daydream;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    daydream = DaydreamApi.create(this);
    try
    {
      filename = null;
      filename = getIntent().getData().toString().substring(7);
      filename = URLDecoder.decode(filename, "UTF-8");
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  @Override
  public void onBackPressed()
  {
    super.onBackPressed();
    System.exit(0);
  }

  @Override
  protected void onResume() {
    super.onResume();
    setupPermissions();
  }

  protected void setupPermissions() {
    boolean ok = true;
    for (String s : permissions)
      if (checkSelfPermission(s) != PackageManager.PERMISSION_GRANTED)
        ok = false;

    if (!ok)
    {
      try
      {
        daydream.exitFromVr(this, 1, null);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            try
            {
              Thread.sleep(5000);
            } catch (InterruptedException e)
            {
              e.printStackTrace();
            }
            runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                requestPermissions(permissions, 2);
              }
            });
          }
        }).start();
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
    else {
      if (filename == null)
      {
        SelectorView.active = true;
        filename = "";
      }
      daydream.create(this).launchInVr(new Intent(this, MainActivity.class));
      finish();
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
      recreate();
  }
}
