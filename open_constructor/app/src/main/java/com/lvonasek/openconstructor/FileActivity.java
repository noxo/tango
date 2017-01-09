package com.lvonasek.openconstructor;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.util.Arrays;

public class FileActivity extends AbstractActivity implements View.OnClickListener {
  private FileAdapter mAdapter;
  private ListView mList;
  private LinearLayout mLayout;
  private ProgressBar mProgress;
  private TextView mText;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);

    mLayout = (LinearLayout) findViewById(R.id.layout_menu_action);
    mList = (ListView) findViewById(R.id.list);
    mText = (TextView) findViewById(R.id.no_data);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    findViewById(R.id.add_button).setOnClickListener(this);
    findViewById(R.id.sketchfab).setOnClickListener(this);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mLayout.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
    setupPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, REQUEST_CODE_PERMISSION_WRITE_STORAGE);
  }

  public void refreshList()
  {
    mAdapter = new FileAdapter(this);
    mAdapter.clearItems();
    String[] files = new File(getPath()).list();
    Arrays.sort(files);
    for(String s : files)
      if(s.substring(s.length() - FILE_EXT.length()).contains(FILE_EXT))
        mAdapter.addItem(s);
    mText.setVisibility(mAdapter.getCount() == 0 ? View.VISIBLE : View.GONE);
    mList.setAdapter(mAdapter);
    mLayout.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
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
      case REQUEST_CODE_PERMISSION_READ_STORAGE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
          refreshList();
        else
          finish();
        break;
      }
      case REQUEST_CODE_PERMISSION_WRITE_STORAGE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
          setupPermission(Manifest.permission.READ_EXTERNAL_STORAGE, REQUEST_CODE_PERMISSION_READ_STORAGE);
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
        showProgress();
        startActivity(new Intent(this, OpenConstructorActivity.class));
        break;
      case R.id.sketchfab:
        showProgress();
        startActivity(new Intent(this, SketchfabActivity.class));
        break;
    }
  }
}
