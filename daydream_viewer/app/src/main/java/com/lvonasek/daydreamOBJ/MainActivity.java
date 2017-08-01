package com.lvonasek.daydreamOBJ;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Vibrator;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity {
  static {
    System.loadLibrary("gvr");
    System.loadLibrary("daydream");
  }

  // Opaque native pointer to the native TreasureHuntRenderer instance.
  private long nativeTreasureHuntRenderer;

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;
  private boolean initialized;

  // Note that pause and resume signals to the native renderer are performed on the GL thread,
  // ensuring thread-safety.
  private final Runnable pauseNativeRunnable =
      new Runnable() {
        @Override
        public void run() {
          nativeOnPause(nativeTreasureHuntRenderer);
        }
      };

  private final Runnable resumeNativeRunnable =
      new Runnable() {
        @Override
        public void run() {
          nativeOnResume(nativeTreasureHuntRenderer);
        }
      };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Ensure fullscreen immersion.
    setImmersiveSticky();
    getWindow()
        .getDecorView()
        .setOnSystemUiVisibilityChangeListener(
            new View.OnSystemUiVisibilityChangeListener() {
              @Override
              public void onSystemUiVisibilityChange(int visibility) {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                  setImmersiveSticky();
                }
              }
            });

    // Initialize GvrLayout and the native renderer.
    initialized = false;
    String filename = null;
    try
    {
      filename = getIntent().getData().toString().substring(7);
      try
      {
        filename = URLDecoder.decode(filename, "UTF-8");
      } catch (UnsupportedEncodingException e)
      {
        e.printStackTrace();
      }
    } catch(Exception e) {
      Toast.makeText(this, R.string.no_file, Toast.LENGTH_LONG).show();
      finish();
      return;
    }
    initialized = true;
    gvrLayout = new GvrLayout(this);
    nativeTreasureHuntRenderer =
        nativeCreateRenderer(getClass().getClassLoader(), this.getApplicationContext(),
            gvrLayout.getGvrApi().getNativeGvrContext(), filename);

    // Add the GLSurfaceView to the GvrLayout.
    surfaceView = new GLSurfaceView(this);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setRenderer(
        new GLSurfaceView.Renderer() {
          @Override
          public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            nativeInitializeGl(nativeTreasureHuntRenderer);
          }

          @Override
          public void onSurfaceChanged(GL10 gl, int width, int height) {}

          @Override
          public void onDrawFrame(GL10 gl) {
            nativeDrawFrame(nativeTreasureHuntRenderer);
          }
        });
    surfaceView.setOnTouchListener(
        new View.OnTouchListener() {
          @Override
          public boolean onTouch(View v, MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
              // Give user feedback and signal a trigger event.
              ((Vibrator) getSystemService(Context.VIBRATOR_SERVICE)).vibrate(50);
              surfaceView.queueEvent(
                  new Runnable() {
                    @Override
                    public void run() {
                      nativeOnTriggerEvent(nativeTreasureHuntRenderer);
                    }
                  });
              return true;
            }
            return false;
          }
        });
    gvrLayout.setPresentationView(surfaceView);

    // Add the GvrLayout to the View hierarchy.
    setContentView(gvrLayout);

    // Enable scan line racing.
    if (gvrLayout.setAsyncReprojectionEnabled(true)) {
      AndroidCompat.setSustainedPerformanceMode(this, true);
    }

    // Enable VR Mode.
    AndroidCompat.setVrModeEnabled(this, true);
  }

  @Override
  protected void onPause() {
    if (initialized) {
      surfaceView.queueEvent(pauseNativeRunnable);
      surfaceView.onPause();
      gvrLayout.onPause();
    }
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    if (initialized) {
      gvrLayout.onResume();
      surfaceView.onResume();
      surfaceView.queueEvent(resumeNativeRunnable);
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (initialized) {
      gvrLayout.shutdown();
      nativeDestroyRenderer(nativeTreasureHuntRenderer);
      nativeTreasureHuntRenderer = 0;
    }
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {
    // Avoid accidental volume key presses while the phone is in the VR headset.
    if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP
        || event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) {
      return true;
    }
    return super.dispatchKeyEvent(event);
  }

  private void setImmersiveSticky() {
    getWindow()
        .getDecorView()
        .setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
  }

  private native long nativeCreateRenderer(ClassLoader appClassLoader, Context context, long gvr, String filename);
  private native void nativeDestroyRenderer(long nativeTreasureHuntRenderer);
  private native void nativeInitializeGl(long nativeTreasureHuntRenderer);
  private native long nativeDrawFrame(long nativeTreasureHuntRenderer);
  private native void nativeOnTriggerEvent(long nativeTreasureHuntRenderer);
  private native void nativeOnPause(long nativeTreasureHuntRenderer);
  private native void nativeOnResume(long nativeTreasureHuntRenderer);
}
