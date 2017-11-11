package com.lvonasek.openconstructor.ui;

import android.os.Build;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.view.WindowManager;

import com.lvonasek.openconstructor.R;

import java.util.Objects;

public class Settings extends PreferenceActivity implements Preference.OnPreferenceChangeListener {
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      setTheme(android.R.style.Theme_Material_NoActionBar_Fullscreen);
    addPreferencesFromResource(R.xml.settings);
    findPreference(getString(R.string.pref_landscape)).setOnPreferenceChangeListener(this);
    findPreference(getString(R.string.pref_cardboard)).setOnPreferenceChangeListener(this);
  }

  @Override
  public boolean onPreferenceChange(Preference preference, Object value)
  {
    if (Objects.equals(preference.getKey(), getString(R.string.pref_landscape)))
      AbstractActivity.setOrientation(!((boolean)value), this);
    return true;
  }

  @Override
  protected void onResume() {
    super.onResume();
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    AbstractActivity.setOrientation(AbstractActivity.isPortrait(this), this);
  }
}
