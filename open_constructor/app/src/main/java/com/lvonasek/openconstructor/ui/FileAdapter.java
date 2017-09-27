package com.lvonasek.openconstructor.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.main.OpenConstructor;
import com.lvonasek.openconstructor.sketchfab.OAuth;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

class FileAdapter extends BaseAdapter
{
  private FileManager mContext;
  private ArrayList<String> mItems;
  private HashMap<String, String> mDescriptions;
  private HashMap<String, Drawable> mIcons;

  FileAdapter(FileManager context)
  {
    mContext = context;
    mDescriptions = new HashMap<>();
    mIcons = new HashMap<>();
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
    view.findViewById(R.id.context).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        contextMenu(index);
      }
    });
    String key = (String)getItem(index);
    TextView name = (TextView) view.findViewById(R.id.name);
    name.setText(key);

    //set description
    TextView desc = (TextView) view.findViewById(R.id.description);
    desc.setText(mDescriptions.get(key));

    //set icon
    View icon = view.findViewById(R.id.icon);
    icon.setBackground(mIcons.get(key));

    //set open action
    view.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View view)
      {
        Intent intent = new Intent(mContext, OpenConstructor.class);
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
        contextMenu(index);
        return true;
      }
    });
    return view;
  }

  void addItem(String name)
  {
    mItems.add(name);
    //load description
    if (!mDescriptions.containsKey(name)) {
      File obj = new File(AbstractActivity.getPath(), name);
      long length = obj.length();
      for (String s : AbstractActivity.getObjResources(obj))
        length += new File(AbstractActivity.getPath(), s).length();
      String description = "";
      description += AbstractActivity.getMtlResource(obj.getAbsolutePath()) + "\n";
      description += (int)(length / 1024 / 1024) + "MB";
      mDescriptions.put(name, description);
    }
    //load icon
    if (!mIcons.containsKey(name)) {
      File obj = new File(AbstractActivity.getPath(), name);
      String mtl = AbstractActivity.getMtlResource(obj.getAbsolutePath());
      final File thumbFile = new File(AbstractActivity.getPath(), mtl + ".png");
      if (thumbFile.exists()) {
        Drawable d = Drawable.createFromPath(thumbFile.getAbsolutePath());
        mIcons.put(name, d);
      } else
        mIcons.put(name, mContext.getDrawable(R.drawable.ic_model_icon));
    }
  }

  void clearItems()
  {
    mDescriptions.clear();
    mIcons.clear();
    mItems.clear();
  }

  private void contextMenu(final int index) {
    String[] operations = mContext.getResources().getStringArray(R.array.file_operations);
    AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
    builder.setTitle(mItems.get(index));
    builder.setItems(operations, new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        switch(which) {
          case 0://open
            Intent intent = new Intent(mContext, OpenConstructor.class);
            intent.putExtra(AbstractActivity.FILE_KEY, mItems.get(index));
            mContext.showProgress();
            mContext.startActivity(intent);
            break;
          case 1://share
            if (mItems.get(index).length() <= 4) {
              Toast.makeText(mContext, R.string.invalid_name, Toast.LENGTH_LONG).show();
              break;
            }
            Intent i = new Intent(mContext, OAuth.class);
            i.putExtra(AbstractActivity.FILE_KEY, mItems.get(index));
            mContext.startActivity(i);
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
                String name = input.getText().toString() + AbstractActivity.FILE_EXT[type];
                File newFile = new File(AbstractActivity.getPath(), name);
                if(newFile.exists())
                  Toast.makeText(mContext, R.string.name_exists, Toast.LENGTH_LONG).show();
                else {
                  File oldFile = new File(AbstractActivity.getPath(), mItems.get(index));
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
              File file = new File(AbstractActivity.getPath(), mItems.get(index));
              if (AbstractActivity.getModelType(mItems.get(index)) == 0) { //OBJ
                for(String s : AbstractActivity.getObjResources(file))
                  if (new File(AbstractActivity.getPath(), s).delete())
                    Log.d(AbstractActivity.TAG, "File " + s + " deleted");
              }
              if (file.delete())
                Log.d(AbstractActivity.TAG, "File " + mItems.get(index) + " deleted");
            } catch(Exception e) {
              e.printStackTrace();
            }
            mContext.refreshUI();
            break;
        }
      }
    });
    builder.setNegativeButton(mContext.getString(android.R.string.cancel), null);
    builder.show();
  }
}
