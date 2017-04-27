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

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.graphics.Point;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
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
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenConstructorActivity extends AbstractActivity implements View.OnClickListener,
        GLSurfaceView.Renderer {

  private ActivityManager mActivityManager;
  private ActivityManager.MemoryInfo mMemoryInfo;
  private ProgressBar mProgress;
  private GLSurfaceView mGLView;
  private SeekBar mSeekbar;
  private String mToLoad;
  private boolean m3drRunning = false;
  private boolean mViewMode = false;
  private long mTimestamp = 0;

  private LinearLayout mLayoutRecBottom;
  private Button mToggleButton;
  private LinearLayout mLayoutRecTop;
  private TextView mResText;
  private Button mCardboard;
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
      public void onServiceConnected(ComponentName name, IBinder srv) {
        double res      = mRes * 0.01;
        double dmin     = 0.6f;
        double dmax     = mRes * 1.5;
        int noise       = isNoiseFilterOn() ? 9 : 0;
        boolean land    = !isPortrait(OpenConstructorActivity.this);
        boolean txt     = isTexturingOn();

        if (android.os.Build.DEVICE.toLowerCase().startsWith("yellowstone"))
          land = !land;

        if(mRes == 0) {
            res = 0.005;
            dmax = 1.0;
        }

        m3drRunning = true;
        String t = getTempPath().getAbsolutePath();
        TangoJNINative.onCreate(OpenConstructorActivity.this);
        TangoJNINative.onTangoServiceConnected(srv, res, dmin, dmax, noise, land, txt, t);
        TangoJNINative.onToggleButtonClicked(m3drRunning);
        TangoJNINative.setView(0, 0, 0, 0, true);
        OpenConstructorActivity.this.runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            refreshUi();
            mProgress.setVisibility(View.GONE);
          }
        });
        mInitialised = true;
      }

      public void onServiceDisconnected(ComponentName name) {
      }
    };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_mesh_builder);
    mActivityManager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
    mMemoryInfo = new ActivityManager.MemoryInfo();
    TangoJNINative.onToggleButtonClicked(false);

    // Setup UI elements and listeners.
    mLayoutRecBottom = (LinearLayout) findViewById(R.id.layout_rec_bottom);
    findViewById(R.id.clear_button).setOnClickListener(this);
    findViewById(R.id.save_button).setOnClickListener(this);
    mToggleButton = (Button) findViewById(R.id.toggle_button);
    mToggleButton.setOnClickListener(this);

    mCardboard = (Button) findViewById(R.id.cardboard_button);
    mLayoutRecTop = (LinearLayout) findViewById(R.id.layout_rec_top);
    mResText = (TextView) findViewById(R.id.res_text);

    // OpenGL view where all of the graphics are drawn
    mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(2);
    mGLView.setRenderer(this);
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
      mToLoad = new File(getPath(), filename).toString();
      setViewerMode(mToLoad);
    }
    else
      mRes = getIntent().getIntExtra(RESOLUTION_KEY, 3);
    refreshUi();
    mProgress.setVisibility(View.VISIBLE);
  }

  @Override
  public synchronized void onClick(View v) {
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
      save();
      break;
    }
    refreshUi();
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();

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
    } else if(!mInitialised && !mTangoBinded) {
      TangoInitHelper.bindTangoService(this, mTangoServiceConnection);
      mTangoBinded = true;
    }
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

  private void refreshUi() {
    int textId = m3drRunning ? R.string.pause : R.string.resume;
    mToggleButton.setText(textId);
    mLayoutRecBottom.setVisibility(View.VISIBLE);
    //memory info
    mActivityManager.getMemoryInfo(mMemoryInfo);
    long freeMBs = mMemoryInfo.availMem / 1048576L;
    String text = freeMBs + " " + getString(R.string.mb_free) + ", ";
    //max distance
    text += getString(R.string.distance) + " ";
    if(mRes > 0)
      text += (1.5f * mRes) + " m, ";
    else if(mRes == 0)
      text += "1.0 m, ";
    //3d resolution
    text += getString(R.string.resolution) + " ";
    if(mRes > 0)
      text += mRes + " cm";
    else if(mRes == 0)
      text += "0.5 cm";
    text += " ";
    //warning
    if (mRes <= 0) {
      text += getString(R.string.extreme);
      mResText.setTextColor(Color.RED);
    } else if (freeMBs < 400)
      mResText.setTextColor(Color.RED);
    else
      mResText.setTextColor(Color.WHITE);
    mResText.setText(text);
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

  private void setViewerMode(final String filename)
  {
    mLayoutRecBottom.setVisibility(View.GONE);
    mLayoutRecTop.setVisibility(View.GONE);
    if (isCardboardEnabled(this)) {
      mCardboard.setVisibility(View.VISIBLE);
      mCardboard.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View view)
        {
          Intent i = new Intent();
          i.setAction(Intent.ACTION_VIEW);
          i.setDataAndType(Uri.parse("file://" + filename), "application/" + CARDBOARD_APP);
          startActivity(i);
          System.exit(0);
        }
      });
    }
    mSeekbar.setVisibility(View.VISIBLE);
    mMoveX = 0;
    mMoveY = 0;
    mYaw = 0;
    mZoom = 10;
    mViewMode = true;
    mSeekbar.setProgress(90);
    TangoJNINative.setZoom(mZoom);
  }

  // Render loop of the Gl context.
  public synchronized void onDrawFrame(GL10 gl) {
    TangoJNINative.onGlSurfaceDrawFrame();
    if (System.currentTimeMillis() - mTimestamp > 1000) {
      mTimestamp = System.currentTimeMillis();
      runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          refreshUi();
        }
      });
    }
  }

  // Called when the surface size changes.
  public synchronized void onSurfaceChanged(GL10 gl, int width, int height) {
    TangoJNINative.onGlSurfaceChanged(width, height);
  }

  // Called when the surface is created or recreated.
  public synchronized void onSurfaceCreated(GL10 gl, EGLConfig config) {
    TangoJNINative.onGlSurfaceCreated();
  }

  private void save() {
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
            //delete old during overwrite
            int type = isTexturingOn() ? 0 : 1;
            File file = new File(getPath(), input.getText().toString() + FILE_EXT[type]);
            if (isTexturingOn()) {
              try {
                if (file.exists())
                  for(String s : getObjResources(file))
                    if (new File(getPath(), s).delete())
                      Log.d(AbstractActivity.TAG, "File " + s + " deleted");
              } catch(Exception e) {
                e.printStackTrace();
              }
            }
            //save
            String dataset = "";
            File file2save = new File(getPath(), input.getText().toString() + FILE_EXT[type]);
            final String filename = file2save.getAbsolutePath();
            if (isTexturingOn()) {
              long timestamp = System.currentTimeMillis();
              File obj = new File(getPath(), timestamp + FILE_EXT[type]);
              for (File f : getTempPath().listFiles())
                if (f.isDirectory()) {
                  dataset = f.toString();
                  break;
                }
              TangoJNINative.save(obj.getAbsolutePath(), dataset);
              if (obj.renameTo(file2save))
                Log.d(TAG, "Obj file " + file2save.toString() + " saved.");
            } else
              TangoJNINative.save(filename, dataset);
            //open???
            OpenConstructorActivity.this.runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                mProgress.setVisibility(View.GONE);
                AlertDialog.Builder builder = new AlertDialog.Builder(OpenConstructorActivity.this);
                builder.setTitle(getString(R.string.view));
                builder.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    dialog.cancel();
                  }
                });
                builder.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    setViewerMode(filename);
                    if (isTexturingOn()) {
                      mProgress.setVisibility(View.VISIBLE);
                      new Thread(new Runnable()
                      {
                        @Override
                        public void run()
                        {
                          TangoJNINative.onClearButtonClicked();
                          TangoJNINative.load(filename);
                          OpenConstructorActivity.this.runOnUiThread(new Runnable()
                          {
                            @Override
                            public void run()
                            {
                              mProgress.setVisibility(View.GONE);
                            }
                          });
                        }
                      }).start();
                    }
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
