package com.lvonasek.openconstructor;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.SeekBar;
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
              case 1://filter
                AlertDialog.Builder filterDlg = new AlertDialog.Builder(mContext);
                String title = mContext.getString(R.string.passes_count) + "\n";
                TextView textView = new TextView(mContext);
                textView.setText(title + mContext.getString(R.string.warning_long));
                filterDlg.setCustomTitle(textView);
                final SeekBar seekBar = new SeekBar(mContext) {
                  Paint p = new Paint();
                  @Override
                  protected void onDraw(Canvas c) {
                    super.onDraw(c);
                    p.setColor(Color.WHITE);
                    p.setTextSize(getHeight() / 2);
                    c.drawText((getProgress() + 1) + "x", getWidth() / 2, getHeight() * 3 / 4, p);
                  }
                };
                seekBar.setMax(9);
                seekBar.setProgress(4);
                filterDlg.setView(seekBar);
                filterDlg.setPositiveButton(mContext.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    mContext.showProgress();
                    new Thread(new Runnable()
                    {
                      @Override
                      public void run()
                      {
                        String oldName = new File(mContext.getPath(), mItems.get(index)).toString();
                        String newName = oldName.replaceAll(AbstractActivity.FILE_EXT, "");
                        newName += "-" + mContext.getString(R.string.filtered) + AbstractActivity.FILE_EXT;
                        TangoJNINative.filter(oldName, newName, seekBar.getProgress() + 1);
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
                  }
                });
                filterDlg.setNegativeButton(mContext.getString(android.R.string.cancel), null);
                filterDlg.create().show();
                break;
              case 2://share
                Intent i = new Intent(mContext, SketchfabActivity.class);
                i.putExtra(AbstractActivity.FILE_KEY, mItems.get(index));
                mContext.startActivity(i);
                break;
              case 3://rename
                AlertDialog.Builder renameDlg = new AlertDialog.Builder(mContext);
                renameDlg.setTitle(mContext.getString(R.string.enter_filename));
                final EditText input = new EditText(mContext);
                renameDlg.setView(input);
                renameDlg.setPositiveButton(mContext.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                    File newFile = new File(mContext.getPath(), input.getText().toString() + AbstractActivity.FILE_EXT);
                    if(newFile.exists())
                      Toast.makeText(mContext, R.string.name_exists, Toast.LENGTH_LONG).show();
                    else {
                      File oldFile = new File(mContext.getPath(), mItems.get(index));
                      oldFile.renameTo(newFile);
                      mContext.refreshList();
                    }
                  }
                });
                renameDlg.setNegativeButton(mContext.getString(android.R.string.cancel), null);
                renameDlg.create().show();
                break;
              case 4://delete
                try {
                  new File(mContext.getPath(), mItems.get(index)).delete();
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
