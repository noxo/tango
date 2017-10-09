package com.lvonasek.daydreamOBJ;

import android.content.Context;
import android.content.res.AssetManager;
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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Scanner;

public class SelectorView extends View
{
  private static final String DEMO_MATERIAL = "1491804160934.mtl";
  private static final String DEMO_MODEL = "1491804160934.obj";
  private static final String DEMO_TEXTURE = "1491804160934_0.png";
  private static final String DEMO_DIR = "/data/data/com.lvonasek.daydreamOBJ";
  private static final String MODEL_DIR = "/mnt/sdcard/Models";
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
    previews.put(DEMO_MODEL, BitmapFactory.decodeResource(getResources(), R.drawable.ic_demo));

    //get models
    File dir = new File(MODEL_DIR);
    if (dir.exists() && dir.isDirectory()) {
      for (String s : dir.list())
        if (s.toLowerCase().endsWith(".obj")) {
          models.add(s);
          previews.put(s, loadThumbmail(new File(MODEL_DIR, s)));
        }
      Collections.sort(models);
    }
  }

  @Override
  protected void onDraw(Canvas c)
  {
    super.onDraw(c);
    if (!active)
      return;

    //thumbnail
    if (models.isEmpty())
      drawIcon(c, previews.get(DEMO_MODEL));
    else
      drawIcon(c, previews.get(models.get(index)));


    //text style
    int y = (int) (c.getHeight() * 0.55f);
    paint.setTextAlign(Paint.Align.CENTER);
    paint.setTextSize(c.getHeight() / 40);
    paint.setColor(Color.argb(128, 128, 128, 128));
    c.drawRect(0, y, c.getWidth(), y + 2.5f * paint.getTextSize(), paint);
    paint.setColor(Color.WHITE);

    if (models.isEmpty())
    {
      //info text
      y += paint.getTextSize();
      String text = getResources().getString(R.string.storage);
      c.drawText(text, c.getWidth() / 4, y, paint);
      c.drawText(text, 3 * c.getWidth() / 4, y, paint);
      y += paint.getTextSize();
      text = getResources().getString(R.string.no_models);
      c.drawText(text, c.getWidth() / 4, y, paint);
      c.drawText(text, 3 * c.getWidth() / 4, y, paint);
    }
    else
    {
      //title
      y += paint.getTextSize() * 1.5f;
      String model = models.get(index);
      c.drawText(model, c.getWidth() / 4, y, paint);
      c.drawText(model, 3 * c.getWidth() / 4, y, paint);
    }
  }

  private void drawIcon(Canvas c, Bitmap b)
  {
    int s = c.getWidth() / 8;
    Rect src = new Rect();
    src.left = 0;
    src.top = 0;
    src.right = b.getWidth();
    src.bottom = b.getHeight();
    Rect dst = new Rect();
    dst.left = c.getWidth() / 4 - s / 2;
    dst.top = (int)(c.getHeight() * 0.55f) - s / 2;
    dst.right = dst.left + s;
    dst.bottom = dst.top + s;
    c.drawBitmap(b, src, dst, paint);
    dst.left += c.getWidth() / 2;
    dst.right += c.getWidth() / 2;
    c.drawBitmap(b, src, dst, paint);
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
          Bitmap b = BitmapFactory.decodeFile(new File(MODEL_DIR, mtl + ".png").getAbsolutePath());
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
    {
      File model = new File(DEMO_DIR, DEMO_MODEL);
      if (!model.exists())
      {
        copyAsset(instance.getContext(), DEMO_MATERIAL, DEMO_DIR);
        copyAsset(instance.getContext(), DEMO_MODEL, DEMO_DIR);
        copyAsset(instance.getContext(), DEMO_TEXTURE, DEMO_DIR);
      }
      return model.getAbsolutePath();
    }
    return new File(MODEL_DIR, instance.models.get(index)).getAbsolutePath();
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


  private static void copyAsset(Context ctx, String file, String destdir)
  {
    AssetManager assetManager = ctx.getAssets();
    new File(destdir).mkdirs();

    InputStream in;
    OutputStream out;

    try
    {
      in = assetManager.open(file);
      out = new FileOutputStream(destdir + "/" + file);
      copyFile(in, out);
      in.close();
      out.flush();
      out.close();
    } catch (IOException e)
    {
      e.printStackTrace();
    }
  }

  private static void copyFile(InputStream in, OutputStream out) throws IOException
  {
    byte[] buffer = new byte[1024];
    int read;
    while ((read = in.read(buffer)) != -1)
    {
      out.write(buffer, 0, read);
    }
    out.close();
  }
}
