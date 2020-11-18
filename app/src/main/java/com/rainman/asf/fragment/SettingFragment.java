package com.rainman.asf.fragment;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.preference.SwitchPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import com.rainman.asf.AppSetting;
import com.rainman.asf.R;
import com.rainman.asf.activity.AboutActivity;
import com.rainman.asf.activity.VisitorActivity;
import com.rainman.asf.core.ScriptEngine;
import com.rainman.asf.core.database.CoreDatabase;
import com.rainman.asf.core.database.Visitor;
import com.rainman.asf.core.database.VisitorDao;
import com.rainman.asf.util.RootUtil;

import java.util.List;
import java.util.Locale;

public class SettingFragment extends PreferenceFragmentCompat {

    private static final int REQUEST_IGNORE_BATTERY_OPTIMIZATIONS = 100;
    private Activity mActivity;
    private Preference mShowAuthorizedDevices;
    private SwitchPreference mBackgroudWhiteList;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
        mActivity = (Activity) context;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        getPreferenceManager().setSharedPreferencesName("setting");
        addPreferencesFromResource(R.xml.pref_settings);

        SwitchPreference spRunByRoot = findPreference("run_by_root");
        assert spRunByRoot != null;
        spRunByRoot.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object o) {
                new Handler().post(new Runnable() {
                    @Override
                    public void run() {
                        ScriptEngine.getInstance().switchEngine();
                    }
                });
                return true;
            }
        });

        if (!RootUtil.haveRoot()) {
            spRunByRoot.setEnabled(false);
            spRunByRoot.setChecked(false);
        }

        SwitchPreference spEnableCmdServer = findPreference("enable_cmd_server");
        assert spEnableCmdServer != null;
        final SwitchPreference spEnableAccesssAuthorization = findPreference("enable_accesss_authorization");
        assert spEnableAccesssAuthorization != null;
        spEnableCmdServer.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object o) {
                new Handler().post(new Runnable() {
                    @Override
                    public void run() {
                        ScriptEngine.getInstance().switchCmdServer();
                    }
                });
                spEnableAccesssAuthorization.setEnabled((boolean) o);
                return true;
            }
        });
        spEnableAccesssAuthorization.setEnabled(AppSetting.isCmdServerEnabled());

        mShowAuthorizedDevices = findPreference("show_authorized_devices");
        assert mShowAuthorizedDevices != null;
        mShowAuthorizedDevices.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                startActivity(new Intent(mActivity, VisitorActivity.class));
                return true;
            }
        });

        Preference aboutApp = findPreference("about_app");
        assert aboutApp != null;
        aboutApp.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                startActivity(new Intent(mActivity, AboutActivity.class));
                return true;
            }
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mBackgroudWhiteList = findPreference("backgroud_white_list");
            if (isIgnoringBatteryOptimizations()) {
                mBackgroudWhiteList.setChecked(true);
                mBackgroudWhiteList.setEnabled(false);
            } else {
                mBackgroudWhiteList.setChecked(false);
                mBackgroudWhiteList.setEnabled(true);
                mBackgroudWhiteList.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference preference, Object newValue) {
                        if ((boolean) newValue) {
                            requestIgnoreBatteryOptimizations();
                        }
                        return true;
                    }
                });
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        VisitorDao visitorDao = CoreDatabase.getInstance(mActivity.getApplicationContext()).getVisitorDao();
        List<Visitor> visitors = visitorDao.getAll();
        int granted = 0, denied = 0;
        for (Visitor visitor : visitors) {
            if (visitor.getAccessPermission() == Visitor.PERMISSION_GRANTED && !visitor.isPermissionExpired()) {
                granted++;
            } else if (visitor.getAccessPermission() == Visitor.PERMISSION_DENIED) {
                denied++;
            }
        }
        mShowAuthorizedDevices.setSummary(String.format(Locale.getDefault(), getString(R.string.authorized_devices_desc), granted, denied));
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private boolean isIgnoringBatteryOptimizations() {
        boolean isIgnoring = false;
        PowerManager powerManager = (PowerManager) mActivity.getSystemService(Context.POWER_SERVICE);
        if (powerManager != null) {
            isIgnoring = powerManager.isIgnoringBatteryOptimizations(mActivity.getPackageName());
        }
        return isIgnoring;
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public void requestIgnoreBatteryOptimizations() {
        try {
            Intent intent = new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
            intent.setData(Uri.parse("package:" + mActivity.getPackageName()));
            startActivityForResult(intent, REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && requestCode == REQUEST_IGNORE_BATTERY_OPTIMIZATIONS) {
            if (resultCode == Activity.RESULT_OK) {
                if (isIgnoringBatteryOptimizations()) {
                    mBackgroudWhiteList.setChecked(true);
                    mBackgroudWhiteList.setEnabled(false);
                    return;
                }
            }
            mBackgroudWhiteList.setChecked(false);
        }
    }
}
