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
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Point;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class OpenConstructorActivity extends AbstractActivity implements View.OnClickListener {

  private ProgressBar mProgress;
  private OpenConstructorRenderer mRenderer;
  private GLSurfaceView mGLView;
  private SeekBar mSeekbar;
  private String mToLoad;
  private boolean m3drRunning = false;
  private boolean mViewMode = false;

  private LinearLayout mLayoutRecBottom;
  private Button mToggleButton;

  private LinearLayout mLayoutRecTop;
  private TextView mResText;
  private int mRes = 3;

  private GestureDetector mGestureDetector;
  private float mMoveX = 0;
  private float mMoveY = 0;
  private float mPitch = 0;
  private float mYaw = 0;
  private float mZoom = 0;

  // Tango Service connection.
  boolean mInitialised = false;
  boolean mTangoBinded = false;
  ServiceConnection mTangoServiceConnection = new ServiceConnection() {
      public void onServiceConnected(ComponentName name, IBinder service) {
        TangoJNINative.onCreate(OpenConstructorActivity.this);
        TangoJNINative.onTangoServiceConnected(service);
        refresh3dr();
        mInitialised = true;
      }

      public void onServiceDisconnected(ComponentName name) {
      }
    };

  public OpenConstructorActivity() {
    TangoJNINative.activityCtor(false);
    mRenderer = new OpenConstructorRenderer();
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_mesh_builder);
    TangoJNINative.onToggleButtonClicked(false);

    // Setup UI elements and listeners.
    mLayoutRecBottom = (LinearLayout) findViewById(R.id.layout_rec_bottom);
    findViewById(R.id.clear_button).setOnClickListener(this);
    findViewById(R.id.save_button).setOnClickListener(this);
    mToggleButton = (Button) findViewById(R.id.toggle_button);
    mToggleButton.setOnClickListener(this);

    mLayoutRecTop = (LinearLayout) findViewById(R.id.layout_rec_top);
    mResText = (TextView) findViewById(R.id.res_text);

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

    //open file
    mToLoad = null;
    String filename = getIntent().getStringExtra(FILE_KEY);
    if ((filename != null) && (filename.length() > 0))
    {
      m3drRunning = false;
      setViewerMode();
      mProgress.setVisibility(View.VISIBLE);
      mToLoad = new File(getPath(), filename).toString();
    }
    else
      mRes = getIntent().getIntExtra(RESOLUTION_KEY, 3);
    refreshUi();
  }

  @Override
  public synchronized void onClick(View v) {
    switch (v.getId()) {
    case R.id.toggle_button:
      if (isPhotoModeOn()) {
        m3drRunning = true;
        mProgress.setVisibility(View.VISIBLE);
      }
      else
        m3drRunning = !m3drRunning;
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            TangoJNINative.onToggleButtonClicked(m3drRunning);
            new Thread(new Runnable() {
              @Override
              public void run() {
                if (isPhotoModeOn()) {
                  while(!TangoJNINative.isPhotoFinished()) {
                    try
                    {
                      Thread.sleep(10);
                    } catch (InterruptedException e)
                    {
                      e.printStackTrace();
                    }
                  }
                  OpenConstructorActivity.this.runOnUiThread(new Runnable()
                  {
                    @Override
                    public void run()
                    {
                      mProgress.setVisibility(View.GONE);
                    }
                  });
                }
              }
            }).start();
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
      setupPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, REQUEST_CODE_PERMISSION_WRITE_STORAGE);
      break;
    }
    refreshUi();
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();
    TangoJNINative.setLandscape(!isPortrait(this));

    if (mViewMode) {
      if (mToLoad != null) {
        final String file = "" + mToLoad;
        mToLoad = null;
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.onCreate(OpenConstructorActivity.this);
            TangoJNINative.load(file);
            mMoveX = TangoJNINative.centerOfStaticModel(true);
            mMoveY = TangoJNINative.centerOfStaticModel(false);
            TangoJNINative.setView(mYaw, mPitch, mMoveX, mMoveY, !mViewMode);
            OpenConstructorActivity.this.runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                mProgress.setVisibility(View.GONE);
              }
            });
            mInitialised = true;
          }
        }).start();
      }
    } else
      setupPermission(Manifest.permission.CAMERA, REQUEST_CODE_PERMISSION_CAMERA);
  }

  @Override
  protected synchronized void onPause() {
    super.onPause();
    mGLView.onPause();
    if (mInitialised)
      TangoJNINative.onPause();
    if (mTangoBinded) {
      unbindService(mTangoServiceConnection);
      Toast.makeText(this, R.string.data_lost, Toast.LENGTH_LONG).show();
    }
  }

  private void refresh3dr()
  {
    if (isPhotoModeOn()) {
      if(mRes > 0)
        TangoJNINative.set3D(mRes * 0.01, 0.6, 5.0);
      else if(mRes == 0)
        TangoJNINative.set3D(0.005, 0.6, 2.0);
    } else {
      if(mRes > 0)
        TangoJNINative.set3D(mRes * 0.01, 0.6, mRes);
      else if(mRes == 0)
        TangoJNINative.set3D(0.005, 0.6, 1);
    }

    m3drRunning = !isPhotoModeOn();
    TangoJNINative.onToggleButtonClicked(m3drRunning);
    TangoJNINative.setPhotoMode(isPhotoModeOn());
  }

  private void refreshUi() {
    int textId = m3drRunning ? R.string.pause : R.string.resume;
    if (isPhotoModeOn())
      textId = R.string.capture;
    mToggleButton.setText(textId);
    mLayoutRecBottom.setVisibility(View.VISIBLE);
    //max distance
    String text = getString(R.string.distance);
    if (isPhotoModeOn()) {
      if(mRes > 0)
        text += "5m, ";
      else if(mRes == 0)
        text += "2m, ";
    } else {
      if(mRes > 0)
        text += mRes + "m, ";
      else if(mRes == 0)
        text += "1m, ";
    }
    //3d resolution
    text += getString(R.string.resolution);
    if(mRes > 0)
      text += mRes + "cm";
    else if(mRes == 0)
      text += "0.5cm";
    text += " ";
    //warning
    if ((mRes <= 0) || ((mRes == 1) && isPhotoModeOn())) {
      text += getString(R.string.extreme);
      mResText.setTextColor(Color.RED);
    } else {
      mResText.setTextColor(Color.WHITE);
    }
    mResText.setText(text);
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
    switch (requestCode) {
      case REQUEST_CODE_PERMISSION_CAMERA: {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
          TangoInitHelper.bindTangoService(this, mTangoServiceConnection);
          mTangoBinded = true;
        } else
          finish();
        break;
      }
      case REQUEST_CODE_PERMISSION_WRITE_STORAGE: {
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
                  File file = new File(getPath(), input.getText().toString() + FILE_EXT);
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
          builder.setNegativeButton(getString(android.R.string.cancel), null);
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

  private boolean isPhotoModeOn()
  {
    SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
    String key = getString(R.string.pref_photomode);
    return pref.getBoolean(key, false);
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
