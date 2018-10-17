package com.lvonasek.openconstructor.main;

import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.TextView;

import com.lvonasek.daydreamOBJ.CardboardActivity;
import com.lvonasek.daydreamOBJ.DaydreamActivity;
import com.lvonasek.openconstructor.ui.AbstractActivity;
import com.lvonasek.openconstructor.ui.Service;
import com.lvonasek.openconstructor.R;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Scanner;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.opengles.GL10;

public class OpenConstructor extends AbstractActivity implements View.OnClickListener,
        GLSurfaceView.Renderer, Runnable {

  private ActivityManager mActivityManager;
  private ActivityManager.MemoryInfo mMemoryInfo;
  private ProgressBar mProgress;
  private GLSurfaceView mGLView;
  private String mToLoad, mOpenedFile;
  private boolean m3drRunning = false;
  private boolean mViewMode = false;
  private boolean mPostprocess = false;
  private boolean mRunning = false;

  private LinearLayout mLayoutRec;
  private Button mToggleButton;
  private LinearLayout mLayoutInfo;
  private TextView mInfoLeft;
  private TextView mInfoRight;
  private TextView mInfoLog;
  private View mBattery;
  private Button mCardboardButton;
  private Button mEditorButton;
  private Button mModeButton;
  private Button mThumbnailButton;
  private int mRes = 3;

  private LinearLayout mLayoutEditor;
  private ArrayList<Button> mEditorAction;
  private TextView mEditorMsg;
  private SeekBar mEditorSeek;
  private Editor mEditor;

  private GestureDetector mGestureDetector;
  private boolean mTopMode;
  private float mMoveX = 0;
  private float mMoveY = 0;
  private float mMoveZ = 0;
  private float mPitch = 0;
  private float mYawM = 0;
  private float mYawR = 0;

  // Tango Service connection.
  boolean mInitialised = false;
  boolean mTangoBinded = false;
  ServiceConnection mTangoServiceConnection = new ServiceConnection() {
      public void onServiceConnected(ComponentName name, final IBinder srv) {
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            double res    = mRes * 0.01;
            double dmin   = 0.6f;
            double dmax   = mRes * 1.5;
            boolean clear = isSpaceClearingOn(OpenConstructor.this);
            int noise     = isNoiseFilterOn(OpenConstructor.this) ? 9 : 0;
            boolean holes = isPoissonReconstructionOn(OpenConstructor.this);
            boolean pose  = isPoseCorrectionOn(OpenConstructor.this);
            boolean sharp = isSharpPhotosOn(OpenConstructor.this);
            boolean land  = isLandscape(OpenConstructor.this);
            boolean asus  = Build.MANUFACTURER.toUpperCase().contains("ASUS");;

            if (android.os.Build.DEVICE.toLowerCase().startsWith("yellowstone"))
              land = !land;

            if (mRes == 0) {
              res = 0.005;
              dmax = 1.0;
            }

            //pause/resume
            final boolean continueScanning = mRes == Integer.MIN_VALUE;
            mPostprocess = mRes == Integer.MAX_VALUE;
            File config = new File(getTempPath(), "config.txt");
            if (continueScanning || mPostprocess) {
              m3drRunning = false;
              try
              {
                Scanner sc = new Scanner(new FileInputStream(config.getAbsolutePath()));
                mRes = sc.nextInt();
                res = Double.parseDouble(sc.next());
                dmin = Double.parseDouble(sc.next());
                dmax = Double.parseDouble(sc.next());
                noise = sc.nextInt();
                sc.close();
              } catch (Exception e)
              {
                e.printStackTrace();
              }
            } else {
              m3drRunning = true;
              deleteRecursive(getTempPath());
              getTempPath().mkdirs();
              Exporter.extractRawData(getResources(), R.raw.config, getTempPath());
              try
              {
                FileOutputStream fos = new FileOutputStream(config.getAbsolutePath());
                fos.write((mRes + " " + res + " " + dmin + " " + dmax + " " + noise).getBytes());
                fos.close();
              } catch (Exception e)
              {
                e.printStackTrace();
              }
            }

            mMoveZ = 10;
            String t = getTempPath().getAbsolutePath();
            JNI.onTangoServiceConnected(srv, res, dmin, dmax, noise, land, sharp, holes, clear, pose, asus, t);
            JNI.onToggleButtonClicked(m3drRunning);
            JNI.setView(0, 0, 0, 0, mMoveZ, true);
            final File obj = new File(getPath(), Service.getLink(OpenConstructor.this));
            if (mPostprocess) {
              Service.process(getString(R.string.postprocessing), Service.SERVICE_POSTPROCESS,
                      OpenConstructor.this, new Runnable()
                      {
                        @Override
                        public void run()
                        {
                          mGLView.onPause();
                          finish();
                          JNI.load(obj.getAbsolutePath());
                          JNI.texturize(obj.getAbsolutePath());
                          Service.finish(TEMP_DIRECTORY + "/" + obj.getName());
                        }
                      });
            } else {
              if (continueScanning)
              {
                JNI.load(obj.getAbsolutePath());
              }
              OpenConstructor.this.runOnUiThread(new Runnable()
              {
                @Override
                public void run()
                {
                  mProgress.setVisibility(View.GONE);
                  if (continueScanning)
                    mToggleButton.setBackgroundResource(R.drawable.ic_record);
                }
              });
            }
            mInitialised = true;
          }
        }).start();
      }

      public void onServiceDisconnected(ComponentName name) {
      }
    };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    mActivityManager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
    mMemoryInfo = new ActivityManager.MemoryInfo();

    // Setup UI elements and listeners.
    mLayoutRec = (LinearLayout) findViewById(R.id.layout_rec);
    findViewById(R.id.clear_button).setOnClickListener(this);
    findViewById(R.id.save_button).setOnClickListener(this);
    mToggleButton = (Button) findViewById(R.id.toggle_button);
    mToggleButton.setOnClickListener(this);

    // Recording info
    mCardboardButton = (Button) findViewById(R.id.cardboard_button);
    mModeButton = (Button) findViewById(R.id.mode_button);
    mEditorButton = (Button) findViewById(R.id.editor_button);
    mLayoutInfo = (LinearLayout) findViewById(R.id.layout_info);
    mInfoLeft = (TextView) findViewById(R.id.info_left);
    mInfoRight = (TextView) findViewById(R.id.info_right);
    mInfoLog = (TextView) findViewById(R.id.infolog);
    mBattery = findViewById(R.id.info_battery);
    mThumbnailButton = (Button) findViewById(R.id.thumbnail_button);

    // Editor
    mLayoutEditor = (LinearLayout) findViewById(R.id.layout_editor);
    mEditorMsg = (TextView) findViewById(R.id.editorMsg);
    mEditorSeek = (SeekBar) findViewById(R.id.editorSeek);
    mEditorAction = new ArrayList<>();
    mEditorAction.add((Button) findViewById(R.id.editor0));
    mEditorAction.add((Button) findViewById(R.id.editor1));
    mEditorAction.add((Button) findViewById(R.id.editor2));
    mEditorAction.add((Button) findViewById(R.id.editor3));
    mEditorAction.add((Button) findViewById(R.id.editor4));
    mEditorAction.add((Button) findViewById(R.id.editor5));
    mEditorAction.add((Button) findViewById(R.id.editorX));
    mEditorAction.add((Button) findViewById(R.id.editorY));
    mEditorAction.add((Button) findViewById(R.id.editorZ));
    mEditor = (Editor) findViewById(R.id.editor);

    // OpenGL view where all of the graphics are drawn
    mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(3);
    mGLView.setRenderer(this);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);

    // Touch controller
    mGestureDetector = new GestureDetector(new GestureDetector.GestureListener()
    {
      @Override
      public boolean IsAcceptingRotation() {
        return mTopMode;
      }

      @Override
      public void OnMove(float dx, float dy) {
        float f = getMoveFactor();
        if (mTopMode) {
          //move factor
          f *= Math.max(1.0, mMoveZ);
          //yaw rotation
          double angle = -mYawM - mYawR;
          double fx = dx * f * Math.cos(angle) + dy * f * Math.sin(angle);
          double fy = dx * f * Math.sin(angle) - dy * f * Math.cos(angle);
          //pitch rotation
          mMoveX += dx * f * Math.cos(angle) * Math.cos(mPitch) - fx * Math.sin(mPitch);
          mMoveY += dx * f * Math.sin(angle) * Math.cos(mPitch) - fy * Math.sin(mPitch);
          mMoveZ += dy * f * Math.cos(mPitch);
        } else {
          f *= 2.0f;
          mPitch += dy * f;
          mYawM += -dx * f;
        }
        JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
      }

      @Override
      public void OnRotation(float angle) {
        mYawR = (float) Math.toRadians(-angle);
        JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
      }

      @Override
      public void OnZoom(float diff) {
        if (mViewMode) {
          diff *= 0.25f * Math.max(1.0, mMoveZ);
          double angle = -mYawM - mYawR;
          mMoveX += diff * Math.sin(angle) * Math.cos(mPitch);
          mMoveY -= diff * Math.cos(angle) * Math.cos(mPitch);
          mMoveZ += diff * Math.sin(mPitch);
        } else {
          mMoveZ -= diff;
          if(mMoveZ < 0)
            mMoveZ = 0;
          if(mMoveZ > 10)
            mMoveZ = 10;
        }
        JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
      }
    }, this);

    //open file
    mToLoad = null;
    String filename = getIntent().getStringExtra(FILE_KEY);
    if ((filename != null) && (filename.length() > 0))
    {
      m3drRunning = false;
      try
      {
        if (new File(getPath(), filename).isFile())
          mToLoad = new File(getPath(), filename).toString();
        else
          mToLoad = getModel(filename).toString();
        setViewerMode(mToLoad);
      } catch (Exception e) {
        Log.e(TAG, "Unable to load " + filename);
        e.printStackTrace();
      }
    }
    else
      mRes = getIntent().getIntExtra(RESOLUTION_KEY, 3);
    mProgress.setVisibility(View.VISIBLE);
  }

  @Override
  public void onClick(View v) {
    switch (v.getId()) {
    case R.id.toggle_button:
      m3drRunning = !m3drRunning;
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            JNI.onToggleButtonClicked(m3drRunning);
          }
        });
      break;
    case R.id.clear_button:
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            JNI.onClearButtonClicked();
          }
        });
      break;
    case R.id.save_button:
      finish();
      break;
    }
    mToggleButton.setBackgroundResource(m3drRunning ? R.drawable.ic_pause : R.drawable.ic_record);
  }

  @Override
  protected void onResume() {
    super.onResume();
    mGLView.onResume();

    if (mViewMode) {
      if (mToLoad != null) {
        final String file = "" + mToLoad;
        mOpenedFile = mToLoad;
        mToLoad = null;
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            JNI.load(file);
            JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
            captureBitmap(false);
            OpenConstructor.this.runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                mProgress.setVisibility(View.GONE);
              }
            });
            mInitialised = true;
          }
        }).start();
      }
    } else
    {
      mRunning = true;
      if (mRes != Integer.MAX_VALUE)
        new Thread(this).start();

      if(!mInitialised && !mTangoBinded) {
        TangoInitHelper.bindTangoService(this, mTangoServiceConnection);
        mTangoBinded = true;
      }
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    if (!mPostprocess) {
      if (mTangoBinded) {
        save();
      } else
        System.exit(0);
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mEditor.initialized())
    {
      mEditor.touchEvent(event);
      if (mEditor.movingLocked())
        return true;
    }
    try {
      mGestureDetector.onTouchEvent(event);
    } catch(Exception e) {
      e.printStackTrace();
    }
    return true;
  }

  private float getMoveFactor()
  {
    Display display = getWindowManager().getDefaultDisplay();
    Point size = new Point();
    display.getSize(size);
    return 2.0f / (size.x + size.y);
  }

  private void setViewerMode(final String filename)
  {
    mLayoutRec.setVisibility(View.GONE);
    mLayoutInfo.setVisibility(View.GONE);
    mModeButton.setVisibility(View.VISIBLE);
    mModeButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        if (mEditor.initialized())
          if (mEditor.movingLocked())
            return;
        mTopMode = !mTopMode;
        mModeButton.setBackgroundResource(mTopMode ? R.drawable.ic_mode3d : R.drawable.ic_mode2d);
        float floor = JNI.getFloorLevel(mMoveX, mMoveY, mMoveZ);
        if (floor < -9999)
          floor = 0;
        if (mTopMode) {
          mMoveZ = floor + 10.0f;
          mPitch = (float) Math.toRadians(-90);
        } else {
          mMoveZ = floor + 1.7f; //1.7m as an average human height
          mPitch = 0;
        }
        JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, false);
      }
    });
    mEditorButton.setVisibility(View.VISIBLE);
    mEditorButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        mCardboardButton.setVisibility(View.GONE);
        mThumbnailButton.setVisibility(View.GONE);
        mEditorButton.setVisibility(View.GONE);
        mLayoutEditor.setVisibility(View.VISIBLE);
        setOrientation(true, OpenConstructor.this);
        mEditor.init(mEditorAction, mEditorMsg, mEditorSeek, mProgress, OpenConstructor.this);
      }
    });
    mCardboardButton.setVisibility(View.VISIBLE);
    mCardboardButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        Intent i;
        if (Build.MANUFACTURER.toUpperCase().startsWith("ASUS"))
          i = new Intent(OpenConstructor.this, DaydreamActivity.class);
        else
          i = new Intent(OpenConstructor.this, CardboardActivity.class);
        i.setDataAndType(Uri.parse(filename), "text/plain");
        startActivity(i);
      }
    });
    mThumbnailButton.setVisibility(View.VISIBLE);
    mThumbnailButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        mProgress.setVisibility(View.VISIBLE);
        mThumbnailButton.setVisibility(View.GONE);
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            captureBitmap(true);
            runOnUiThread(new Runnable()
            {
              @Override
              public void run()
              {
                mProgress.setVisibility(View.GONE);
                mThumbnailButton.setVisibility(View.VISIBLE);
              }
            });
          }
        }).start();
      }
    });
    float floor = JNI.getFloorLevel(mMoveX, mMoveY, mMoveZ);
    if (floor < -9999)
      floor = 0;
    mMoveX = 0;
    mMoveY = 0;
    mMoveZ = floor + 10.0f;
    mPitch = (float) Math.toRadians(-90);
    mYawM  = 0;
    mYawR  = 0;
    mTopMode = true;
    mViewMode = true;
    JNI.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, false);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    if (mEditor.initialized())
    {
      final int visibility = mModeButton.getVisibility();
      final int newVisibility = mEditor.movingLocked() ? View.GONE : View.VISIBLE;
      if (visibility != newVisibility) {
        runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            mModeButton.setVisibility(newVisibility);
          }
        });
      }
    }
    JNI.onGlSurfaceDrawFrame();
  }

  @Override
  public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {}

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    JNI.onGlSurfaceChanged(width, height);
  }

  public static int getBatteryPercentage(Context context) {
    IntentFilter iFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
    Intent batteryStatus = context.registerReceiver(null, iFilter);
    int level = batteryStatus != null ? batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1) : -1;
    int scale = batteryStatus != null ? batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1) : -1;
    float batteryPct = level / (float) scale;
    return (int) (batteryPct * 100);
  }

  private void save()
  {
    Service.process(getString(R.string.saving), Service.SERVICE_SAVE, this, new Runnable() {
      @Override
      public void run()
      {
        JNI.onToggleButtonClicked(false);
        mGLView.onPause();
        finish();
        long timestamp = System.currentTimeMillis();
        final File obj = new File(getTempPath(), timestamp + Exporter.FILE_EXT[0]);
        if (JNI.save(obj.getAbsolutePath()))
          Service.finish(TEMP_DIRECTORY + "/" + obj.getName());
        else {
          Service.reset(OpenConstructor.this);
          System.exit(0);
        }
      }
    });
  }

  @Override
  public void run()
  {
    while (mRunning) {
      try
      {
        Thread.sleep(1000);
      } catch (InterruptedException e)
      {
        e.printStackTrace();
      }
      runOnUiThread(new Runnable()
      {
        @Override
        public void run()
        {
          //memory info
          mActivityManager.getMemoryInfo(mMemoryInfo);
          long freeMBs = mMemoryInfo.availMem / 1048576L;
          mInfoLeft.setText(freeMBs + " MB");

          //warning
          if (freeMBs < 400)
            mInfoLeft.setTextColor(Color.RED);
          else
            mInfoLeft.setTextColor(Color.WHITE);

          //battery state
          int bat = getBatteryPercentage(OpenConstructor.this);
          mInfoRight.setText(bat + "%");
          int icon = R.drawable.ic_battery_0;
          if (bat > 10)
            icon = R.drawable.ic_battery_20;
          if (bat > 30)
            icon = R.drawable.ic_battery_40;
          if (bat > 50)
            icon = R.drawable.ic_battery_60;
          if (bat > 70)
            icon = R.drawable.ic_battery_80;
          if (bat > 90)
            icon = R.drawable.ic_battery_100;
          mBattery.setBackgroundResource(icon);

          //warning
          if (bat < 15)
            mInfoRight.setTextColor(Color.RED);
          else
            mInfoRight.setTextColor(Color.WHITE);

          //update info about Tango
          String text = JNI.getEvent(OpenConstructor.this.getResources());
          mInfoLog.setVisibility(text.length() > 0 ? View.VISIBLE : View.GONE);
          mInfoLog.setText(text);

          if(!mViewMode)
            mLayoutInfo.setVisibility(View.VISIBLE);
          else
            mRunning = false;
        }
      });
    }
  }

  private Bitmap createBitmapFromGLSurface(int x, int y, int w, int h, GL10 gl) {
    int bitmapBuffer[] = new int[w * h];
    int bitmapSource[] = new int[w * h];
    IntBuffer intBuffer = IntBuffer.wrap(bitmapBuffer);
    intBuffer.position(0);

    try {
      gl.glReadPixels(x, y, w, h, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, intBuffer);
      int offset1, offset2;
      for (int i = 0; i < h; i++) {
        offset1 = i * w;
        offset2 = (h - i - 1) * w;
        for (int j = 0; j < w; j++) {
          int texturePixel = bitmapBuffer[offset1 + j];
          int blue = (texturePixel >> 16) & 0xff;
          int red = (texturePixel << 16) & 0x00ff0000;
          int pixel = (texturePixel & 0xff00ff00) | red | blue;
          bitmapSource[offset2 + j] = pixel;
        }
      }
    } catch (Exception e) {
      return null;
    }
    return Bitmap.createBitmap(bitmapSource, w, h, Bitmap.Config.ARGB_8888);
  }

  private void captureBitmap(boolean forced)
  {
    try {
      while(!JNI.animFinished())
      {
        Thread.sleep(20);
      }
    } catch (InterruptedException e)
    {
      e.printStackTrace();
    }
    final boolean[] finished = {false};
    final File thumbFile = new File(new File(mOpenedFile).getParent(), Exporter.getMtlResource(mOpenedFile) + ".png");
    if (forced || !thumbFile.exists())
    {
      mGLView.queueEvent(new Runnable()
      {
        @Override
        public void run()
        {
          int size = Math.min(mGLView.getWidth(), mGLView.getHeight());
          int x = (mGLView.getWidth() - size) / 2;
          int y = (mGLView.getHeight() - size) / 2;
          EGL10 egl = (EGL10) EGLContext.getEGL();
          GL10 gl = (GL10) egl.eglGetCurrentContext().getGL();
          Bitmap bitmap = createBitmapFromGLSurface(x, y, size, size, gl);
          if (bitmap != null)
          {
            try
            {
              File temp = new File(getTempPath(), "thumb.png");
              FileOutputStream out = new FileOutputStream(temp);
              bitmap = Bitmap.createScaledBitmap(bitmap, 256, 256, false);
              bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
              temp.renameTo(thumbFile);
            } catch (Exception e)
            {
              e.printStackTrace();
            }
          }
          finished[0] = true;
        }
      });
      try {
        while(!finished[0])
        {
          Thread.sleep(20);
        }
      } catch (InterruptedException e)
      {
        e.printStackTrace();
      }
    }
  }
}
