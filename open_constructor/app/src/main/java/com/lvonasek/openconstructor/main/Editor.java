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
  private enum Colors { CONTRAST, GAMMA, SATURATION, TONE }
  private enum Screen { MAIN, COLOR, SELECT }
  private enum Status { IDLE, UPDATING_COLORS, WAITING_SELECTION_POINT }

  private ArrayList<Button> mButtons;
  private Activity mContext;
  private Colors mColors;
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
          //TODO:implement preview
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
      return;
    }
    //back button
    else if (view.getId() == R.id.editor0) {
      if (mStatus == Status.WAITING_SELECTION_POINT)
        setSelectScreen();
      else if (mStatus == Status.UPDATING_COLORS) {
        //TODO:implement updating colors
        setColorScreen();
      }
      else
        setMainScreen();
    }

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
      //area selection
      if (view.getId() == R.id.editor2) {
        //TODO:implement
      }
      //select object
      if (view.getId() == R.id.editor3) {
        showText(R.string.editor_select_object_desc);
        mStatus = Status.WAITING_SELECTION_POINT;
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
        mColors = Colors.CONTRAST;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor2)
      {
        mColors = Colors.GAMMA;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor3)
      {
        mColors = Colors.SATURATION;
        mStatus = Status.UPDATING_COLORS;
        showSeekBar();
      }
      if (view.getId() == R.id.editor4)
      {
        mColors = Colors.TONE;
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
            //TODO:implement resetting colors
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
    mButtons.get(2).setText(mContext.getString(R.string.editor_select_area));
    mButtons.get(3).setText(mContext.getString(R.string.editor_select_object));
    mButtons.get(4).setText(mContext.getString(R.string.editor_select_less));
    mButtons.get(5).setText(mContext.getString(R.string.editor_select_more));
    mScreen = Screen.SELECT;
  }

  private void showSeekBar()
  {
    for (Button b : mButtons)
      b.setVisibility(View.GONE);
    mButtons.get(0).setVisibility(View.VISIBLE);
    mSeek.setMax(510);
    mSeek.setProgress(256);
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
    if (mStatus == Status.WAITING_SELECTION_POINT)
    {
      TangoJNINative.applySelect(x, y);
      mStatus = Status.IDLE;
      setSelectScreen();
    }
  }
}
