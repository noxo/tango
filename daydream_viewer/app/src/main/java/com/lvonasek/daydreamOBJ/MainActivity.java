package com.lvonasek.daydreamOBJ;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.nfc.NfcAdapter;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AbstractActivity {
  static {
    System.loadLibrary("gvr");
    System.loadLibrary("daydream");
  }

  // Opaque native pointer to the native TreasureHuntRenderer instance.
  private long nativeTreasureHuntRenderer;

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;

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
  protected void onAddressChanged(String address)
  {
    Toast.makeText(this, "Connected to controller " + address, Toast.LENGTH_LONG).show();
  }

  @Override
  protected void onConnectionChanged(boolean on)
  {
  }

  @Override
  protected void onDataReceived()
  {
    if (DaydreamController.getStatus().get(DaydreamController.BTN_CLICK) > 0)
      nativeOnTriggerEvent(nativeTreasureHuntRenderer);
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
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
    gvrLayout = new GvrLayout(this);

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

    nativeTreasureHuntRenderer = nativeCreateRenderer(getClass().getClassLoader(),
            getApplicationContext(), gvrLayout.getGvrApi().getNativeGvrContext(), EntryActivity.filename);

    // Enable VR Mode.
    AndroidCompat.setVrModeEnabled(this, true);
  }

  @Override
  protected void onPause() {
    surfaceView.queueEvent(pauseNativeRunnable);
    surfaceView.onPause();
    gvrLayout.onPause();
    //NFC
    try {
      NfcAdapter.getDefaultAdapter(this).disableForegroundDispatch(this);
    } catch(Exception e) {
      e.printStackTrace();
    }
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    gvrLayout.onResume();
    surfaceView.onResume();
    surfaceView.queueEvent(resumeNativeRunnable);
    //NFC
    try {
      NfcAdapter adapter = NfcAdapter.getDefaultAdapter(this);
      PendingIntent pendingIntent = PendingIntent.getActivity(
              this, 0, new Intent(this, getClass()).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
      adapter.enableForegroundDispatch(this, pendingIntent, null, null);
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    gvrLayout.shutdown();
    nativeDestroyRenderer(nativeTreasureHuntRenderer);
    nativeTreasureHuntRenderer = 0;
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event)
  {
    // Avoid accidental volume key presses while the phone is in the VR headset.
    return event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP || event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN || super.dispatchKeyEvent(event);
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

  @Override
  public void onNewIntent(Intent intent) {}

  private native long nativeCreateRenderer(ClassLoader appClassLoader, Context context, long gvr, String filename);
  private native void nativeDestroyRenderer(long nativeTreasureHuntRenderer);
  private native void nativeInitializeGl(long nativeTreasureHuntRenderer);
  private native long nativeDrawFrame(long nativeTreasureHuntRenderer);
  private native void nativeOnTriggerEvent(long nativeTreasureHuntRenderer);
  private native void nativeOnPause(long nativeTreasureHuntRenderer);
  private native void nativeOnResume(long nativeTreasureHuntRenderer);
}
