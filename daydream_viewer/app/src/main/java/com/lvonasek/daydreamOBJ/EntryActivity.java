package com.lvonasek.daydreamOBJ;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.widget.Toast;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

public class EntryActivity extends AbstractActivity {

  public static String filename = null;

  private static final int PERMISSIONS_CODE = 1985;

  @Override
  protected void onAddressChanged(String address)
  {

  }

  @Override
  protected void onConnectionChanged(boolean on)
  {

  }

  @Override
  protected void onDataReceived()
  {

  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    try
    {
      filename = getIntent().getData().toString().substring(7);
      try
      {
        filename = URLDecoder.decode(filename, "UTF-8");
      } catch (UnsupportedEncodingException e)
      {
        e.printStackTrace();
      }
    } catch(Exception e) {
      Toast.makeText(this, R.string.no_file, Toast.LENGTH_LONG).show();
      finish();
      return;
    }
  }

  @Override
  protected void onResume() {
    super.onResume();
    setupPermissions();
    finish();
  }

  protected void setupPermissions() {
    String[] permissions = {
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.NFC,
            Manifest.permission.READ_EXTERNAL_STORAGE
    };
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      boolean ok = true;
      for (String s : permissions)
        if (checkSelfPermission(s) != PackageManager.PERMISSION_GRANTED)
          ok = false;

      if (!ok)
        requestPermissions(permissions, PERMISSIONS_CODE);
      else
        onRequestPermissionsResult(PERMISSIONS_CODE, null, new int[]{PackageManager.PERMISSION_GRANTED});
    } else
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
          startActivity(new Intent(this, MainActivity.class));
        break;
      }
    }
  }

  @Override
  public void onNewIntent(Intent intent) {}
}
