/**
 * This class is modified version of SketchfabActivity class from RTABMAP.
 * Do not use without reading licence conditions:
 * https://github.com/introlab/rtabmap
 */

package com.lvonasek.openconstructor.sketchfab;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import android.widget.EditText;
import android.widget.Toast;

import com.lvonasek.openconstructor.ui.AbstractActivity;
import com.lvonasek.openconstructor.R;
import com.lvonasek.openconstructor.ui.Service;

public class Uploader extends AbstractActivity implements OnClickListener
{
  private static final String UPLOADER_TAG = "openconstructor";
  private static final String UPLOAD_URL = "https://api.sketchfab.com/v3/models";

  private ProgressDialog mProgressDialog;
  private EditText mFilename;
  private EditText mDescription;
  private EditText mTags;
  private Button mButtonOk;
  private String mFile;
  private String mModelUri;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_upload);

    mFilename = (EditText) findViewById(R.id.editText_filename);
    mDescription = (EditText) findViewById(R.id.editText_description);
    mTags = (EditText) findViewById(R.id.editText_tags);
    mButtonOk = (Button) findViewById(R.id.button_ok);

    mProgressDialog = new ProgressDialog(this);
    mProgressDialog.setCanceledOnTouchOutside(false);

    mFile = getIntent().getExtras().getString(FILE_KEY);
    mFilename.setText(mFile);
    mButtonOk.setEnabled(mFilename.getText().toString().length() > 0);
    mButtonOk.setOnClickListener(this);

    mFilename.addTextChangedListener(new TextWatcher()
    {

      @Override
      public void afterTextChanged(Editable s)
      {
      }

      @Override
      public void beforeTextChanged(CharSequence s, int start,
                                    int count, int after)
      {
      }

      @Override
      public void onTextChanged(CharSequence s, int start,
                                int before, int count)
      {
        mButtonOk.setEnabled(s.length() != 0);
      }
    });

    mFilename.setSelectAllOnFocus(true);
    mFilename.requestFocus();
  }

  @Override
  public void onClick(View v)
  {
    // Handle button clicks.
    switch (v.getId())
    {
      case R.id.button_ok:
        File model2share = new File(AbstractActivity.getPath(), mFile);
        ArrayList<String> list = new ArrayList<>();
        list.add(model2share.getAbsolutePath());
        if (AbstractActivity.getModelType(mFile) == 0) //OBJ
          for (String s : AbstractActivity.getObjResources(model2share))
            list.add(new File(AbstractActivity.getPath(), s).getAbsolutePath());
        zipAndPublish(list.toArray(new String[list.size()]), mFilename.getText().toString());
        break;
    }
  }

  private void zipAndPublish(final String[] filesToZip, final String fileName)
  {
    final String zipOutput = new File(getTempPath(), fileName + ".zip").getAbsolutePath();
    mProgressDialog.setTitle(getString(R.string.sketchfab_dialog_title));
    mProgressDialog.setMessage(getString(R.string.compressing));
    mProgressDialog.show();

    Thread workingThread = new Thread(new Runnable()
    {
      public void run()
      {
        try
        {
          zip(filesToZip, zipOutput);
          runOnUiThread(new Runnable()
          {
            public void run()
            {
              mProgressDialog.dismiss();

              File f = new File(zipOutput);

              // Continue?
              AlertDialog.Builder builder = new AlertDialog.Builder(Uploader.this);
              builder.setTitle(R.string.sketchfab_upload_ready);

              final int fileSizeMB = (int) f.length() / (1024 * 1024);
              final int fileSizeKB = (int) f.length() / (1024);
              if (fileSizeMB == 0)
                builder.setMessage(getString(R.string.upload_size_is) + " " + fileSizeKB + " KB. " + getString(R.string.continue_question));
              else
              {
                String warning = fileSizeMB >= 50 ? getString(R.string.sketchfab_pro) : "";
                builder.setMessage(getString(R.string.upload_size_is) + " " + fileSizeMB + " MB. " + getString(R.string.continue_question) + warning);
              }


              builder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
              {
                public void onClick(DialogInterface dialog, int which)
                {
                  Service.process(Uploader.this, getString(R.string.sketchfab_uploading), false, new Runnable()
                  {
                    @Override
                    public void run()
                    {
                      String charset = "UTF-8";
                      File uploadFile = new File(zipOutput);
                      try
                      {
                        String token = OAuth.getToken();
                        MultipartUtility multipart = new MultipartUtility(UPLOAD_URL, token, charset);
                        multipart.addFormField("name", fileName);
                        multipart.addFormField("description", mDescription.getText().toString());
                        multipart.addFormField("tags", UPLOADER_TAG + " " + mTags.getText().toString());
                        multipart.addFormField("source", "open-constructor");
                        multipart.addFormField("isPublished", "false");
                        multipart.addFilePart("modelFile", uploadFile);
                        List<String> response = multipart.finish();
                        for (String line : response)
                        {
                          if (line.contains("\"uid\":\""))
                          {
                            String[] sArray = line.split("\"uid\":\"");
                            mModelUri = (sArray[1].split("\""))[0];
                          }
                        }
                      } catch (IOException e)
                      {
                        e.printStackTrace();
                      }
                    }
                  }, new Runnable()
                  {
                    @Override
                    public void run()
                    {
                      if (mModelUri != null)
                      {
                        Intent i = new Intent(Uploader.this, Home.class);
                        i.putExtra(AbstractActivity.URL_KEY, "https://sketchfab.com/models/" + mModelUri);
                        Uploader.this.startActivity(i);
                        finish();
                      } else
                        Toast.makeText(getApplicationContext(), "Upload failed!", Toast.LENGTH_LONG).show();
                    }
                  });
                  finish();
                }
              });
              builder.setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener()
              {
                public void onClick(DialogInterface dialog, int which)
                {
                  // do nothing...
                }
              });
              builder.show();
            }
          });
        } catch (Exception e)
        {
          e.printStackTrace();
        }
      }
    });

    workingThread.start();
  }
}