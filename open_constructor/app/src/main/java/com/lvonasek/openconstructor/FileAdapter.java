package com.lvonasek.openconstructor;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

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
        //TODO:menu - open, upload, rename, delete...
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
