package com.rainman.asf.core;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;

import com.rainman.asf.AppSetting;
import com.rainman.asf.FloatingWindow;

public class DeviceEventReceiver extends BroadcastReceiver {

    private static final String TAG = "DeviceEventReceiver";
    private static DeviceEventReceiver mDeviceEventReceiver = new DeviceEventReceiver();
    private int mOldVolumeValue = -1;

    public static void registerReceiver(Context context) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("android.media.VOLUME_CHANGED_ACTION");
        intentFilter.addAction("android.intent.action.PHONE_STATE");
        intentFilter.addAction("android.intent.action.CONFIGURATION_CHANGED");
        context.registerReceiver(mDeviceEventReceiver, intentFilter);
    }

    public static void unregisterReceiver(Context context) {
        context.unregisterReceiver(mDeviceEventReceiver);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction() != null) {
            switch (intent.getAction()) {
                case "android.media.VOLUME_CHANGED_ACTION":
                    Log.i(TAG, "volume changed");
                    Bundle extras = intent.getExtras();
                    assert extras != null;
                    int volume = (int) extras.get("android.media.EXTRA_VOLUME_STREAM_VALUE");
                    if (AppSetting.isVolumnKeyControl() && mOldVolumeValue != -1) {
                        if (volume < mOldVolumeValue) {
                            ScriptEngine.getInstance().startScript();
                        } else if (volume > mOldVolumeValue) {
                            ScriptEngine.getInstance().stopScript();
                        }
                    }
                    mOldVolumeValue = volume;
                    break;
                case "android.intent.action.PHONE_STATE":
                    Log.i(TAG, "calling");
                    if (AppSetting.isStopWhenCalling()) {
                        ScriptEngine.getInstance().stopScript();
                    }
                    break;
                case "android.intent.action.CONFIGURATION_CHANGED":
                    Log.i(TAG, "rotation changed");
                    ScriptEngine.getInstance().reportDisplayInfo();
                    FloatingWindow.getInstance().rotateScreen();
                    break;
            }
        }
    }
}
