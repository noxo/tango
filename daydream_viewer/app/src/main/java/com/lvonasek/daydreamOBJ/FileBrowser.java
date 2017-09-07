package com.lvonasek.daydreamOBJ;

import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.SparseIntArray;
import android.view.View;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;

public class FileBrowser extends View
{
  private Context mContext;
  private Paint mPaint = new Paint();
  private static File dir = new File("/mnt/sdcard/");
  private static ArrayList<File> files = new ArrayList<>();
  private static int pointer = 0;
  private static long timestamp = 0;
  private static boolean waitingForController = true;

  public FileBrowser(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    mContext = context;
    synchronized (this)
    {
      File newdir = new File(dir, "Models");
      if (newdir.exists())
        dir = newdir;
      updateDirInfo();
    }
  }

  @Override
  protected synchronized void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);
    mPaint.setColor(Color.BLACK);
    mPaint.setStyle(Paint.Style.FILL);
    canvas.drawRect(0, 0, getWidth(), getHeight(), mPaint);
    if (EntryActivity.filename != null)
      return;
    if (files.isEmpty())
    {
      try {
        updateDirInfo();
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
    int size = getHeight() / 32;
    if (waitingForController)
    {
      mPaint.setColor(Color.YELLOW);
      mPaint.setTextSize(size);
      mPaint.setTextAlign(Paint.Align.CENTER);
      canvas.drawText(mContext.getString(R.string.no_controller), canvas.getWidth() / 2, canvas.getHeight() / 2, mPaint);
      canvas.drawText(mContext.getString(R.string.no_daydream), canvas.getWidth() / 2, size, mPaint);
    } else {
      int index = 0;
      int start = pointer / 10 * 10;
      int end = Math.min(start + 10, files.size());
      for (int i = start; i < end; i++)
      {
        mPaint.setColor(pointer == i ? Color.YELLOW : Color.WHITE);
        mPaint.setTextSize(size);
        mPaint.setTextAlign(Paint.Align.CENTER);
        int y = canvas.getHeight() * 40 / 100 + index * size;
        String name = files.get(i) == null ? "[Parent directory]" : files.get(i).getName();
        if ((files.get(i) != null) && files.get(i).isDirectory())
          name = name.toUpperCase();
        canvas.drawText(name, canvas.getWidth() / 2, y, mPaint);
        index++;
      }
    }
    postInvalidate();
  }

  public static synchronized void onControllerData(SparseIntArray status)
  {
    if (timestamp + 250 > System.currentTimeMillis())
      return;
    waitingForController = false;

    if (status.get(DaydreamController.BTN_CLICK) > 0)
    {
      timestamp = System.currentTimeMillis();
      File current = dir;
      if (files.get(pointer) == null)
      {
        try {
          dir = dir.getParentFile();
          updateDirInfo();
        } catch(Exception e) {
          dir = current;
          updateDirInfo();
        }
        pointer = 0;
      }
      else if (files.get(pointer).isDirectory())
      {
        try {
          dir = files.get(pointer);
          updateDirInfo();
        } catch(Exception e) {
          dir = current;
          updateDirInfo();
        }
        pointer = 0;
      } else {
        EntryActivity.filename = files.get(pointer).getAbsolutePath();
        try {
          Thread.sleep(500);
        } catch (Exception e)
        {
          e.printStackTrace();
        }
        EntryActivity.activity.startActivity(new Intent(EntryActivity.activity, MainActivity.class));
      }
    }

    if (status.get(DaydreamController.SWP_Y) > 50)
    {
      pointer++;
      timestamp = System.currentTimeMillis();
    }
    if (status.get(DaydreamController.SWP_Y) < -50)
    {
      pointer--;
      timestamp = System.currentTimeMillis();
    }

    if (pointer < 0)
      pointer = 0;
    if (pointer >= files.size())
      pointer = files.size() - 1;
  }

  private static void updateDirInfo()
  {
    try {
      files.clear();
      files.addAll(Arrays.asList(dir.listFiles(new FileFilter()
      {
        @Override
        public boolean accept(File file)
        {
          if (file.isDirectory())
            return true;
          String s = file.getName().toLowerCase();
          return s.substring(s.length() - 4).contains(".obj");
        }
      })));
      Collections.sort(files, new Comparator<File>() {
        public int compare(File a, File b) {
          if (a.isDirectory() != b.isDirectory())
            return a.isDirectory() ? 1 : -1;
          else
            return a.getName().compareTo(b.getName());
        }
      });
      if (dir.getAbsolutePath().length() > "/mnt/sdcard".length())
        files.add(0, null);
    } catch(Exception e) {
      e.printStackTrace();
    }
  }
}
