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

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.common_ui);

    //stereo view
    GvrView gvrView = (GvrView) findViewById(R.id.gvr_view);
    gvrView.setEGLConfigChooser(8, 8, 8, 8, 16, 8);
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
    nativeDestroyRenderer();
  }

  @Override
  public synchronized void onSurfaceChanged(int width, int height) {
  }

  @Override
  public synchronized void onSurfaceCreated(EGLConfig config) {
    GLES20.glClearColor(0.1f, 0.1f, 0.1f, 0.5f);
    System.loadLibrary("daydream");
    nativeInitializeGl();
    nativeCreateRenderer(EntryActivity.filename);
    nativeDrawFrame(modelViewProjection);
  }

  @Override
  public synchronized void onNewFrame(HeadTransform headTransform) {
    headTransform.getHeadView(headView, 0);
  }

  @Override
  public synchronized void onDrawEye(Eye eye) {
    GLES20.glEnable(GLES20.GL_DEPTH_TEST);
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
    float[] perspective = eye.getPerspective(0.1f, 1000.0f);
    Matrix.multiplyMM(modelViewProjection, 0, perspective, 0, eye.getEyeView(), 0);
    nativeDrawFrame(modelViewProjection);
  }

  @Override
  public synchronized void onFinishFrame(Viewport viewport) {
    runOnUiThread(this);
    nativeUpdate();
  }

  @Override
  public void run()
  {
    Controller controller = controllerManager.getController();
    controller.update();
    if (controller.clickButtonState)
      nativeOnTriggerEvent(0, 0, 0.05f, headView);
  }

  private synchronized native void nativeCreateRenderer(String filename);
  private synchronized native void nativeDestroyRenderer();
  private synchronized native void nativeInitializeGl();
  private synchronized native void nativeDrawFrame(float[] matrix);
  private synchronized native void nativeOnTriggerEvent(float x, float y, float z, float[] matrix);
  private synchronized native void nativeUpdate();
}
