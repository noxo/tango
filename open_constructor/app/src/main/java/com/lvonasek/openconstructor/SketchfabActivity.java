package com.lvonasek.openconstructor;

import android.Manifest;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class SketchfabActivity extends AbstractActivity
{
  private static final String SKETCHFAB_HOME = "https://sketchfab.com";
  private static final String SKETCHFAB_UPLOAD = "https://sketchfab.com/upload";

  private Uri mUri;
  private WebView mWebView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_sketchfab);

    mUri = filename2Uri(getIntent().getStringExtra(FILE_KEY));
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
    setupPermission(Manifest.permission.INTERNET, REQUEST_CODE_PERMISSION_INTERNET);
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case REQUEST_CODE_PERMISSION_INTERNET:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
          if (mUri != null)
            mWebView.loadUrl(SKETCHFAB_UPLOAD);
          else
            mWebView.loadUrl(SKETCHFAB_HOME);
        }else
          finish();
        break;
      }
    }
  }
}
