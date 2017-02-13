package com.lvonasek.openconstructor;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.util.Arrays;

public class FileActivity extends AbstractActivity implements View.OnClickListener {
  private ListView mList;
  private LinearLayout mLayout;
  private ProgressBar mProgress;
  private TextView mText;

  private static final int PERMISSIONS_CODE = 1987;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);

    mLayout = (LinearLayout) findViewById(R.id.layout_menu_action);
    mList = (ListView) findViewById(R.id.list);
    mText = (TextView) findViewById(R.id.no_data);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    findViewById(R.id.settings).setOnClickListener(this);
    findViewById(R.id.add_button).setOnClickListener(this);
    findViewById(R.id.sketchfab).setOnClickListener(this);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mLayout.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
    setupPermissions();
  }

  public void refreshUI()
  {
    FileAdapter adapter = new FileAdapter(this);
    adapter.clearItems();
    String[] files = new File(getPath()).list();
    Arrays.sort(files);
    for(String s : files)
      if(getModelType(s) >= 0)
        adapter.addItem(s);
    mText.setVisibility(adapter.getCount() == 0 ? View.VISIBLE : View.GONE);
    mList.setAdapter(adapter);
    mLayout.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
  }

  protected void setupPermissions() {
    String[] permissions = {
            Manifest.permission.CAMERA,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
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

  public void showProgress()
  {
    mLayout.setVisibility(View.GONE);
    mProgress.setVisibility(View.VISIBLE);
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case PERMISSIONS_CODE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
          refreshUI();
        else
          finish();
        break;
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId()) {
      case R.id.add_button:
        String[] resolutions = getResources().getStringArray(R.array.resolutions);
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getString(R.string.scene_res));
        builder.setItems(resolutions, new DialogInterface.OnClickListener()
                {
                  @Override
                  public void onClick(DialogInterface dialog, int which)
                  {
                    showProgress();
                    Intent intent = new Intent(FileActivity.this, OpenConstructorActivity.class);
                    intent.putExtra(AbstractActivity.RESOLUTION_KEY, which);
                    startActivity(intent);
                  }
                });
        builder.create().show();
        break;
      case R.id.settings:
        startActivity(new Intent(this, SettingsActivity.class));
        break;
      case R.id.sketchfab:
        showProgress();
        startActivity(new Intent(this, SketchfabActivity.class));
        break;
    }
  }
}
