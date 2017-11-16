package com.lvonasek.daydreamOBJ;

import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Bundle;

import com.google.vr.sdk.base.AndroidCompat;
import com.google.vr.sdk.base.Eye;
import com.google.vr.sdk.base.GvrActivity;
import com.google.vr.sdk.base.GvrView;
import com.google.vr.sdk.base.HeadTransform;
import com.google.vr.sdk.base.Viewport;
import com.google.vr.sdk.controller.Controller;
import com.google.vr.sdk.controller.ControllerManager;

import javax.microedition.khronos.egl.EGLConfig;

public class MainActivity extends GvrActivity implements GvrView.StereoRenderer, Runnable
{
  private ControllerManager controllerManager;

  private float[] headView = new float[16];
  private float[] modelViewProjection = new float[16];
  private float[] rotation = new float[16];
  private long timestamp = 0;
  private float addYaw = 0;
  private float touchX = 0;
  private float yaw = 0;
  private float speed = 0.05f;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.common_ui);

    //stereo view
    GvrView gvrView = (GvrView) findViewById(R.id.gvr_view);
    gvrView.setEGLConfigChooser(8, 8, 8, 8, 24, 8);
    gvrView.setRenderer(this);
    gvrView.setTransitionViewEnabled(true);
    gvrView.setDistortionCorrectionEnabled(true);
    if (gvrView.setAsyncReprojectionEnabled(true))
      AndroidCompat.setSustainedPerformanceMode(this, true);
    setGvrView(gvrView);
  }


  @Override
  protected void onStart() {
    super.onStart();
    controllerManager = new ControllerManager(this, new ControllerManager.EventListener()
    {
      @Override
      public void onApiStatusChanged(int i)
      {
      }

      @Override
      public void onRecentered()
      {
        yaw = 0;
      }
    });
    controllerManager.start();
  }

  @Override
  protected void onStop() {
    controllerManager.stop();
    controllerManager = null;
    super.onStop();
  }

  @Override
  public synchronized void onRendererShutdown() {
  }

  @Override
  public synchronized void onSurfaceChanged(int width, int height) {
  }

  @Override
  public synchronized void onSurfaceCreated(EGLConfig config) {
    GLES20.glClearColor(0, 0, 0, 1);
    System.loadLibrary("daydream");
    nativeInitializeGl();
    nativeLoadModel(EntryActivity.filename);
  }

  @Override
  public synchronized void onNewFrame(HeadTransform headTransform) {
    Matrix.setRotateM(rotation, 0, yaw, 0, 1, 0);
    headTransform.getHeadView(headView, 0);
    Matrix.multiplyMM(headView, 0, headView, 0, rotation, 0);
  }

  @Override
  public synchronized void onDrawEye(Eye eye) {
    GLES20.glEnable(GLES20.GL_DEPTH_TEST);
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
    float[] perspective = eye.getPerspective(0.1f, 1000.0f);
    float[] view = eye.getEyeView();
    Matrix.multiplyMM(view, 0, view, 0, rotation, 0);
    Matrix.multiplyMM(modelViewProjection, 0, perspective, 0, view, 0);
    nativeDrawFrame(modelViewProjection);
  }

  @Override
  public synchronized void onFinishFrame(Viewport viewport) {
    runOnUiThread(this);
    nativeUpdate();
  }

  @Override
  public synchronized void run()
  {
    if (controllerManager == null)
      return;
    Controller controller = controllerManager.getController();
    controller.update();
    if (timestamp > System.currentTimeMillis())
      return;
    if (SelectorView.active)
    {
      // model selector
      nativeLoadModel("");
      if (controller.clickButtonState)
      {
        SelectorView.activate(false);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            EntryActivity.filename = SelectorView.getSelected();
            nativeLoadModel(EntryActivity.filename);
          }
        }).start();
        timestamp = System.currentTimeMillis() + 500;
      }
      else if (controller.appButtonState)
      {
        if (!EntryActivity.filename.isEmpty())
        {
          SelectorView.activate(false);
          timestamp = System.currentTimeMillis() + 500;
        }
      }
      else if (controller.isTouching)
      {
        if (touchX == 0)
          touchX = controller.touch.x;
        float diff = touchX - controller.touch.x;
        if (Math.abs(diff) > 0.5f)
        {
          SelectorView.update(diff < 0);
          timestamp = System.currentTimeMillis() + 500;
        }
      } else {
        touchX = 0;
      }
    } else {
      //scene moving
      if (controller.appButtonState)
      {
        SelectorView.activate(true);
        timestamp = System.currentTimeMillis() + 500;
      }
      if (controller.clickButtonState)
        nativeOnTriggerEvent(0, 0, speed, headView);
      if (controller.volumeUpButtonState)
        speed *= 1.25f;
      if (controller.volumeDownButtonState)
        speed /= 1.25f;
      if (controller.isTouching)
      {
        if (touchX == 0)
          touchX = controller.touch.x;
        addYaw = touchX - controller.touch.x;
      } else {
        touchX = 0;
      }
      addYaw *= 0.95f;
      yaw -= 2.0f * addYaw;
    }
  }

  private synchronized native void nativeLoadModel(String filename);
  private synchronized native void nativeInitializeGl();
  private synchronized native void nativeDrawFrame(float[] matrix);
  private synchronized native void nativeOnTriggerEvent(float x, float y, float z, float[] matrix);
  private synchronized native void nativeUpdate();
}
