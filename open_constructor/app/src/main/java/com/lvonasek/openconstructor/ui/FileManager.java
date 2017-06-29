package com.lvonasek.openconstructor.ui;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.main.OpenConstructor;
import com.lvonasek.openconstructor.sketchfab.Home;

import java.io.File;
import java.util.Arrays;

public class FileManager extends AbstractActivity implements View.OnClickListener {
  private ListView mList;
  private LinearLayout mLayout;
  private LinearLayout mOperations;
  private ProgressBar mProgress;
  private TextView mText;
  private boolean first = true;

  private static final String EXTRA_KEY_PERMISSIONTYPE = "PERMISSIONTYPE";
  private static final String EXTRA_VALUE_ADF = "ADF_LOAD_SAVE_PERMISSION";
  private static final int PERMISSIONS_CODE = 1987;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_files);

    mLayout = (LinearLayout) findViewById(R.id.layout_menu_action);
    mOperations = (LinearLayout) findViewById(R.id.layout_service_action);
    mList = (ListView) findViewById(R.id.list);
    mText = (TextView) findViewById(R.id.info_text);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    findViewById(R.id.settings).setOnClickListener(this);
    findViewById(R.id.add_button).setOnClickListener(this);
    findViewById(R.id.sketchfab).setOnClickListener(this);
    findViewById(R.id.service_continue).setOnClickListener(this);
    findViewById(R.id.service_show_result).setOnClickListener(this);
    findViewById(R.id.service_finish).setOnClickListener(this);
  }

  @Override
  public void onBackPressed()
  {
    Initializator.letMeGo();
    super.onBackPressed();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    mLayout.setVisibility(View.VISIBLE);
    mOperations.setVisibility(View.GONE);
    mProgress.setVisibility(View.GONE);
    if (Service.getRunning(this) > Service.SERVICE_NOT_RUNNING) {
      mLayout.setVisibility(View.GONE);
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
                mText.setText(getString(R.string.working) + "\n\n" + Service.getMessage());
              }
            });
          }
        }
      }).start();
    } else if (Service.getRunning(this) < Service.SERVICE_NOT_RUNNING) {
      mLayout.setVisibility(View.GONE);
      mList.setVisibility(View.GONE);
      mOperations.setVisibility(View.VISIBLE);
      mText.setVisibility(View.VISIBLE);
      mText.setText(getString(R.string.finished) + "\n" + getString(R.string.turn_off));
      int service = Math.abs(Service.getRunning(this));
      if (service == Service.SERVICE_SKETCHFAB)
        findViewById(R.id.service_continue).setVisibility(View.GONE);
      else if (service == Service.SERVICE_POSTPROCESS)
        finishScanning();
    } else if (first) {
      first = false;
      Intent intent1 = new Intent();
      intent1.setAction("android.intent.action.REQUEST_TANGO_PERMISSION");
      intent1.putExtra(EXTRA_KEY_PERMISSIONTYPE, EXTRA_VALUE_ADF);
      startActivityForResult(intent1, 1);
    }
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
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if(resultCode == Activity.RESULT_OK)
      setupPermissions();
    else
      finish();
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case PERMISSIONS_CODE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
          if (Build.DEVICE.toLowerCase().contains("asus_a002")) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle(getString(R.string.unsupported));
            builder.setMessage(getString(R.string.unsupported_asus));
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
            {
              @Override
              public void onClick(DialogInterface dialog, int i)
              {
                dialog.dismiss();
              }
            });
            builder.create().show();
          }
          refreshUI();
        } else
          finish();
        break;
      }
    }
  }

  @Override
  public void onClick(View v)
  {
    Intent intent = new Intent(FileManager.this, OpenConstructor.class);
    switch (v.getId()) {
      case R.id.add_button:
        showProgress();
        startScanning();
        break;
      case R.id.settings:
        startActivity(new Intent(this, Settings.class));
        break;
      case R.id.sketchfab:
        showProgress();
        startActivity(new Intent(this, Home.class));
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
        int service = Math.abs(Service.getRunning(this));
        if (service == Service.SERVICE_SKETCHFAB)
          finishOperation();
        else if (service == Service.SERVICE_SAVE)
        {
          showProgress();
          intent.putExtra(AbstractActivity.RESOLUTION_KEY, Integer.MAX_VALUE);
          startActivity(intent);
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
    mOperations.setVisibility(View.GONE);
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setTitle(getString(R.string.enter_filename));
    final EditText input = new EditText(this);
    builder.setCancelable(false);
    builder.setView(input);
    builder.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which) {

        showProgress();
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            //delete old files during overwrite
            try {
              File file = new File(getPath(), input.getText().toString() + FILE_EXT[0]);
              if (file.exists())
                for(String s : getObjResources(file))
                  if (new File(getPath(), s).delete())
                    Log.d(AbstractActivity.TAG, "File " + s + " deleted");
            } catch(Exception e) {
              e.printStackTrace();
            }

            //move file from temp into folder
            File obj = new File(getPath(), Service.getLink(FileManager.this));
            String saveFilename = input.getText().toString();
            for(String s : getObjResources(obj.getAbsoluteFile()))
              if (new File(getTempPath(), s).renameTo(new File(getPath(), s)))
                Log.d(AbstractActivity.TAG, "File " + s + " saved");
            File file2save = new File(getPath(), saveFilename + FILE_EXT[0]);
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
    });
    builder.create().show();
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
}
