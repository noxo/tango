package com.lvonasek.openconstructor;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.util.Arrays;

public class FileActivity extends AbstractActivity
{
  FileAdapter mAdapter;
  ImageButton mButton;
  ListView mList;
  ProgressBar mProgress;
  TextView mText;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);

    mList = (ListView) findViewById(R.id.list);
    mText = (TextView) findViewById(R.id.no_data);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    mButton = (ImageButton) findViewById(R.id.add_button);
    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        showProgress();
        startActivity(new Intent(FileActivity.this, OpenConstructorActivity.class));
      }
    });
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mButton.setVisibility(View.VISIBLE);
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
    mButton.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
  }

  public void showProgress()
  {
    mButton.setVisibility(View.GONE);
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
}
