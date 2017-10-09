package com.lvonasek.daydreamOBJ;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Scanner;

public class SelectorView extends View
{
  private static final String DIR = "/mnt/sdcard/Models";
  public static boolean active = false;
  public static int index = 0;
  private static SelectorView instance;

  private ArrayList<String> models;
  private HashMap<String, Bitmap> previews;
  private Paint paint;

  public SelectorView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    index = 0;
    instance = this;
    models = new ArrayList<>();
    paint = new Paint();
    previews = new HashMap<>();

    //get models
    File dir = new File(DIR);
    if (dir.exists() && dir.isDirectory()) {
      for (String s : dir.list())
        if (s.toLowerCase().endsWith(".obj")) {
          models.add(s);
          previews.put(s, loadThumbmail(new File(DIR, s)));
        }
      Collections.sort(models);
    }
  }

  @Override
  protected void onDraw(Canvas c)
  {
    super.onDraw(c);
    paint.setColor(Color.WHITE);
    paint.setTextAlign(Paint.Align.CENTER);
    paint.setTextSize(c.getHeight() / 25);
    if (models.isEmpty())
    {
      int y = (int) (c.getHeight() * 0.8f);
      String text = getResources().getString(R.string.storage);
      c.drawText(text, c.getWidth() / 4, y, paint);
      c.drawText(text, 3 * c.getWidth() / 4, y, paint);
      y += paint.getTextSize();
      text = getResources().getString(R.string.no_models);
      c.drawText(text, c.getWidth() / 4, y, paint);
      c.drawText(text, 3 * c.getWidth() / 4, y, paint);
    }
    else if (active)
    {
      //title
      String model = models.get(index);
      c.drawText(model, c.getWidth() / 4, c.getHeight() * 0.8f, paint);
      c.drawText(model, 3 * c.getWidth() / 4, c.getHeight() * 0.8f, paint);

      //icon
      int s = c.getWidth() / 8;
      Bitmap b = previews.get(model);
      Rect src = new Rect();
      src.left = 0;
      src.top = 0;
      src.right = b.getWidth();
      src.bottom = b.getHeight();
      Rect dst = new Rect();
      dst.left = c.getWidth() / 4 - s / 2;
      dst.top = (int)(c.getHeight() * 0.6f) - s / 2;
      dst.right = dst.left + s;
      dst.bottom = dst.top + s;
      c.drawBitmap(b, src, dst, paint);
      dst.left += c.getWidth() / 2;
      dst.right += c.getWidth() / 2;
      c.drawBitmap(b, src, dst, paint);
    }
  }

  private Bitmap loadThumbmail(File file)
  {
    try
    {
      Scanner sc = new Scanner(new FileInputStream(file.getAbsolutePath()));
      while (sc.hasNext()) {
        String line = sc.nextLine();
        if (line.contains("mtllib")) {
          String mtl = line.substring(line.indexOf("mtllib") + 7);
          Bitmap b = BitmapFactory.decodeFile(new File(DIR, mtl + ".png").getAbsolutePath());
          if (b != null)
            return b;
        }
      }
    } catch (FileNotFoundException e)
    {
      e.printStackTrace();
    }
    return BitmapFactory.decodeResource(getResources(), R.drawable.ic_model_icon);
  }

  public static String getSelected()
  {
    if (instance.models.isEmpty())
      return null;
    return new File(DIR, instance.models.get(index)).getAbsolutePath();
  }

  public static void activate(boolean on)
  {
    active = on;
    instance.postInvalidate();
  }

  public static void update(boolean next)
  {
    if (instance != null)
    {
      if (next)
        index++;
      else
        index--;
      if (index < 0)
        index = instance.models.size() - 1;
      if (index >= instance.models.size())
        index = 0;
      instance.postInvalidate();
    }
  }
}
