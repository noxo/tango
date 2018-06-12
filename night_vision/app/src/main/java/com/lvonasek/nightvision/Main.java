package com.lvonasek.nightvision;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class Main extends Activity implements GLSurfaceView.Renderer, View.OnClickListener {

  // Layout
  private Button mButtonColor;
  private Button mButtonFar;
  private Button mButtonMode;
  private Button mButtonNear;
  private GLSurfaceView mGLView;
  private LinearLayout mLayout;

  // Config
  private int mColor;
  private int mFar;
  private int mNear;
  private int mMode;

  // Tango Service connection.
  boolean mInitialised = false;
  boolean mTangoBinded = false;
  ServiceConnection mTangoServiceConnection = new ServiceConnection() {
      public void onServiceConnected(ComponentName name, final IBinder srv) {
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {

            boolean updown = Build.MANUFACTURER.toUpperCase().contains("ASUS");
            JNI.onTangoServiceConnected(srv, updown);
            mInitialised = true;
          }
        }).start();
      }

      public void onServiceDisconnected(ComponentName name) {
      }
    };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    setContentView(R.layout.activity_main);

    // OpenGL view where all of the graphics are drawn
    mGLView = findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(3);
    mGLView.setRenderer(this);
    mGLView.setOnClickListener(this);

    mLayout = findViewById(R.id.layout);
    mLayout.setVisibility(View.GONE);
    mLayout.setOnClickListener(this);

    mButtonColor = findViewById(R.id.color_button);
    mButtonColor.setOnClickListener(this);
    mButtonFar = findViewById(R.id.far_button);
    mButtonFar.setOnClickListener(this);
    mButtonMode = findViewById(R.id.mode_button);
    mButtonMode.setOnClickListener(this);
    mButtonNear = findViewById(R.id.near_button);
    mButtonNear.setOnClickListener(this);

    SharedPreferences pref = getPreferences(Context.MODE_PRIVATE);
    mColor = pref.getInt("color", 0);
    mMode = pref.getInt("mode", 1);
    mNear = pref.getInt("near", 1);
    mFar = pref.getInt("far", 1);
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();
    //setType();

    if(!mInitialised && !mTangoBinded) {
      TangoInitHelper.bindTangoService(this, mTangoServiceConnection);
      mTangoBinded = true;
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    System.exit(0);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    JNI.onGlSurfaceDrawFrame();
  }

  @Override
  public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {}

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    JNI.onGlSurfaceChanged(width, height);
    applyConfig();
  }

  private void applyConfig()
  {
    //Loop possible configs
    mColor %= getResources().getStringArray(R.array.view_colors).length;
    mFar %= getResources().getStringArray(R.array.point_size).length;
    mNear %= getResources().getStringArray(R.array.point_size).length;
    mMode %= getResources().getStringArray(R.array.view_modes).length;

    //Apply config
    switch(mMode) {
      //Normal
      case 0:
        JNI.setViewParams(1, 1.0f, 2.0f, 0.0f);
        break;
      //Fullscreen
      case 1:
        JNI.setViewParams(1, 1.8f, 2.6f, 0.0f);
        break;
      //VR
      case 2:
        if (Build.BRAND.toUpperCase().startsWith("ASUS"))
          JNI.setViewParams(2, 1.5f, 1.5f, 0.0f);
        else
          JNI.setViewParams(2, 1.25f, 1.25f, 0.1f);
        break;
    }
    JNI.setColorParams(mColor);
    JNI.setPointParams((float)Math.pow(mNear + 4, 2.0f), (float)Math.pow(mFar + 1, 4.0f));

    //Set ui text
    mButtonColor.setText(getString(R.string.view_colors) + "\n" + getResources().getStringArray(R.array.view_colors)[mColor]);
    mButtonFar.setText(getString(R.string.points_far) + "\n" + getResources().getStringArray(R.array.point_size)[mFar]);
    mButtonNear.setText(getString(R.string.points_near) + "\n" + getResources().getStringArray(R.array.point_size)[mNear]);
    mButtonMode.setText(getString(R.string.view_mode) + "\n" + getResources().getStringArray(R.array.view_modes)[mMode]);

    //Save
    SharedPreferences.Editor e = getPreferences(Context.MODE_PRIVATE).edit();
    e.putInt("color", mColor);
    e.putInt("mode", mMode);
    e.putInt("near", mNear);
    e.putInt("far", mFar);
    e.apply();
  }

  @Override
  public void onClick(View v) {
    switch(v.getId()) {
      case R.id.gl_surface_view:
        if (mLayout.getVisibility() == View.VISIBLE)
          mLayout.setVisibility(View.GONE);
        else
          mLayout.setVisibility(View.VISIBLE);
        break;
      case R.id.color_button:
        mColor++;
        applyConfig();
        break;
      case R.id.far_button:
        mFar++;
        applyConfig();
        break;
      case R.id.near_button:
        mNear++;
        applyConfig();
        break;
      case R.id.mode_button:
        mMode++;
        applyConfig();
        break;
    }
  }
}
