package com.rainman.asf;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;

import java.util.LinkedList;
import java.util.List;

public class DeviceEvent extends BroadcastReceiver {

    private static final String TAG = "DeviceEvent";
    private static final DeviceEvent mInstance = new DeviceEvent();
    private final List<EventListener> mEventListeners = new LinkedList<>();
    private int mOldVolumeValue = -1;

    public static class EventListener {

        public void onRotationChanged() {

        }

        public void onCalling() {

        }

        public void onVolumnUp() {

        }

        public void onVolumnDown() {

        }
    }

    public static void registerListener(EventListener listener) {
        mInstance.mEventListeners.add(listener);
    }

    public static void unregisterListener(EventListener listener) {
        mInstance.mEventListeners.remove(listener);
    }

    public static void registerReceiver(Context context) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("android.media.VOLUME_CHANGED_ACTION");
        intentFilter.addAction("android.intent.action.PHONE_STATE");
        intentFilter.addAction("android.intent.action.CONFIGURATION_CHANGED");
        context.registerReceiver(mInstance, intentFilter);
    }

    public static void unregisterReceiver(Context context) {
        context.unregisterReceiver(mInstance);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction() != null) {
            switch (intent.getAction()) {
                case "android.media.VOLUME_CHANGED_ACTION":
                    Log.i(TAG, "volume changed");
                    Bundle extras = intent.getExtras();
                    assert extras != null;
                    Integer volume = (Integer) extras.get("android.media.EXTRA_VOLUME_STREAM_VALUE");
                    assert volume != null;
                    if (mOldVolumeValue != -1) {
                        if (volume < mOldVolumeValue) {
                            onVolumnDown();
                        } else if (volume > mOldVolumeValue) {
                            onVolumnUp();
                        }
                    }
                    mOldVolumeValue = volume;
                    break;
                case "android.intent.action.PHONE_STATE":
                    Log.i(TAG, "calling");
                    onCalling();
                    break;
                case "android.intent.action.CONFIGURATION_CHANGED":
                    Log.i(TAG, "rotation changed");
                    onRotationChanged();
                    break;
            }
        }
    }

    private void onRotationChanged() {
        for (EventListener listener : mEventListeners) {
            listener.onRotationChanged();
        }
    }

    private void onCalling() {
        for (EventListener listener : mEventListeners) {
            listener.onCalling();
        }
    }

    private void onVolumnUp() {
        for (EventListener listener : mEventListeners) {
            listener.onVolumnUp();
        }
    }

    private void onVolumnDown() {
        for (EventListener listener : mEventListeners) {
            listener.onVolumnDown();
        }
    }
}
