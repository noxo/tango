package com.lvonasek.openconstructor.main;

import android.hardware.Camera;
import android.media.ExifInterface;
import android.util.Log;

import com.lvonasek.openconstructor.ui.AbstractActivity;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class Exporter
{
  private static final int BUFFER_SIZE = 65536;
  private static boolean FOCAL_CACHED = false;
  private static float FOCAL_VALUE = 0;

  private static Camera openRearCamera() {
    Camera cam = null;
    Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
    int cameraCount = Camera.getNumberOfCameras();
    for (int camIdx = 0; camIdx < cameraCount; camIdx++) {
      Camera.getCameraInfo(camIdx, cameraInfo);
      if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
        cam = Camera.open(camIdx);
        break;
      }
    }
    return cam;
  }

  private static float getFocalLength()
  {
    if (!FOCAL_CACHED) {
      Camera cam = openRearCamera();
      FOCAL_VALUE = cam.getParameters().getFocalLength();
      cam.release();
      FOCAL_CACHED = true;
    }
    return FOCAL_VALUE;
  }

  private static void patchExif(File file)
  {
    try
    {
      ExifInterface newExif = new ExifInterface(file.getAbsolutePath());
      String focal = (long)(getFocalLength() * 100) + "/" + 100;
      newExif.setAttribute(ExifInterface.TAG_FOCAL_LENGTH, focal);
      newExif.saveAttributes();
    } catch (IOException e)
    {
      e.printStackTrace();
    }
  }

  public static void patchPath(File path)
  {
    for (File f : path.listFiles())
      if (f.getName().contains(".jpg"))
        patchExif(f);
    Log.d(AbstractActivity.TAG, "Patching exifs finished");
  }

  public static void zip(String[] files, String zip) throws Exception {
    try (ZipOutputStream out = new ZipOutputStream(new BufferedOutputStream(new FileOutputStream(zip))))
    {
      byte data[] = new byte[BUFFER_SIZE];
      for (String file : files)
      {
        FileInputStream fi = new FileInputStream(file);
        try (BufferedInputStream origin = new BufferedInputStream(fi, BUFFER_SIZE))
        {
          ZipEntry entry = new ZipEntry(file.substring(file.lastIndexOf("/") + 1));
          out.putNextEntry(entry);
          int count;
          while ((count = origin.read(data, 0, BUFFER_SIZE)) != -1)
          {
            out.write(data, 0, count);
          }
        }
      }
    }
  }
}
