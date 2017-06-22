package com.lvonasek.openconstructor.main;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;

import com.lvonasek.openconstructor.ui.AbstractActivity;
import com.lvonasek.openconstructor.R;

import java.io.File;
import java.util.ArrayList;

public class Editor extends View implements Button.OnClickListener, View.OnTouchListener
{
  private enum Effect { CONTRAST, GAMMA, SATURATION, TONE, RESET, CLONE, DELETE, MOVE, ROTATE, SCALE }
  private enum Screen { MAIN, COLOR, SELECT, TRANSFORM }
  private enum Status { IDLE, SELECT_OBJECT, SELECT_RECT, UPDATE_COLORS, UPDATE_TRANSFORM }

  private int mAxis;
  private ArrayList<Button> mButtons;
  private AbstractActivity mContext;
  private Effect mEffect;
  private ProgressBar mProgress;
  private Screen mScreen;
  private SeekBar mSeek;
  private Status mStatus;
  private TextView mMsg;
  private Paint mPaint;
  private Rect mRect;

  private boolean mComplete;
  private boolean mInitialized;

  public Editor(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    mInitialized = false;
    mPaint = new Paint();
    mPaint.setColor(0x8080FF80);
    mRect = new Rect();
  }

  public void init(ArrayList<Button> buttons, TextView msg, SeekBar seek, ProgressBar progress, AbstractActivity context)
  {
    for (Button b : buttons) {
      b.setOnClickListener(this);
      b.setOnTouchListener(this);
    }
    mAxis = 1;
    mButtons = buttons;
    mContext = context;
    mMsg = msg;
    mProgress = progress;
    mSeek = seek;
    setMainScreen();

    mComplete = true;
    mInitialized = true;
    mProgress.setVisibility(View.VISIBLE);
    mSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
    {
      @Override
      public void onProgressChanged(SeekBar seekBar, int value, boolean byUser)
      {
        if ((mStatus == Status.UPDATE_COLORS) || (mStatus == Status.UPDATE_TRANSFORM)) {
          value -= 127;
          JNI.previewEffect(mEffect.ordinal(), value, mAxis);
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
        JNI.completeSelection(mComplete);
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

  private void applyTransform()
  {
    final int axis = mAxis;
    mProgress.setVisibility(View.VISIBLE);
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        JNI.applyEffect(mEffect.ordinal(), mSeek.getProgress() - 127, axis);
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

  public boolean initialized() { return mInitialized; }

  public boolean movingLocked()
  {
    return mStatus != Status.IDLE;
  }

  private Rect normalizeRect(Rect input)
  {
    Rect output = new Rect();
    if (input.left > input.right) {
      output.left = input.right;
      output.right = input.left;
    } else {
      output.left = input.left;
      output.right = input.right;
    }

    if (input.top > input.bottom) {
      output.top = input.bottom;
      output.bottom = input.top;
    } else {
      output.top = input.top;
      output.bottom = input.bottom;
    }
    return output;
  }

  @Override
  public void onClick(final View view)
  {
    //axis buttons
    if (view.getId() == R.id.editorX) {
      applyTransform();
      mAxis = 0;
      showSeekBar(true);
    }
    if (view.getId() == R.id.editorY) {
      applyTransform();
      mAxis = 1;
      showSeekBar(true);
    }
    if (view.getId() == R.id.editorZ) {
      applyTransform();
      mAxis = 2;
      showSeekBar(true);
    }

    //main menu
    if (mScreen == Screen.MAIN) {
      if (view.getId() == R.id.editor0)
        save();
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
      if ((mStatus == Status.SELECT_OBJECT) || (mStatus == Status.SELECT_RECT))
        setSelectScreen();
      else if (mStatus == Status.UPDATE_COLORS) {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            JNI.applyEffect(mEffect.ordinal(), mSeek.getProgress() - 127, 0);
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
      else if (mStatus == Status.UPDATE_TRANSFORM) {
        applyTransform();
        setTransformScreen();
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
            JNI.completeSelection(mComplete);
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
        mStatus = Status.SELECT_OBJECT;
      }
      //rect selection
      if (view.getId() == R.id.editor3) {
        showText(R.string.editor_select_rect_desc);
        mStatus = Status.SELECT_RECT;
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
            JNI.multSelection(false);
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
            JNI.multSelection(true);
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
        mStatus = Status.UPDATE_COLORS;
        showSeekBar(false);
      }
      if (view.getId() == R.id.editor2)
      {
        mEffect = Effect.GAMMA;
        mStatus = Status.UPDATE_COLORS;
        showSeekBar(false);
      }
      if (view.getId() == R.id.editor3)
      {
        mEffect = Effect.SATURATION;
        mStatus = Status.UPDATE_COLORS;
        showSeekBar(false);
      }
      if (view.getId() == R.id.editor4)
      {
        mEffect = Effect.TONE;
        mStatus = Status.UPDATE_COLORS;
        showSeekBar(false);
      }
      if (view.getId() == R.id.editor5)
      {
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            JNI.applyEffect(Effect.RESET.ordinal(), 0, 0);
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
            JNI.applyEffect(Effect.CLONE.ordinal(), 0, 0);
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
            JNI.applyEffect(Effect.DELETE.ordinal(), 0, 0);
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
      if (view.getId() == R.id.editor3)
      {
        mEffect = Effect.MOVE;
        mStatus = Status.UPDATE_TRANSFORM;
        showSeekBar(true);
      }
      if (view.getId() == R.id.editor4)
      {
        mEffect = Effect.ROTATE;
        mStatus = Status.UPDATE_TRANSFORM;
        showSeekBar(true);
      }
      if (view.getId() == R.id.editor5)
      {
        mEffect = Effect.SCALE;
        mStatus = Status.UPDATE_TRANSFORM;
        showSeekBar(false);
      }
    }
  }

  @Override
  protected void onDraw(Canvas c)
  {
    super.onDraw(c);
    c.drawRect(normalizeRect(mRect), mPaint);
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
    mButtons.get(6).setVisibility(View.GONE);
    mButtons.get(7).setVisibility(View.GONE);
    mButtons.get(8).setVisibility(View.GONE);
    mButtons.get(0).setBackgroundResource(R.drawable.ic_back_small);
  }

  private void save() {
    //filename dialog
    AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
    builder.setTitle(mContext.getString(R.string.enter_filename));
    final EditText input = new EditText(mContext);
    builder.setView(input);
    builder.setPositiveButton(mContext.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which) {
        //delete old during overwrite
        File file = new File(AbstractActivity.getPath(), input.getText().toString() + AbstractActivity.FILE_EXT[0]);
        try {
          if (file.exists())
            for(String s : AbstractActivity.getObjResources(file))
              if (new File(AbstractActivity.getPath(), s).delete())
                Log.d(AbstractActivity.TAG, "File " + s + " deleted");
        } catch(Exception e) {
          e.printStackTrace();
        }
        mProgress.setVisibility(View.VISIBLE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            long timestamp = System.currentTimeMillis();
            final File obj = new File(AbstractActivity.getTempPath(), timestamp + AbstractActivity.FILE_EXT[0]);
            JNI.saveWithTextures(obj.getAbsolutePath());
            for(String s : AbstractActivity.getObjResources(obj.getAbsoluteFile()))
              if (new File(AbstractActivity.getTempPath(), s).renameTo(new File(AbstractActivity.getPath(), s)))
                Log.d(AbstractActivity.TAG, "File " + s + " saved");
            final File file2save = new File(AbstractActivity.getPath(), input.getText().toString() + AbstractActivity.FILE_EXT[0]);
            if (obj.renameTo(file2save))
              Log.d(AbstractActivity.TAG, "Obj file " + file2save.toString() + " saved.");
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
        dialog.cancel();
      }
    });
    builder.setNegativeButton(mContext.getString(android.R.string.cancel), null);
    builder.create().show();
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
    mButtons.get(3).setText(mContext.getString(R.string.editor_select_rect));
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

  private void showSeekBar(boolean axes)
  {
    for (Button b : mButtons)
      b.setVisibility(View.GONE);
    mButtons.get(0).setVisibility(View.VISIBLE);
    if (axes)
      updateAxisButtons();
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

  public void touchEvent(final MotionEvent event)
  {
    if (mStatus == Status.SELECT_OBJECT) {
      mProgress.setVisibility(View.VISIBLE);
      new Thread(new Runnable()
      {
        @Override
        public void run()
        {
          JNI.applySelect(event.getX(), getHeight() - event.getY(), false);
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
      mStatus = Status.IDLE;
      setSelectScreen();
    }
    if (mStatus == Status.SELECT_RECT)
    {
      if (event.getAction() == MotionEvent.ACTION_DOWN) {
        mRect.left = (int) event.getX();
        mRect.top = (int) event.getY();
        mRect.right = mRect.left;
        mRect.bottom = mRect.top;
      }
      if (event.getAction() == MotionEvent.ACTION_MOVE) {
        mRect.right = (int) event.getX();
        mRect.bottom = (int) event.getY();
      }
      if (event.getAction() == MotionEvent.ACTION_UP) {
        Rect rect = normalizeRect(mRect);
        rect.top = getHeight() - rect.top;
        rect.bottom = getHeight() - rect.bottom;
        JNI.rectSelection(rect.left, rect.bottom, rect.right, rect.top);
        mRect = new Rect();
      }
      postInvalidate();
    }
  }

  private void updateAxisButtons()
  {
    mButtons.get(6).setVisibility(View.VISIBLE);
    mButtons.get(7).setVisibility(View.VISIBLE);
    mButtons.get(8).setVisibility(View.VISIBLE);
    mButtons.get(6).setText("X");
    mButtons.get(7).setText("Y");
    mButtons.get(8).setText("Z");
    mButtons.get(6).setTextColor(mAxis == 0 ? Color.BLACK : Color.WHITE);
    mButtons.get(7).setTextColor(mAxis == 1 ? Color.BLACK : Color.WHITE);
    mButtons.get(8).setTextColor(mAxis == 2 ? Color.BLACK : Color.WHITE);
    mButtons.get(6).setBackgroundColor(mAxis == 0 ? Color.WHITE : Color.TRANSPARENT);
    mButtons.get(7).setBackgroundColor(mAxis == 1 ? Color.WHITE : Color.TRANSPARENT);
    mButtons.get(8).setBackgroundColor(mAxis == 2 ? Color.WHITE : Color.TRANSPARENT);
  }
}
