/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.lvonasek.openconstructor;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.IBinder;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class OpenConstructorActivity extends Activity implements View.OnClickListener {

  private static final String PLAY_STORE = "https://play.google.com/store/apps/details?id=";
  private static final String PLY_VIEWER = "net.chucknology.tango.scanview";

  private static final int REQUEST_CODE_PERMISSION_CAMERA = 1987;
  private static final int REQUEST_CODE_PERMISSION_STORAGE = 1988;

  private ProgressBar mProgress;
  private OpenConstructorRenderer mRenderer;
  private GLSurfaceView mGLView;

  private Button mClearButton;
  private Button mSaveButton;
  private Button mToggleButton;

  private TextView mResText;
  private Button mResPlus;
  private Button mResMinus;
  private int mRes = 3;

  private ScaleGestureDetector mScaleDetector;
  private float mZoom = 0;

  private boolean m3drRunning = true;

  // Tango Service connection.
  boolean mInitialised = false;
  ServiceConnection mTangoServiceConnection = new ServiceConnection() {
      public void onServiceConnected(ComponentName name, IBinder service) {
        TangoJNINative.onCreate(OpenConstructorActivity.this);
        TangoJNINative.onTangoServiceConnected(service);
      }

      public void onServiceDisconnected(ComponentName name) {
      }
    };

  public OpenConstructorActivity() {
    TangoJNINative.activityCtor(m3drRunning);
    mRenderer = new OpenConstructorRenderer();
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_mesh_builder);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    // Setup UI elements and listeners.
    mClearButton = (Button) findViewById(R.id.clear_button);
    mClearButton.setOnClickListener(this);

    mToggleButton = (Button) findViewById(R.id.toggle_button);
    mToggleButton.setOnClickListener(this);

    mSaveButton = (Button) findViewById(R.id.save_button);
    mSaveButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        //pause
        m3drRunning = false;
        TangoJNINative.onToggleButtonClicked(m3drRunning);
        refreshUi();
        //save
        setupPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, REQUEST_CODE_PERMISSION_STORAGE);
      }
    });

    mResText = (TextView) findViewById(R.id.res_text);
    mResPlus = (Button) findViewById(R.id.res_plus);
    mResMinus = (Button) findViewById(R.id.res_minus);

    mResPlus.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        mRes++;
        if(mRes > 10)
          mRes = 10;
        refresh3dr();
        refreshUi();
      }
    });
    mResMinus.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        mRes--;
        if(mRes <= -1)
          mRes = -1;
        refresh3dr();
        refreshUi();
      }
    });

    // OpenGL view where all of the graphics are drawn
    mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(2);
    mGLView.setRenderer(mRenderer);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);

    refreshUi();

    mScaleDetector = new ScaleGestureDetector(this, new ScaleGestureDetector.OnScaleGestureListener() {
      @Override
      public void onScaleEnd(ScaleGestureDetector detector) {
      }
      @Override
      public boolean onScaleBegin(ScaleGestureDetector detector) {
        return true;
      }
      @Override
      public boolean onScale(ScaleGestureDetector detector) {
        mZoom -= detector.getScaleFactor() - 1;
        if(mZoom < 0)
          mZoom = 0;
        if(mZoom > 10)
          mZoom = 10;
        TangoJNINative.setZoom(mZoom);
        return false;
      }
    });
  }

  @Override
  public void onClick(View v) {
    switch (v.getId()) {
    case R.id.toggle_button:
      m3drRunning = !m3drRunning;
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            TangoJNINative.onToggleButtonClicked(m3drRunning);
          }
        });
      break;
    case R.id.clear_button:
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            TangoJNINative.onClearButtonClicked();
          }
        });
      break;
    }

    refreshUi();
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();
    setupPermission(Manifest.permission.CAMERA, REQUEST_CODE_PERMISSION_CAMERA);
  }

  @Override
  protected synchronized void onPause() {
    super.onPause();
    mGLView.onPause();
    if(mInitialised) {
      TangoJNINative.onPause();
      unbindService(mTangoServiceConnection);
      Toast.makeText(this, R.string.data_lost, Toast.LENGTH_LONG).show();
    }
  }

  private void openModel(String filename, Context context) {
    try {
      //open viewer
      Intent intent = new Intent(Intent.ACTION_VIEW);
      intent.setDataAndType(Uri.fromFile(new File(filename)), "text/plain");
      intent.setPackage(PLY_VIEWER);
      startActivity(intent);
    } catch (Exception e) {
      //App not found
      Toast.makeText(context, R.string.external_app, Toast.LENGTH_LONG).show();
      Uri uri = Uri.parse(PLAY_STORE + PLY_VIEWER);
      startActivity(new Intent(Intent.ACTION_VIEW,uri));
    }
    System.exit(0);
  }

  private void refresh3dr()
  {
    if(mRes > 0)
      TangoJNINative.set3D(mRes * 0.01, 0.6, mRes);
    else if(mRes == 0)
      TangoJNINative.set3D(0.005, 0.6, 0.85);
    else if(mRes == -1)
      TangoJNINative.set3D(0.0025, 0.6, 0.75);
  }

  private void refreshUi() {
    int textId = m3drRunning ? R.string.pause : R.string.resume;
    mToggleButton.setText(textId);
    //max distance
    String text = getString(R.string.distance);
    if(mRes > 0)
      text += mRes + "m, ";
    else if(mRes == 0)
      text += "0.85m, ";
    else if(mRes == -1)
      text += "0.75m, ";
    //3d resolution
    text += getString(R.string.resolution);
    if(mRes > 0)
      text += mRes + "cm";
    else if(mRes == 0)
      text += "0.5cm";
    else if(mRes == -1)
      text += "0.25cm";
    text += "\n";
    //warning
    if(mRes <= 0) {
      text += getString(R.string.extreme);
      mResText.setTextColor(Color.RED);
    } else {
      mResText.setTextColor(Color.WHITE);
    }
    mResText.setText(text);
  }

  private void setupPermission(String permission, int requestCode) {
    if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
      requestPermissions(new String[]{permission}, requestCode);
    else
      onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
    switch (requestCode) {
      case REQUEST_CODE_PERMISSION_CAMERA: {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
        {
          TangoInitHelper.bindTangoService(this, mTangoServiceConnection);
          mInitialised = true;
        } else
          finish();
        break;
      }
      case REQUEST_CODE_PERMISSION_STORAGE: {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
          //filename dialog
          final Context context = OpenConstructorActivity.this;
          AlertDialog.Builder builder = new AlertDialog.Builder(context);
          builder.setTitle(getString(R.string.enter_filename));
          final EditText input = new EditText(context);
          builder.setView(input);
          builder.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
              mProgress.setVisibility(View.VISIBLE);
              new Thread(new Runnable(){
                @Override
                public void run()
                {
                  //save
                  File file = new File(getPath(), input.getText().toString() + ".ply");
                  final String filename = file.getAbsolutePath();
                  TangoJNINative.save(filename);
                  //open???
                  OpenConstructorActivity.this.runOnUiThread(new Runnable()
                  {
                    @Override
                    public void run()
                    {
                      mProgress.setVisibility(View.GONE);
                      AlertDialog.Builder builder = new AlertDialog.Builder(context);
                      builder.setTitle(getString(R.string.view));
                      builder.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                          openModel(filename, context);
                          dialog.cancel();
                        }
                      });
                      builder.setNegativeButton(getString(android.R.string.cancel), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                          dialog.cancel();
                        }
                      });
                      builder.create().show();
                    }
                  });
                }
              }).start();
              dialog.cancel();
            }
          });
          builder.setNegativeButton(getString(android.R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
              dialog.cancel();
            }
          });
          builder.create().show();
        }
      }
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    mScaleDetector.onTouchEvent(event);
    return true;
  }

  private String getPath() {
    String dir = OpenConstructorActivity.this.getExternalMediaDirs()[0].toString();
    dir = dir.substring(0, dir.indexOf("Android"));
    dir += "Models/";
    new File(dir).mkdir();
    return dir;
  }
}
