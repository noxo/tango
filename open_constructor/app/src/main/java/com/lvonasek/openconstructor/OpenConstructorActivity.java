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
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Point;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class OpenConstructorActivity extends Activity implements View.OnClickListener {

  private static final String MODEL_DIRECTORY = "/Models/";
  private static final int REQUEST_CODE_PERMISSION_CAMERA = 1987;
  private static final int REQUEST_CODE_PERMISSION_STORAGE = 1988;

  private ProgressBar mProgress;
  private OpenConstructorRenderer mRenderer;
  private GLSurfaceView mGLView;
  private SeekBar mSeekbar;

  private LinearLayout mLayoutRecBottom;
  private Button mClearButton;
  private Button mSaveButton;
  private Button mToggleButton;

  private LinearLayout mLayoutRecTop;
  private TextView mResText;
  private Button mResPlus;
  private Button mResMinus;
  private int mRes = 3;

  private boolean mViewMode = false;
  private GestureDetector mGestureDetector;
  private float mMoveX = 0;
  private float mMoveY = 0;
  private float mPitch = 0;
  private float mYaw = 0;
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
    mLayoutRecBottom = (LinearLayout) findViewById(R.id.layout_rec_bottom);
    mClearButton = (Button) findViewById(R.id.clear_button);
    mClearButton.setOnClickListener(this);
    mToggleButton = (Button) findViewById(R.id.toggle_button);
    mToggleButton.setOnClickListener(this);
    mSaveButton = (Button) findViewById(R.id.save_button);
    mSaveButton.setOnClickListener(this);

    mLayoutRecTop = (LinearLayout) findViewById(R.id.layout_rec_top);
    mResText = (TextView) findViewById(R.id.res_text);
    mResPlus = (Button) findViewById(R.id.res_plus);
    mResPlus.setOnClickListener(this);
    mResMinus = (Button) findViewById(R.id.res_minus);
    mResMinus.setOnClickListener(this);

    // OpenGL view where all of the graphics are drawn
    mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(2);
    mGLView.setRenderer(mRenderer);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);
    mSeekbar = (SeekBar) findViewById(R.id.seekBar);
    mSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
    {
      @Override
      public void onProgressChanged(SeekBar seekBar, int value, boolean byUser)
      {
        mPitch = (float) Math.toRadians(-value);
        TangoJNINative.setView(mYaw, mPitch, mMoveX, mMoveY, !mViewMode);
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar)
      {
      }

      @Override
      public void onStopTrackingTouch(SeekBar seekBar)
      {
      }
    });
    refreshUi();

    mGestureDetector = new GestureDetector(new GestureDetector.GestureListener()
    {
      @Override
      public void OnMove(float dx, float dy)
      {
        double angle = -mYaw;
        float f = getMoveFactor();
        mMoveX += dx * f * Math.cos( angle ) + dy * f * Math.sin( angle );
        mMoveY += dx * f * Math.sin( angle ) - dy * f * Math.cos( angle );
        TangoJNINative.setView(mYaw, mPitch, mMoveX, mMoveY, !mViewMode);
      }

      @Override
      public void OnRotation(float angle)
      {
        mYaw = (float) Math.toRadians(-angle);
        TangoJNINative.setView(mYaw, mPitch, mMoveX, mMoveY, !mViewMode);
      }

      @Override
      public void OnZoom(float diff)
      {
        mZoom -= diff;
        int min = mViewMode ? 1 : 0;
        if(mZoom < min)
          mZoom = min;
        if(mZoom > 10)
          mZoom = 10;
        TangoJNINative.setZoom(mZoom);
      }
    }, this);
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
    case R.id.save_button:
      //pause
      m3drRunning = false;
      TangoJNINative.onToggleButtonClicked(false);
      refreshUi();
      //save
      setupPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, REQUEST_CODE_PERMISSION_STORAGE);
      break;
    case R.id.res_minus:
      mRes--;
      if(mRes <= -1)
        mRes = -1;
      refresh3dr();
      refreshUi();
      break;
    case R.id.res_plus:
      mRes++;
      if(mRes > 10)
        mRes = 10;
      refresh3dr();
      refreshUi();
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
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
    {
      if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
        requestPermissions(new String[]{permission}, requestCode);
      else
        onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
    } else
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
                          setViewerMode();
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
  public void onBackPressed()
  {
    System.exit(0);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    mGestureDetector.onTouchEvent(event);
    return true;
  }

  private float getMoveFactor()
  {
    Display display = getWindowManager().getDefaultDisplay();
    Point size = new Point();
    display.getSize(size);
    return 2.0f / (size.x + size.y) * (float)Math.pow(mZoom, 0.5f) * 2.0f;
  }

  private String getPath() {
    String dir = Environment.getExternalStorageDirectory().getPath() + MODEL_DIRECTORY;
    new File(dir).mkdir();
    return dir;
  }

  private void setViewerMode()
  {
    mLayoutRecBottom.setVisibility(View.GONE);
    mLayoutRecTop.setVisibility(View.GONE);
    mSeekbar.setVisibility(View.VISIBLE);
    mMoveX = 0;
    mMoveY = 0;
    mYaw = 0;
    mZoom = 10;
    mViewMode = true;
    mSeekbar.setProgress(90);
    TangoJNINative.setZoom(mZoom);
  }
}
