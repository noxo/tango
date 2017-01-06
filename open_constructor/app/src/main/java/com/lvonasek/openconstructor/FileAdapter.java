package com.lvonasek.openconstructor;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.ArrayList;

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
        intent.putExtra(FileUtils.FILE_KEY, mItems.get(index));
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
                intent.putExtra(FileUtils.FILE_KEY, mItems.get(index));
                mContext.showProgress();
                mContext.startActivity(intent);
                break;
              case 1://filter
                mContext.showProgress();
                new Thread(new Runnable()
                {
                  @Override
                  public void run()
                  {
                    String oldName = new File(FileUtils.getPath(), mItems.get(index)).toString();
                    String newName = oldName.replaceAll(FileUtils.FILE_EXT, "");
                    newName += "-" + mContext.getString(R.string.filtered) + FileUtils.FILE_EXT;
                    TangoJNINative.filter(oldName, newName);
                    mContext.runOnUiThread(new Runnable()
                    {
                      @Override
                      public void run()
                      {
                        mContext.refreshList();
                      }
                    });
                  }
                }).start();
                break;
              case 2://share
                Intent i = new Intent(mContext, SketchfabActivity.class);
                i.putExtra(FileUtils.FILE_KEY, mItems.get(index));
                mContext.startActivity(i);
                break;
              case 3://rename
                AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
                builder.setTitle(mContext.getString(R.string.enter_filename));
                final EditText input = new EditText(mContext);
                builder.setView(input);
                builder.setPositiveButton(mContext.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    File newFile = new File(FileUtils.getPath(), input.getText().toString() + FileUtils.FILE_EXT);
                    if(newFile.exists())
                      Toast.makeText(mContext, R.string.name_exists, Toast.LENGTH_LONG).show();
                    else {
                      File oldFile = new File(FileUtils.getPath(), mItems.get(index));
                      oldFile.renameTo(newFile);
                      mContext.refreshList();
                    }
                  }
                });
                builder.setNegativeButton(mContext.getString(android.R.string.cancel), null);
                builder.create().show();
                break;
              case 4://delete
                try {
                  new File(FileUtils.getPath(), mItems.get(index)).delete();
                } catch(Exception e){}
                mContext.refreshList();
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
}
