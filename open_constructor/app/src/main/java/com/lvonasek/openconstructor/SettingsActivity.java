package com.lvonasek.openconstructor;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.view.WindowManager;

public class SettingsActivity extends PreferenceActivity implements Preference.OnPreferenceChangeListener {
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.settings);
    findPreference(getString(R.string.pref_landscape)).setOnPreferenceChangeListener(this);
    findPreference(getString(R.string.pref_cardboard)).setOnPreferenceChangeListener(this);
  }

  @Override
  public boolean onPreferenceChange(Preference preference, Object value)
  {
    if (preference.getKey() == getString(R.string.pref_landscape))
      AbstractActivity.setOrientation(!((boolean)value), this);
    else if (preference.getKey() == getString(R.string.pref_cardboard)) {
      if (!AbstractActivity.isCardboardAppInstalled(this)) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getString(R.string.cardboard_support));
        builder.setPositiveButton(getString(R.string.yes), new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
            AbstractActivity.installCardboardApp(SettingsActivity.this);
            dialog.dismiss();
            recreate();
          }
        });
        builder.setNegativeButton(getString(R.string.no), new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface dialog, int which) {
            dialog.dismiss();
            recreate();
          }
        });
        builder.create().show();
      }
    }
    return true;
  }

  @Override
  protected void onPause()
  {
    update();
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    AbstractActivity.setOrientation(AbstractActivity.isPortrait(this), this);
    update();
  }

  private void update() {
    if (!AbstractActivity.isCardboardAppInstalled(this)) {
      SharedPreferences.Editor prefs = PreferenceManager.getDefaultSharedPreferences(this).edit();
      prefs.putBoolean("pref_cardboard", false);
      prefs.apply();
    }
  }
}
