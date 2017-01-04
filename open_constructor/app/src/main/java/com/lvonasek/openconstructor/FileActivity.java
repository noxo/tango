package com.lvonasek.openconstructor;

import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ProgressBar;

import java.io.File;
import java.util.Arrays;

public class FileActivity extends ListActivity
{
  FileAdapter mAdapter;
  ImageButton mButton;
  ProgressBar mProgress;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    mButton = (ImageButton) findViewById(R.id.add_button);
    mButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        mButton.setVisibility(View.GONE);
        mProgress.setVisibility(View.VISIBLE);
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

  private void refreshList()
  {
    mAdapter = new FileAdapter(this);
    mAdapter.clearItems();
    String[] files = new File(FileUtils.getPath()).list();
    Arrays.sort(files);
    for(String s : files)
      if(s.substring(s.length() - 4).contains(".ply"))
        mAdapter.addItem(s);
    getListView().setAdapter(mAdapter);
  }
}
