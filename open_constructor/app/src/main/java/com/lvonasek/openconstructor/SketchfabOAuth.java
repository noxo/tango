package com.lvonasek.openconstructor;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.util.ArrayList;
import java.util.Scanner;

import cz.msebera.android.httpclient.Header;
import cz.msebera.android.httpclient.HttpResponse;
import cz.msebera.android.httpclient.NameValuePair;
import cz.msebera.android.httpclient.client.HttpClient;
import cz.msebera.android.httpclient.client.entity.UrlEncodedFormEntity;
import cz.msebera.android.httpclient.client.methods.HttpPost;
import cz.msebera.android.httpclient.impl.client.DefaultHttpClient;
import cz.msebera.android.httpclient.message.BasicNameValuePair;
import cz.msebera.android.httpclient.util.EntityUtils;

public class SketchfabOAuth extends AbstractActivity
{
  private static final String CLIENT_ID = "57kFPBj5OaSdKeroj1p2WQw7n11YFeDLea6sXnqi";
  private static final String OAUTH_URL = "https://sketchfab.com/oauth2/authorize/?response_type=code&client_id=";
  private static final String REDIRECT  = "http://127.0.0.1:8080";
  private static final String TOKEN_URL = "https://sketchfab.com/oauth2/token/";
  private static final int PERMISSIONS_CODE = 1989;

  private static String token = null;
  private String mExtra;
  private WebView mWebView;

  public static String getToken()
  {
    return token;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_sketchfab);

    mExtra = getIntent().getStringExtra(FILE_KEY);
    mWebView = (WebView) findViewById(R.id.webview);
    WebSettings webSettings = mWebView.getSettings();
    webSettings.setJavaScriptEnabled(true);
    mWebView.setWebViewClient(new WebViewClient() {
      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url) {
        // Here put your code
        if (url.startsWith(REDIRECT)) {
          if (!url.contains("code="))
          {
            finish();
            return true;
          }
          String code = url.substring(url.indexOf("code=") + 5);

          String secret = new String(TangoJNINative.clientSecret());
          final ArrayList<NameValuePair> authorize = new ArrayList<>();
          authorize.add(new BasicNameValuePair("grant_type", "authorization_code"));
          authorize.add(new BasicNameValuePair("code", code));
          authorize.add(new BasicNameValuePair("client_id", CLIENT_ID));
          authorize.add(new BasicNameValuePair("client_secret", secret));
          authorize.add(new BasicNameValuePair("redirect_uri", REDIRECT));

          new Thread(new Runnable()
          {
            @Override
            public void run()
            {
              try{
                HttpClient httpclient = new DefaultHttpClient();
                HttpPost httppost = new HttpPost(TOKEN_URL);
                httppost.setEntity(new UrlEncodedFormEntity(authorize));
                HttpResponse response = httpclient.execute(httppost);
                if(response.getStatusLine().getStatusCode() == 200)
                {
                  token = EntityUtils.toString(response.getEntity());
                  token = token.substring(token.indexOf("access_token"));
                  token = token.substring(token.indexOf(":"));
                  token = token.substring(token.indexOf("\""));
                  token = token.substring(1, token.indexOf("\","));
                  Activity activity = SketchfabOAuth.this;
                  Intent i = new Intent(activity, SketchfabUploader.class);
                  i.putExtra(AbstractActivity.FILE_KEY, mExtra);
                  SketchfabOAuth.this.startActivity(i);
                  finish();
                  return;
                }
                Log.e("Oauth return code=", "" + response.getStatusLine().getStatusCode());
              } catch(Exception e) {
                e.printStackTrace();
              }
              finish();
            }
          }).start();
          return true;
        }
        return false;
      }
    });
    setupPermissions();
  }


  protected void setupPermissions() {
    String[] permissions = {
            Manifest.permission.INTERNET
    };
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      boolean ok = true;
      for (String s : permissions)
        if (checkSelfPermission(s) != PackageManager.PERMISSION_GRANTED)
          ok = false;

      if (!ok)
        requestPermissions(permissions, PERMISSIONS_CODE);
      else
        onRequestPermissionsResult(PERMISSIONS_CODE, null, new int[]{PackageManager.PERMISSION_GRANTED});
    } else
      onRequestPermissionsResult(PERMISSIONS_CODE, null, new int[]{PackageManager.PERMISSION_GRANTED});
  }


  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case PERMISSIONS_CODE:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
          mWebView.loadUrl(OAUTH_URL + CLIENT_ID + "&redirect_uri=" + REDIRECT);
        else
          finish();
        break;
      }
    }
  }
}
