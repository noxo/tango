package com.lvonasek.openconstructor;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.util.Arrays;

public class FileActivity extends ListActivity
{
  FileAdapter mAdapter;
  ImageButton mButton;
  ProgressBar mProgress;
  TextView mText;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);
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
    refreshList();
    mButton.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
  }

  public void refreshList()
  {
    mAdapter = new FileAdapter(this);
    mAdapter.clearItems();
    String[] files = new File(FileUtils.getPath()).list();
    Arrays.sort(files);
    for(String s : files)
      if(s.substring(s.length() - 4).contains(".ply"))
        mAdapter.addItem(s);
    mText.setVisibility(mAdapter.getCount() == 0 ? View.VISIBLE : View.GONE);
    getListView().setAdapter(mAdapter);
  }

  public void showProgress()
  {
    mButton.setVisibility(View.GONE);
    mProgress.setVisibility(View.VISIBLE);
  }
}
