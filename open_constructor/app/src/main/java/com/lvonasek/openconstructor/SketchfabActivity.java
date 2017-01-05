package com.lvonasek.openconstructor;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.WindowManager;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class SketchfabActivity extends Activity
{
  private static final int REQUEST_CODE_PERMISSION_INTERNET = 1989;
  private static final String SKETCHFAB_UPLOAD = "https://sketchfab.com/upload";

  private WebView mWebView;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_sketchfab);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    final Uri uri = FileUtils.filename2Uri(getIntent().getStringExtra(FileUtils.FILE_KEY));
    mWebView = (WebView) findViewById(R.id.webview);
    WebSettings webSettings = mWebView.getSettings();
    webSettings.setJavaScriptEnabled(true);
    mWebView.setWebViewClient(new WebViewClient());
    mWebView.setWebChromeClient(new WebChromeClient()
    {
      //For Android 4.1+
      public void openFileChooser(ValueCallback<Uri> callback, String acceptType, String capture){
        callback.onReceiveValue(uri);
      }
      //For Android 5.0+
      public boolean onShowFileChooser(WebView webView, ValueCallback<Uri[]> callback, WebChromeClient.FileChooserParams params){
        callback.onReceiveValue(new Uri[]{uri});
        return true;
      }
    });
    setupPermission(Manifest.permission.INTERNET, REQUEST_CODE_PERMISSION_INTERNET);
  }

  private void setupPermission(String permission, int requestCode) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
        requestPermissions(new String[]{permission}, requestCode);
      else
        onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
    } else
      onRequestPermissionsResult(requestCode, null, new int[]{PackageManager.PERMISSION_GRANTED});
  }

  @Override
  public synchronized void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
  {
    switch (requestCode)
    {
      case REQUEST_CODE_PERMISSION_INTERNET:
      {
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
          mWebView.loadUrl(SKETCHFAB_UPLOAD);
        else
          finish();
        break;
      }
    }
  }
}
