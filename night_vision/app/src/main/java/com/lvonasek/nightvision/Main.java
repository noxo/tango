package com.lvonasek.nightvision;

import android.app.Activity;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.view.WindowManager;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class Main extends Activity implements GLSurfaceView.Renderer {

  private GLSurfaceView mGLView;

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
            JNI.onTangoServiceConnected(srv, 5, 0.5f, 10, 0, updown);
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
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();

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
  }
}
