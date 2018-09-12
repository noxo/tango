package com.lvonasek.openconstructor.main;

import android.content.res.Resources;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.util.Log;

import com.lvonasek.openconstructor.ui.AbstractActivity;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Scanner;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

public class Exporter
{
  private static final int BUFFER_SIZE = 65536;
  public static final String[] FILE_EXT = {".obj"};
  private static boolean FOCAL_CACHED = false;
  private static float FOCAL_VALUE = 0;

  public static void extractRawData(Resources res, int data, File outputDir)
  {
    InputStream in = res.openRawResource(data);
    try {
      ZipInputStream zin = new ZipInputStream(in);
      ZipEntry entry;
      while ((entry = zin.getNextEntry()) != null) {
        String name = entry.getName();
        if( entry.isDirectory() ) {
          new File(outputDir, name).mkdirs();
        } else {
          int s = name.lastIndexOf( File.separatorChar );
          String dir =  s == -1 ? null : name.substring( 0, s );
          if( dir != null )
            new File(outputDir, dir).mkdirs();
          ByteArrayOutputStream baos = new ByteArrayOutputStream();
          byte[] buffer = new byte[BUFFER_SIZE];
          int count;
          while ((count = zin.read(buffer)) != -1)
            baos.write(buffer, 0, count);
          BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(new File(outputDir, name)));
          out.write(baos.toByteArray());
          out.close();
        }
      }
      in.close();
    } catch (Exception e)
    {
      e.printStackTrace();
    }
  }

  public static int getModelType(String filename) {
    for(int i = 0; i < FILE_EXT.length; i++) {
      int begin = filename.length() - FILE_EXT[i].length();
      if (begin >= 0)
        if (filename.substring(begin).contains(FILE_EXT[i]))
          return i;
    }
    return -1;
  }

  public static String getMtlResource(String obj)
  {
    try
    {
      Scanner sc = new Scanner(new FileInputStream(obj));
      while(sc.hasNext()) {
        String line = sc.nextLine();
        if (line.startsWith("mtllib")) {
          return line.substring(7);
        }
      }
      sc.close();
    } catch (Exception e)
    {
      e.printStackTrace();
    }
    return null;
  }

  public static ArrayList<String> getObjResources(File file)
  {
    HashSet<String> files = new HashSet<>();
    ArrayList<String> output = new ArrayList<>();
    String mtlLib = getMtlResource(file.getAbsolutePath());
    if (mtlLib != null) {
      output.add(mtlLib);
      output.add(mtlLib + ".png");
      mtlLib = file.getParent() + "/" + mtlLib;
      try
      {
        Scanner sc = new Scanner(new FileInputStream(mtlLib));
        while(sc.hasNext()) {
          String line = sc.nextLine();
          if (line.startsWith("map_Kd")) {
            String filename = line.substring(7);
            if (!files.contains(filename))
            {
              files.add(filename);
              output.add(filename);
            }
          }
        }
        sc.close();
      } catch (Exception e)
      {
        e.printStackTrace();
      }
    }
    return output;
  }

  public static void makeStructure(String path) {
    //get list of files
    ArrayList<String> models = new ArrayList<>();
    String[] files = new File(path).list();
    Arrays.sort(files);
    for(String s : files)
      if(new File(path, s).isFile())
        if(getModelType(s) >= 0)
          models.add(s);

    //restructure models
    for (String s : models) {

      //create temp folder
      File temp = new File(path, "temp");
      if (temp.exists())
        AbstractActivity.deleteRecursive(temp);
      temp.mkdir();

      //get model files
      ArrayList<String> res = getObjResources(new File(path, s));
      res.add(s);

      //restructure
      for (String r : res) {
        File f = new File(path, r);
        if (f.exists())
          f.renameTo(new File(temp, r));
      }
      temp.renameTo(new File(path, s));
    }
  }

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
      out.setMethod(ZipOutputStream.DEFLATED);
      out.setLevel(9);
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
