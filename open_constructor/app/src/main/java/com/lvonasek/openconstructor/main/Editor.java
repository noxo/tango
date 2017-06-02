package com.lvonasek.openconstructor.main;

import android.app.Activity;
import android.graphics.Color;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.TangoJNINative;

import java.util.ArrayList;

public class Editor implements Button.OnClickListener, View.OnTouchListener
{
  private enum Effect { GAMMA, SATURATION }
  private enum Screen { MAIN, COLOR, SELECT }
  private enum Status { IDLE, WAITING_SELECTION_POINT }

  private ArrayList<Button> mButtons;
  private Activity mContext;
  private ProgressBar mProgress;
  private Screen mScreen;
  private Status mStatus;
  private TextView mMsg;

  public Editor(ArrayList<Button> buttons, TextView msg, ProgressBar progress, Activity context)
  {
    for (Button b : buttons) {
      b.setOnClickListener(this);
      b.setOnTouchListener(this);
    }
    mButtons = buttons;
    mContext = context;
    mMsg = msg;
    mProgress = progress;
    mStatus = Status.IDLE;
    setMainScreen();
  }

  @Override
  public void onClick(final View view)
  {
    if (mScreen == Screen.MAIN) {
      if (view.getId() == R.id.editor1)
        setSelectScreen();
      if (view.getId() == R.id.editor2)
        setColorScreen();
      return;
    } else if (view.getId() == R.id.editor1)
      setMainScreen();

    if (mScreen == Screen.SELECT) {
      //TODO:
      if (view.getId() == R.id.editor3) {
        showText(R.string.editor_select_object_desc);
        mStatus = Status.WAITING_SELECTION_POINT;
      }
      if (view.getId() == R.id.editor4)
        TangoJNINative.multSelection(false);
      if (view.getId() == R.id.editor5)
        TangoJNINative.multSelection(true);
    }
    //color editing
    if (mScreen == Screen.COLOR) {
      mProgress.setVisibility(View.VISIBLE);
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          if (view.getId() == R.id.editor2)
            TangoJNINative.applyEffect(Effect.GAMMA.ordinal(), 16);
          if (view.getId() == R.id.editor3)
            TangoJNINative.applyEffect(Effect.GAMMA.ordinal(), -16);
          if (view.getId() == R.id.editor4)
            TangoJNINative.applyEffect(Effect.SATURATION.ordinal(), 0.3f);
          if (view.getId() == R.id.editor5)
            TangoJNINative.applyEffect(Effect.SATURATION.ordinal(), -0.3f);
          mContext.runOnUiThread(new Runnable()
          {
            @Override
            public void run()
            {
              mProgress.setVisibility(View.INVISIBLE);
            }
          });
        }
      }).start();
    }
  }

  @Override
  public boolean onTouch(View view, MotionEvent motionEvent)
  {
    if (view instanceof Button) {
      Button b = (Button) view;
      if (motionEvent.getAction() == MotionEvent.ACTION_DOWN)
        b.setTextColor(Color.YELLOW);
      if (motionEvent.getAction() == MotionEvent.ACTION_UP)
        b.setTextColor(Color.WHITE);
    }
    return false;
  }

  private void initButtons()
  {
    mMsg.setVisibility(View.GONE);
    for (Button b : mButtons)
    {
      b.setText("");
      b.setVisibility(View.VISIBLE);
    }
  }

  private void setMainScreen()
  {
    initButtons();
    mButtons.get(0).setText(mContext.getString(R.string.editor_main_select));
    mButtons.get(1).setText(mContext.getString(R.string.editor_main_colors));
    mScreen = Screen.MAIN;
  }

  private void setColorScreen()
  {
    initButtons();
    mButtons.get(0).setText(mContext.getString(R.string.editor_back));
    mButtons.get(1).setText(mContext.getString(R.string.editor_colors_gamma_add));
    mButtons.get(2).setText(mContext.getString(R.string.editor_colors_gamma_sub));
    mButtons.get(3).setText(mContext.getString(R.string.editor_colors_saturation_add));
    mButtons.get(4).setText(mContext.getString(R.string.editor_colors_saturation_sub));
    mScreen = Screen.COLOR;
  }

  private void setSelectScreen()
  {
    initButtons();
    mButtons.get(0).setText(mContext.getString(R.string.editor_back));
    mButtons.get(1).setText(mContext.getString(R.string.editor_select_area));
    mButtons.get(2).setText(mContext.getString(R.string.editor_select_object));
    mButtons.get(3).setText(mContext.getString(R.string.editor_select_less));
    mButtons.get(4).setText(mContext.getString(R.string.editor_select_more));
    mScreen = Screen.SELECT;
  }

  private void showText(int resId)
  {
    for (Button b : mButtons)
      b.setVisibility(View.GONE);
    mMsg.setText(mContext.getString(resId));
    mMsg.setVisibility(View.VISIBLE);
  }

  public void touchEvent(float x, float y)
  {
    if (mStatus == Status.WAITING_SELECTION_POINT)
    {
      TangoJNINative.applySelect(x, y);
      mStatus = Status.IDLE;
      setSelectScreen();
    }
  }
}
