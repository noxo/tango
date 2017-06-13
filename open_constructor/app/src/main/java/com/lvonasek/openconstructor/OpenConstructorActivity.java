package com.lvonasek.openconstructor;

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.graphics.Point;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenConstructorActivity extends AbstractActivity implements View.OnClickListener,
        GLSurfaceView.Renderer, Runnable {

  private ActivityManager mActivityManager;
  private ActivityManager.MemoryInfo mMemoryInfo;
  private ProgressBar mProgress;
  private GLSurfaceView mGLView;
  private String mToLoad;
  private boolean m3drRunning = false;
  private boolean mViewMode = false;
  private boolean mRunning = false;
  private boolean mFirstSave = true;
  private String mSaveFilename = "";

  private LinearLayout mLayoutRec;
  private Button mToggleButton;
  private LinearLayout mLayoutInfo;
  private TextView mInfoLeft;
  private TextView mInfoMiddle;
  private TextView mInfoRight;
  private TextView mInfoLog;
  private View mBattery;
  private Button mCardboard;
  private Button mModeButton;
  private int mRes = 3;

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
      public void onServiceConnected(ComponentName name, IBinder srv) {
        double res      = mRes * 0.01;
        double dmin     = 0.6f;
        double dmax     = mRes * 1.5;
        int noise       = isNoiseFilterOn() ? 9 : 0;
        boolean land    = !isPortrait(OpenConstructorActivity.this);

        if (android.os.Build.DEVICE.toLowerCase().startsWith("yellowstone"))
          land = !land;

        if (mRes == 0) {
          res = 0.005;
          dmax = 1.0;
        }

        m3drRunning = true;
        String t = getTempPath().getAbsolutePath();
        TangoJNINative.onTangoServiceConnected(srv, res, dmin, dmax, noise, land, t);
        TangoJNINative.onToggleButtonClicked(m3drRunning);
        TangoJNINative.setView(0, 0, 0, 0, 0, true);
        OpenConstructorActivity.this.runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            mProgress.setVisibility(View.GONE);
          }
        });
        mInitialised = true;
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

    mCardboard = (Button) findViewById(R.id.cardboard_button);
    mModeButton = (Button) findViewById(R.id.mode_button);
    mLayoutInfo = (LinearLayout) findViewById(R.id.layout_info);
    mInfoLeft = (TextView) findViewById(R.id.info_left);
    mInfoMiddle = (TextView) findViewById(R.id.info_middle);
    mInfoRight = (TextView) findViewById(R.id.info_right);
    mInfoLog = (TextView) findViewById(R.id.infolog);
    mBattery = findViewById(R.id.info_battery);

    // OpenGL view where all of the graphics are drawn
    mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);
    mGLView.setEGLContextClientVersion(2);
    mGLView.setRenderer(this);
    mProgress = (ProgressBar) findViewById(R.id.progressBar);

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
        TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
      }

      @Override
      public void OnRotation(float angle) {
        mYawR = (float) Math.toRadians(-angle);
        TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
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
        TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
      }
    }, this);

    //open file
    mToLoad = null;
    String filename = getIntent().getStringExtra(FILE_KEY);
    if ((filename != null) && (filename.length() > 0))
    {
      m3drRunning = false;
      mToLoad = new File(getPath(), filename).toString();
      setViewerMode(mToLoad);
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
            TangoJNINative.onToggleButtonClicked(m3drRunning);
          }
        });
      break;
    case R.id.clear_button:
      mGLView.queueEvent(new Runnable() {
          @Override
          public void run() {
            TangoJNINative.onClearButtonClicked();
          }
        });
      break;
    case R.id.save_button:
      //pause
      m3drRunning = false;
      TangoJNINative.onToggleButtonClicked(false);
      save();
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
        mToLoad = null;
        new Thread(new Runnable()
        {
          @Override
          public void run()
          {
            TangoJNINative.load(file);
            TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, !mViewMode);
            OpenConstructorActivity.this.runOnUiThread(new Runnable()
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
    mGLView.onPause();
    mRunning = false;
    if (mTangoBinded) {
      unbindService(mTangoServiceConnection);
      Toast.makeText(this, R.string.data_lost, Toast.LENGTH_LONG).show();
    }
    System.exit(0);
  }

  @Override
  public void onBackPressed()
  {
    System.exit(0);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    mGestureDetector.onTouchEvent(event);
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
        mTopMode = !mTopMode;
        mModeButton.setBackgroundResource(mTopMode ? R.drawable.ic_mode3d : R.drawable.ic_mode2d);
        float floor = TangoJNINative.getFloorLevel(mMoveX, mMoveY, mMoveZ);
        if (floor < -9999)
          floor = 0;
        if (mTopMode) {
          mMoveZ = floor + 10.0f;
          mPitch = (float) Math.toRadians(-90);
        } else {
          mMoveZ = floor + 1.7f; //1.7m as an average human height
          mPitch = 0;
        }
        TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, false);
      }
    });
    if (isCardboardEnabled(this)) {
      mCardboard.setVisibility(View.VISIBLE);
      mCardboard.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View view)
        {
          Intent i = new Intent(Intent.ACTION_VIEW);
          i.setDataAndType(Uri.parse("file://" + filename), "application/" + CARDBOARD_APP);
          i.setPackage(CARDBOARD_APP);
          startActivity(i);
          System.exit(0);
        }
      });
    }
    float floor = TangoJNINative.getFloorLevel(mMoveX, mMoveY, mMoveZ);
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
    TangoJNINative.setView(mYawM + mYawR, mPitch, mMoveX, mMoveY, mMoveZ, false);
  }

  // Render loop of the Gl context.
  public void onDrawFrame(GL10 gl) {
    TangoJNINative.onGlSurfaceDrawFrame();
  }

  // Called when the surface size changes.
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    TangoJNINative.onGlSurfaceChanged(width, height);
  }

  // Called when the surface is created or recreated.
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    TangoJNINative.onGlSurfaceCreated();
  }

  private void save() {
    //filename dialog
    if (mFirstSave) {
      final Context context = OpenConstructorActivity.this;
      AlertDialog.Builder builder = new AlertDialog.Builder(context);
      builder.setTitle(getString(R.string.enter_filename));
      final EditText input = new EditText(context);
      builder.setView(input);
      builder.setPositiveButton(getString(android.R.string.ok), new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
          //delete old during overwrite
          File file = new File(getPath(), input.getText().toString() + FILE_EXT[0]);
          try {
            if (file.exists())
              for(String s : getObjResources(file))
                if (new File(getPath(), s).delete())
                  Log.d(AbstractActivity.TAG, "File " + s + " deleted");
          } catch(Exception e) {
            e.printStackTrace();
          }
          mSaveFilename = input.getText().toString();
          saveObj();
          dialog.cancel();
        }
      });
      builder.setNegativeButton(getString(android.R.string.cancel), null);
      builder.create().show();
    } else
      saveObj();
  }

  private void saveObj()
  {
    mLayoutRec.setVisibility(View.GONE);
    mProgress.setVisibility(View.VISIBLE);
    new Thread(new Runnable(){
      @Override
      public void run()
      {
        mFirstSave = false;
        String dataset = "";
        long timestamp = System.currentTimeMillis();
        final File obj = new File(getTempPath(), timestamp + FILE_EXT[0]);
        for (File f : getTempPath().listFiles())
          if (f.isDirectory()) {
            dataset = f.toString();
            break;
          }
        TangoJNINative.save(obj.getAbsolutePath(), dataset);
        //open???
        final String data = dataset;
        OpenConstructorActivity.this.runOnUiThread(new Runnable()
        {
          @Override
          public void run()
          {
            mProgress.setVisibility(View.GONE);
            AlertDialog.Builder builder = new AlertDialog.Builder(OpenConstructorActivity.this);
            builder.setTitle(getString(R.string.view));
            builder.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog, int which) {
                mLayoutRec.setVisibility(View.VISIBLE);
                dialog.cancel();
              }
            });
            builder.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog, int which) {
                mProgress.setVisibility(View.VISIBLE);
                new Thread(new Runnable()
                {
                  @Override
                  public void run()
                  {
                    if (isTexturingOn())
                      TangoJNINative.texturize(obj.getAbsolutePath(), data);
                    for(String s : getObjResources(obj.getAbsoluteFile()))
                      if (new File(getTempPath(), s).renameTo(new File(getPath(), s)))
                        Log.d(AbstractActivity.TAG, "File " + s + " saved");
                    final File file2save = new File(getPath(), mSaveFilename + FILE_EXT[0]);
                    if (obj.renameTo(file2save))
                      Log.d(TAG, "Obj file " + file2save.toString() + " saved.");

                    OpenConstructorActivity.this.runOnUiThread(new Runnable()
                    {
                      @Override
                      public void run()
                      {
                        setViewerMode(file2save.getAbsolutePath());
                        mProgress.setVisibility(View.GONE);
                      }
                    });
                  }
                }).start();
                dialog.cancel();
              }
            });
            builder.setCancelable(false);
            builder.create().show();
          }
        });
      }
    }).start();
  }

  public static int getBatteryPercentage(Context context) {
    IntentFilter iFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
    Intent batteryStatus = context.registerReceiver(null, iFilter);
    int level = batteryStatus != null ? batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1) : -1;
    int scale = batteryStatus != null ? batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1) : -1;
    float batteryPct = level / (float) scale;
    return (int) (batteryPct * 100);
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
          int bat = getBatteryPercentage(OpenConstructorActivity.this);
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

          //max distance
          String text = getString(R.string.distance) + " ";
          if(mRes > 0)
            text += (1.5f * mRes) + " m, ";
          else if(mRes == 0)
            text += "1.0 m, ";
          //3d resolution
          text += getString(R.string.resolution) + " ";
          if(mRes > 0)
            text += mRes + " cm";
          else if(mRes == 0)
            text += "0.5 cm";
          mInfoMiddle.setText(text);

          //update info about Tango
          text = new String(TangoJNINative.getEvent());
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
}
