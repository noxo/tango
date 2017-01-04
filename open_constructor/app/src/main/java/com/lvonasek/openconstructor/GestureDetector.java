package com.lvonasek.openconstructor;

import android.content.Context;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

public class GestureDetector
{
  private static final int INVALID_ANGLE = 9999;
  private static final int INVALID_POINTER_ID = -1;
  private float fX, fY, sX, sY;
  private int ptrID1, ptrID2;
  private float mAngle = 0;
  private float mLastAngle = INVALID_ANGLE + 1;
  private float mMoveX = 0;
  private float mMoveY = 0;
  private boolean mMoveValid = false;

  private GestureListener mListener;
  private ScaleGestureDetector mScaleDetector;

  public GestureDetector(GestureListener listener, Context context)
  {
    mListener = listener;
    ptrID1 = INVALID_POINTER_ID;
    ptrID2 = INVALID_POINTER_ID;

    mScaleDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.OnScaleGestureListener() {
      @Override
      public void onScaleEnd(ScaleGestureDetector detector) {
      }
      @Override
      public boolean onScaleBegin(ScaleGestureDetector detector) {
        return true;
      }
      @Override
      public boolean onScale(ScaleGestureDetector detector) {
        if (mListener != null)
          mListener.OnZoom(detector.getScaleFactor() - 1.0f);
        return false;
      }
    });
  }

  public boolean onTouchEvent(MotionEvent event)
  {
    mScaleDetector.onTouchEvent(event);
    switch (event.getActionMasked())
    {
      case MotionEvent.ACTION_DOWN:
        ptrID1 = event.getPointerId(event.getActionIndex());
        mMoveX = event.getRawX();
        mMoveY = event.getRawY();
        mMoveValid = true;
        break;
      case MotionEvent.ACTION_POINTER_DOWN:
        ptrID2 = event.getPointerId(event.getActionIndex());
        sX = event.getX(event.findPointerIndex(ptrID1));
        sY = event.getY(event.findPointerIndex(ptrID1));
        fX = event.getX(event.findPointerIndex(ptrID2));
        fY = event.getY(event.findPointerIndex(ptrID2));
        mMoveValid = false;
        break;
      case MotionEvent.ACTION_MOVE:
        if (ptrID1 != INVALID_POINTER_ID && ptrID2 != INVALID_POINTER_ID)
        {
          float nfX, nfY, nsX, nsY;
          nsX = event.getX(event.findPointerIndex(ptrID1));
          nsY = event.getY(event.findPointerIndex(ptrID1));
          nfX = event.getX(event.findPointerIndex(ptrID2));
          nfY = event.getY(event.findPointerIndex(ptrID2));

          float angle = angleBetweenLines(fX, fY, sX, sY, nfX, nfY, nsX, nsY);
          float gap = angle - mLastAngle;
          while(gap > 180)
          {
            gap -= 360.0f;
          }
          while(gap < -180)
          {
            gap += 360.0f;
          }
          if(mLastAngle < INVALID_ANGLE)
            mAngle += gap;
          mLastAngle = angle;

          if (mListener != null)
            mListener.OnRotation(mAngle);
        }
        else if (mMoveValid)
        {
          if (mListener != null)
            mListener.OnMove(mMoveX - event.getRawX(), event.getRawY() - mMoveY);
          mMoveX = event.getRawX();
          mMoveY = event.getRawY();
        }
        break;
      case MotionEvent.ACTION_UP:
        ptrID1 = INVALID_POINTER_ID;
        mLastAngle = INVALID_ANGLE + 1;
        mMoveValid = false;
        break;
      case MotionEvent.ACTION_POINTER_UP:
        ptrID2 = INVALID_POINTER_ID;
        mLastAngle = INVALID_ANGLE + 1;
        mMoveValid = false;
        break;
      case MotionEvent.ACTION_CANCEL:
        ptrID1 = INVALID_POINTER_ID;
        ptrID2 = INVALID_POINTER_ID;
        mLastAngle = INVALID_ANGLE + 1;
        mMoveValid = false;
        break;
    }
    return true;
  }

  private float angleBetweenLines(float fX, float fY, float sX, float sY, float nfX, float nfY, float nsX, float nsY)
  {
    float angle1 = (float) Math.atan2((fY - sY), (fX - sX));
    float angle2 = (float) Math.atan2((nfY - nsY), (nfX - nsX));

    float angle = ((float) Math.toDegrees(angle1 - angle2)) % 360;
    if (angle < -180.f) angle += 360.0f;
    if (angle > 180.f) angle -= 360.0f;
    return angle;
  }

  public interface GestureListener
  {
    void OnMove(float dx, float dy);

    void OnRotation(float angle);

    void OnZoom(float diff);
  }
}