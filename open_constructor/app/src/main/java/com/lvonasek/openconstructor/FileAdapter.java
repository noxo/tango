package com.lvonasek.openconstructor;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Scanner;

class FileAdapter extends BaseAdapter
{
  private FileActivity mContext;
  private ArrayList<String> mItems;

  FileAdapter(FileActivity context)
  {
    mContext = context;
    mItems = new ArrayList<>();
  }

  @Override
  public int getCount()
  {
    return mItems.size();
  }

  @Override
  public Object getItem(int i)
  {
    return mItems.get(i);
  }

  @Override
  public long getItemId(int i)
  {
    return i;
  }

  @Override
  public View getView(final int index, View view, ViewGroup viewGroup)
  {
    LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    if (view == null)
      view = inflater.inflate(R.layout.view_item, null, true);
    TextView name = (TextView) view.findViewById(R.id.name);
    name.setText(mItems.get(index));
    view.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        Intent intent = new Intent(mContext, OpenConstructorActivity.class);
        intent.putExtra(AbstractActivity.FILE_KEY, mItems.get(index));
        mContext.showProgress();
        mContext.startActivity(intent);
      }
    });
    view.setOnLongClickListener(new View.OnLongClickListener()
    {
      @Override
      public boolean onLongClick(View view)
      {
        String[] operations = mContext.getResources().getStringArray(R.array.file_operations);
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mItems.get(index));
        builder.setItems(operations, new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            switch(which) {
              case 0://open
                Intent intent = new Intent(mContext, OpenConstructorActivity.class);
                intent.putExtra(AbstractActivity.FILE_KEY, mItems.get(index));
                mContext.showProgress();
                mContext.startActivity(intent);
                break;
              case 1://share
                mContext.showProgress();
                new Thread(new Runnable() {
                  @Override
                  public void run()
                  {
                    try
                    {
                      String zip = mContext.getPath() + AbstractActivity.ZIP_TEMP;
                      File model2share = new File(mContext.getPath(), mItems.get(index));
                      ArrayList<String> list = new ArrayList<>();
                      if (AbstractActivity.getModelType(mItems.get(index)) == 0) //OBJ
                        for (String s : getObjResources(model2share))
                        list.add(new File(mContext.getPath(), s).getAbsolutePath());
                      list.add(model2share.getAbsolutePath());
                      mContext.zip(list.toArray(new String[list.size()]), zip);
                      mContext.runOnUiThread(new Runnable()
                      {
                        @Override
                        public void run()
                        {
                          Intent i = new Intent(mContext, SketchfabActivity.class);
                          i.putExtra(AbstractActivity.FILE_KEY, AbstractActivity.ZIP_TEMP);
                          mContext.startActivity(i);
                        }
                      });
                    } catch (Exception e)
                    {
                      e.printStackTrace();
                    }
                  }
                }).start();
                break;
              case 2://rename
                AlertDialog.Builder renameDlg = new AlertDialog.Builder(mContext);
                renameDlg.setTitle(mContext.getString(R.string.enter_filename));
                final EditText input = new EditText(mContext);
                renameDlg.setView(input);
                renameDlg.setPositiveButton(mContext.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    int type = AbstractActivity.getModelType(mItems.get(index));
                    File newFile = new File(mContext.getPath(), input.getText().toString() + AbstractActivity.FILE_EXT[type]);
                    if(newFile.exists())
                      Toast.makeText(mContext, R.string.name_exists, Toast.LENGTH_LONG).show();
                    else {
                      File oldFile = new File(mContext.getPath(), mItems.get(index));
                      if (oldFile.renameTo(newFile))
                        Log.d(AbstractActivity.TAG, "File " + oldFile + " renamed to " + newFile);
                      mContext.refreshUI();
                    }
                  }
                });
                renameDlg.setNegativeButton(mContext.getString(android.R.string.cancel), null);
                renameDlg.create().show();
                break;
              case 3://delete
                try {
                  File file = new File(mContext.getPath(), mItems.get(index));
                  if (AbstractActivity.getModelType(mItems.get(index)) == 0) { //OBJ
                    for(String s : getObjResources(file))
                      if (new File(mContext.getPath(), s).delete())
                        Log.d(AbstractActivity.TAG, "File " + s + " deleted");
                  }
                  if (file.delete())
                    Log.d(AbstractActivity.TAG, "File " + mItems.get(index) + " deleted");
                } catch(Exception e){
                  e.printStackTrace();
                }
                mContext.refreshUI();
                break;
            }
          }
        });
        builder.setNegativeButton(mContext.getString(android.R.string.cancel), null);
        builder.show();
        return true;
      }
    });
    return view;
  }

  void addItem(String name)
  {
    mItems.add(name);
  }

  void clearItems()
  {
    mItems.clear();
  }

  private ArrayList<String> getObjResources(File file) throws FileNotFoundException
  {
    Scanner sc = new Scanner(new FileInputStream(file.getAbsolutePath()));
    String filter = "xxx" + System.currentTimeMillis(); //not possible filter
    while(sc.hasNext()) {
      String line = sc.nextLine();
      if (line.startsWith("usemtl")) {
        filter = line.substring(7, line.indexOf('_'));
        break;
      }
    }
    sc.close();
    ArrayList<String> output = new ArrayList<>();
    for(String s : new File(mContext.getPath()).list())
      if(s.startsWith(filter))
        output.add(s);
    return output;
  }
}
