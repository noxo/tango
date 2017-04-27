package com.lvonasek.openconstructor.sketchfab;

import android.Manifest;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.lvonasek.openconstructor.AbstractActivity;
import com.lvonasek.openconstructor.R;

public class Home extends AbstractActivity
{
  private static final String SKETCHFAB_HOME = "https://sketchfab.com/login";
  private static final String SKETCHFAB_UPLOAD = "https://sketchfab.com/upload"; //deprecated(unused)

  private static final int PERMISSIONS_CODE = 1990;

  private String mUrl;
  private Uri mUri;
  private WebView mWebView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_sketchfab);

    mUri = filename2Uri(getIntent().getStringExtra(FILE_KEY));
    mUrl = getIntent().getStringExtra(URL_KEY);
    mWebView = (WebView) findViewById(R.id.webview);
    WebSettings webSettings = mWebView.getSettings();
    webSettings.setJavaScriptEnabled(true);
    mWebView.setWebViewClient(new WebViewClient());
    if (mUri != null) {
      mWebView.setWebChromeClient(new WebChromeClient()
      {
        //For Android 4.1+
        public void openFileChooser(ValueCallback<Uri> callback, String acceptType, String capture){
          callback.onReceiveValue(mUri);
        }
        //For Android 5.0+
        public boolean onShowFileChooser(WebView webView, ValueCallback<Uri[]> callback, WebChromeClient.FileChooserParams params){
          callback.onReceiveValue(new Uri[]{mUri});
          return true;
        }
      });
    }
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
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
          if (mUri != null)
            mWebView.loadUrl(SKETCHFAB_UPLOAD);
          else if (mUrl != null)
            mWebView.loadUrl(mUrl);
          else
            mWebView.loadUrl(SKETCHFAB_HOME);
        }else
          finish();
        break;
      }
    }
  }
}
