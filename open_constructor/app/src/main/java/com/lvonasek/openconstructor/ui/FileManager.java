package com.lvonasek.openconstructor.ui;

import android.Manifest;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.main.Exporter;
import com.lvonasek.openconstructor.main.OpenConstructor;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

public class FileManager extends AbstractActivity implements View.OnClickListener {
  private ArrayList<Button> buttons;
  private ListView mList;
  private AdView mAdView;
  private Button mAdd;
  private Button mSettings;
  private ProgressBar mProgress;
  private TextView mText;
  private static final int PERMISSIONS_CODE = 1987;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);

    mAdd = (Button) findViewById(R.id.add_button);
    mList = (ListView) findViewById(R.id.list);
    mText = (TextView) findViewById(R.id.info_text);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    mSettings = (Button) findViewById(R.id.settings);
    mAdView = (AdView)findViewById(R.id.adMob);

    buttons = new ArrayList<>();
    buttons.add((Button) findViewById(R.id.service_continue));
    buttons.add((Button) findViewById(R.id.service_show_result));
    buttons.add((Button) findViewById(R.id.service_finish));
    buttons.add((Button) findViewById(R.id.service_cancel));
    for (Button b : buttons)
      b.setOnClickListener(this);
    mAdd.setOnClickListener(this);
    mSettings.setOnClickListener(this);
  }

  @Override
  public void onBackPressed()
  {
    moveTaskToBack(true);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mAdd.setVisibility(View.VISIBLE);
    mSettings.setVisibility(View.VISIBLE);
    showAd(false);
    for (Button b : buttons)
      b.setVisibility(View.GONE);
    mProgress.setVisibility(View.GONE);
    int service = Service.getRunning(this);
    if (service > Service.SERVICE_NOT_RUNNING) {
      for (Button b : buttons)
        if (b.getId() == R.id.service_cancel)
          b.setVisibility(View.VISIBLE);
      mAdd.setVisibility(View.GONE);
      mSettings.setVisibility(View.GONE);
      showAd(true);
      mList.setVisibility(View.GONE);
      mText.setVisibility(View.VISIBLE);
      mText.setText("");
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          while(true) {
            try
            {
              Thread.sleep(1000);
            } catch (Exception e)
            {
              e.printStackTrace();
            }
            FileManager.this.runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                if (Service.getMessage() == null)
                  mText.setText(getString(R.string.failed));
                else
                  mText.setText(getString(R.string.working) + "\n\n" + Service.getMessage());
              }
            });
          }
        }
      }).start();
    } else if (Service.getRunning(this) < Service.SERVICE_NOT_RUNNING) {
      mAdd.setVisibility(View.GONE);
      mSettings.setVisibility(View.GONE);
      showAd(true);
      mList.setVisibility(View.GONE);
      for (Button b : buttons)
        b.setVisibility(View.VISIBLE);
      mText.setVisibility(View.VISIBLE);
      boolean paused = Math.abs(Service.getRunning(this)) == Service.SERVICE_SAVE;
      int text = paused ? R.string.paused : R.string.finished;
      mText.setText(getString(text) + "\n" + getString(R.string.turn_off));
      service = Math.abs(Service.getRunning(this));
      if (service == Service.SERVICE_SKETCHFAB)
        findViewById(R.id.service_continue).setVisibility(View.GONE);
      else if (service == Service.SERVICE_POSTPROCESS)
        finishScanning();
    } else {
      setupPermissions();
    }
  }

  public void refreshUI()
  {
    final FileAdapter adapter = new FileAdapter(this);
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        //update file structure
        Exporter.makeStructure(getPath());
        //get list of files
        adapter.clearItems();
        String[] files = new File(getPath()).list();
        Arrays.sort(files);
        for(String s : files)
          if(new File(getPath(), s).isDirectory())
            if(Exporter.getModelType(s) >= 0)
              adapter.addItem(s);

        runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            mText.setVisibility(adapter.getCount() == 0 ? View.VISIBLE : View.GONE);
            mList.setAdapter(adapter);
            mAdd.setVisibility(View.VISIBLE);
            mSettings.setVisibility(View.VISIBLE);
            mProgress.setVisibility(View.GONE);
          }
        });
      }
    }).start();
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
    mAdd.setVisibility(View.GONE);
    mSettings.setVisibility(View.GONE);
    mProgress.setVisibility(View.VISIBLE);
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case PERMISSIONS_CODE:
      {
        for (int r : grantResults)
        {
          if (r != PackageManager.PERMISSION_GRANTED)
          {
            finish();
            return;
          }
        }
        refreshUI();
        break;
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    int service = Service.getRunning(this);
    Intent intent = new Intent(FileManager.this, OpenConstructor.class);
    switch (v.getId()) {
      case R.id.add_button:
        startScanning();
        break;
      case R.id.settings:
        startActivity(new Intent(this, Settings.class));
        break;
      case R.id.service_continue:
        showProgress();
        intent.putExtra(AbstractActivity.RESOLUTION_KEY, Integer.MIN_VALUE);
        startActivity(intent);
        break;
      case R.id.service_show_result:
        showProgress();
        startActivity(Service.getIntent(this));
        break;
      case R.id.service_finish:
        if (Math.abs(service) == Service.SERVICE_SKETCHFAB)
          finishOperation();
        else if (Math.abs(service) == Service.SERVICE_SAVE)
        {
          showProgress();
          intent.putExtra(AbstractActivity.RESOLUTION_KEY, Integer.MAX_VALUE);
          startActivity(intent);
        }
        break;
      case R.id.service_cancel:
        if ((service > Service.SERVICE_NOT_RUNNING) && (service != Service.SERVICE_SKETCHFAB)) {
          AlertDialog.Builder builder = new AlertDialog.Builder(this);
          builder.setTitle(getString(R.string.warning));
          builder.setMessage(getString(R.string.warning_reset));
          builder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialogInterface, int i)
            {
              Service.reset(FileManager.this);
              System.exit(0);
            }
          });
          builder.setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialogInterface, int i)
            {
            }
          });
          builder.show();
        } else {
          Service.reset(this);
          System.exit(0);
        }
        break;
    }
  }

  private void startScanning()
  {
    String[] resolutions = getResources().getStringArray(R.array.resolutions);
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(getString(R.string.scene_res));
    builder.setItems(resolutions, new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        showProgress();
        Intent intent = new Intent(FileManager.this, OpenConstructor.class);
        intent.putExtra(AbstractActivity.RESOLUTION_KEY, which);
        startActivity(intent);
      }
    });
    builder.create().show();
  }


  private void finishScanning()
  {
    for (Button b : buttons)
      b.setVisibility(View.GONE);
    showProgress();
    Date date = new Date() ;
    SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd_HHmmss");
    final String filename = dateFormat.format(date);
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        //delete old files during overwrite
        try
        {
          File file = new File(getPath(), filename + Exporter.FILE_EXT[0]);
          if (file.exists())
            for (String s : Exporter.getObjResources(file))
              if (new File(getPath(), s).delete())
                Log.d(AbstractActivity.TAG, "File " + s + " deleted");
        } catch (Exception e)
        {
          e.printStackTrace();
        }

        //move file from temp into folder
        File obj = new File(getPath(), Service.getLink(FileManager.this));
        for (String s : Exporter.getObjResources(obj.getAbsoluteFile()))
          if (new File(getTempPath(), s).renameTo(new File(getPath(), s)))
            Log.d(AbstractActivity.TAG, "File " + s + " saved");
        File file2save = new File(getPath(), filename + Exporter.FILE_EXT[0]);
        if (obj.renameTo(file2save))
          Log.d(TAG, "Obj file " + file2save.toString() + " saved.");

        //finish
        deleteRecursive(getTempPath());
        Service.reset(FileManager.this);
        Intent intent = new Intent(FileManager.this, OpenConstructor.class);
        intent.putExtra(AbstractActivity.FILE_KEY, file2save.getName());
        showProgress();
        startActivity(intent);
      }
    }).start();
  }

  private void finishOperation()
  {
    showProgress();
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        deleteRecursive(getTempPath());
        Service.reset(FileManager.this);
        finish();
      }
    }).start();
  }

  private void restartPostprocessing()
  {
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        File obj = null;
        long max = 0;
        File dir = getTempPath();
        for (File f : dir.listFiles())
        {
          if (max < f.length())
          {
            max = f.length();
            obj = f;
          }
        }
        SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(getBaseContext()).edit();
        e.putInt(Service.SERVICE_RUNNING, Service.SERVICE_NOT_RUNNING);
        e.putString(Service.SERVICE_LINK, TEMP_DIRECTORY + "/" + obj.getName());
        e.commit();
        Intent intent = new Intent(FileManager.this, OpenConstructor.class);
        intent.putExtra(AbstractActivity.RESOLUTION_KEY, Integer.MAX_VALUE);
        startActivity(intent);
      }
    }).start();
  }

  private void showAd(boolean show)
  {
    if (show)
    {

      try {
        mAdView.setVisibility(View.VISIBLE);
        mAdView.loadAd(new AdRequest.Builder().build());
      } catch (Exception e)
      {
        e.printStackTrace();
      }
    }
    else
      mAdView.setVisibility(View.GONE);
  }
}
