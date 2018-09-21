package com.lvonasek.daydreamOBJ;

import com.google.vr.sdk.base.Viewport;

public class CardboardActivity extends VRActivity
{
  @Override
  public void onCardboardTrigger() {
    nativeOnTriggerEvent(0, 0, 1, headView);
  }

  @Override
  public synchronized void onFinishFrame(Viewport viewport) {
    nativeUpdate();
  }
}