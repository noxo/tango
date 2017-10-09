package com.lvonasek.daydreamOBJ;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import java.net.URLDecoder;

public class EntryActivity extends Activity {

  public static String filename = null;
  private static final int PERMISSIONS_CODE = 1985;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
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
  protected void onResume() {
    super.onResume();
    setupPermissions();
  }

  protected void setupPermissions() {
    String[] permissions = { Manifest.permission.READ_EXTERNAL_STORAGE };
    boolean ok = true;
    for (String s : permissions)
      if (checkSelfPermission(s) != PackageManager.PERMISSION_GRANTED)
        ok = false;

    if (!ok)
      requestPermissions(permissions, PERMISSIONS_CODE);
    else
      onRequestPermissionsResult(PERMISSIONS_CODE, null, new int[]{PackageManager.PERMISSION_GRANTED});
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case PERMISSIONS_CODE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
        {
          if (filename == null)
          {
            SelectorView.active = true;
            filename = "";
          }
          startActivity(new Intent(this, MainActivity.class));
        }
        finish();
        break;
      }
    }
  }
}
