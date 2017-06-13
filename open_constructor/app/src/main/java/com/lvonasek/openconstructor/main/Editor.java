package com.lvonasek.openconstructor.main;

import android.app.Activity;
import android.graphics.Color;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;

import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.TangoJNINative;

import java.util.ArrayList;

public class Editor implements Button.OnClickListener, View.OnTouchListener
{
  private enum Effect { CONTRAST, GAMMA, SATURATION, TONE, RESET, CLONE, DELETE, MOVE, ROTATE, SCALE }
  private enum Screen { MAIN, COLOR, SELECT, TRANSFORM }
  private enum Status { IDLE, UPDATING_COLORS, WAITING_SELECTION_OBJECTS, WAITING_SELECTION_TRIANGLES }

  private ArrayList<Button> mButtons;
  private Activity mContext;
  private Effect mEffect;
  private ProgressBar mProgress;
  private Screen mScreen;
  private SeekBar mSeek;
  private Status mStatus;
  private TextView mMsg;

  private boolean mComplete;

  public Editor(ArrayList<Button> buttons, TextView msg, SeekBar seek, ProgressBar progress, Activity context)
  {
    for (Button b : buttons) {
      b.setOnClickListener(this);
      b.setOnTouchListener(this);
    }
    mButtons = buttons;
    mContext = context;
    mMsg = msg;
    mProgress = progress;
    mSeek = seek;
    setMainScreen();

    mComplete = true;
    mProgress.setVisibility(View.VISIBLE);
    mSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
    {
      @Override
      public void onProgressChanged(SeekBar seekBar, int value, boolean byUser)
      {
        if (byUser && (mStatus == Status.UPDATING_COLORS)) {
          value -= 127;
          TangoJNINative.previewEffect(mEffect.ordinal(), value);
        }
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
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        TangoJNINative.completeSelection(mComplete);
        mContext.runOnUiThread(new Runnable()
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

  public boolean movingLocked()
  {
    return (mStatus == Status.WAITING_SELECTION_OBJECTS) ||(mStatus == Status.WAITING_SELECTION_TRIANGLES);
  }

  @Override
  public void onClick(final View view)
  {
    //main menu
    if (mScreen == Screen.MAIN) {
      if (view.getId() == R.id.editor0)
      {
        //TODO:implement saving
      }
      if (view.getId() == R.id.editor1)
        setSelectScreen();
      if (view.getId() == R.id.editor2)
        setColorScreen();
      if (view.getId() == R.id.editor3)
        setTransformScreen();
      return;
    }
    //back button
    else if (view.getId() == R.id.editor0) {
      if ((mStatus == Status.WAITING_SELECTION_OBJECTS) || (mStatus == Status.WAITING_SELECTION_TRIANGLES))
        setSelectScreen();
      else if (mStatus == Status.UPDATING_COLORS) {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.applyEffect(mEffect.ordinal(), mSeek.getProgress() - 127);
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
        setColorScreen();
      }
      else
        setMainScreen();
    }

    //selecting objects
    if (mScreen == Screen.SELECT) {
      //select all/none
      if (view.getId() == R.id.editor1)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            mComplete = !mComplete;
            TangoJNINative.completeSelection(mComplete);
            mContext.runOnUiThread(new Runnable()
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
      //select object
      if (view.getId() == R.id.editor2) {
        showText(R.string.editor_select_object_desc);
        mStatus = Status.WAITING_SELECTION_OBJECTS;
      }
      //triangle selection
      if (view.getId() == R.id.editor3) {
        showText(R.string.editor_select_triangle_desc);
        mStatus = Status.WAITING_SELECTION_TRIANGLES;
      }
      //select less
      if (view.getId() == R.id.editor4)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.multSelection(false);
            mContext.runOnUiThread(new Runnable()
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
      //select more
      if (view.getId() == R.id.editor5)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.multSelection(true);
            mContext.runOnUiThread(new Runnable()
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
    }

    //color editing
    if (mScreen == Screen.COLOR) {
      if (view.getId() == R.id.editor1)
      {
        mEffect = Effect.CONTRAST;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor2)
      {
        mEffect = Effect.GAMMA;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor3)
      {
        mEffect = Effect.SATURATION;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor4)
      {
        mEffect = Effect.TONE;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor5)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.applyEffect(Effect.RESET.ordinal(), 0);
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

    // transforming objects
    if (mScreen == Screen.TRANSFORM) {
      if (view.getId() == R.id.editor1)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.applyEffect(Effect.CLONE.ordinal(), 0);
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
      if (view.getId() == R.id.editor2)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.applyEffect(Effect.DELETE.ordinal(), 0);
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
      //TODO:implement move/rotate/scale
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
    mSeek.setVisibility(View.GONE);
    mStatus = Status.IDLE;
    for (Button b : mButtons)
    {
      b.setText("");
      b.setVisibility(View.VISIBLE);
    }
    mButtons.get(0).setBackgroundResource(R.drawable.ic_back_small);
  }

  private void setMainScreen()
  {
    initButtons();
    mButtons.get(0).setBackgroundResource(R.drawable.ic_save_small);
    mButtons.get(1).setText(mContext.getString(R.string.editor_main_select));
    mButtons.get(2).setText(mContext.getString(R.string.editor_main_colors));
    mButtons.get(3).setText(mContext.getString(R.string.editor_main_transform));
    mScreen = Screen.MAIN;
  }

  private void setColorScreen()
  {
    initButtons();
    mButtons.get(1).setText(mContext.getString(R.string.editor_colors_contrast));
    mButtons.get(2).setText(mContext.getString(R.string.editor_colors_gamma));
    mButtons.get(3).setText(mContext.getString(R.string.editor_colors_saturation));
    mButtons.get(4).setText(mContext.getString(R.string.editor_colors_tone));
    mButtons.get(5).setText(mContext.getString(R.string.editor_colors_reset));
    mScreen = Screen.COLOR;
  }

  private void setSelectScreen()
  {
    initButtons();
    mButtons.get(1).setText(mContext.getString(R.string.editor_select_all));
    mButtons.get(2).setText(mContext.getString(R.string.editor_select_object));
    mButtons.get(3).setText(mContext.getString(R.string.editor_select_triangle));
    mButtons.get(4).setText(mContext.getString(R.string.editor_select_less));
    mButtons.get(5).setText(mContext.getString(R.string.editor_select_more));
    mScreen = Screen.SELECT;
  }

  private void setTransformScreen()
  {
    initButtons();
    mButtons.get(1).setText(mContext.getString(R.string.editor_transform_clone));
    mButtons.get(2).setText(mContext.getString(R.string.editor_transform_delete));
    mButtons.get(3).setText(mContext.getString(R.string.editor_transform_move));
    mButtons.get(4).setText(mContext.getString(R.string.editor_transform_rotate));
    mButtons.get(5).setText(mContext.getString(R.string.editor_transform_scale));
    mScreen = Screen.TRANSFORM;
  }

  private void showSeekBar()
  {
    for (Button b : mButtons)
      b.setVisibility(View.GONE);
    mButtons.get(0).setVisibility(View.VISIBLE);
    mSeek.setMax(255);
    mSeek.setProgress(127);
    mSeek.setVisibility(View.VISIBLE);
  }

  private void showText(int resId)
  {
    for (Button b : mButtons)
      b.setVisibility(View.GONE);
    mButtons.get(0).setVisibility(View.VISIBLE);
    mMsg.setText(mContext.getString(resId));
    mMsg.setVisibility(View.VISIBLE);
  }

  public void touchEvent(float x, float y)
  {
    if (mStatus == Status.WAITING_SELECTION_OBJECTS) {
      TangoJNINative.applySelect(x, y, false);
      mStatus = Status.IDLE;
      setSelectScreen();
    }
    if (mStatus == Status.WAITING_SELECTION_TRIANGLES)
      TangoJNINative.applySelect(x, y, true);
  }
}
