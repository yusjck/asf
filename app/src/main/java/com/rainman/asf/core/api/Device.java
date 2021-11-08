package com.rainman.asf.core.api;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.KeyguardManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.PowerManager;
import android.telephony.TelephonyManager;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

import androidx.annotation.Keep;

import com.rainman.asf.R;
import com.rainman.asf.util.SystemUtils;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;

@Keep
public class Device {

    private static final String TAG = "Device";
    private final Context mContext;
    private PowerManager.WakeLock mWakeLock;

    public Device(Context context) {
        mContext = context;
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        try {
            mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP, "ASF:Device");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public String getDisplayInfo() throws JSONException {
        Display defaultDisplay = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        int rotation = 0;
        switch (defaultDisplay.getRotation()) {
            case Surface.ROTATION_0:
                rotation = 0;
                break;
            case Surface.ROTATION_90:
                rotation = 90;
                break;
            case Surface.ROTATION_180:
                rotation = 180;
                break;
            case Surface.ROTATION_270:
                rotation = 270;
                break;
        }
        Point displayPoint = new Point();
        defaultDisplay.getRealSize(displayPoint);
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("width", displayPoint.x);
        jsonObject.put("height", displayPoint.y);
        jsonObject.put("rotation", rotation);
        return jsonObject.toString();
    }

    private void checkReadPhoneStatePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (mContext.checkSelfPermission(Manifest.permission.READ_PHONE_STATE)
                    != PackageManager.PERMISSION_GRANTED) {
                throw new SecurityException(mContext.getString(R.string.read_phone_state_permission_denied));
            }
        }
    }

    public String getBrand() {
        return Build.BRAND;
    }

    public String getModel() {
        return Build.MODEL;
    }

    @SuppressLint("MissingPermission")
    public String getIMEI() {
        checkReadPhoneStatePermission();
        TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            return null;
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return tm.getImei();
        } else {
            return tm.getDeviceId();
        }
    }

    public String getWifiSSID() {
        WifiManager manager = (WifiManager) mContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        return manager.getConnectionInfo().getSSID();
    }

    public String getSDCardPath() {
        File externalFilesDir = mContext.getExternalFilesDir(null);
        return externalFilesDir != null ? externalFilesDir.getPath() : null;
    }

    public boolean isDeviceLocked() {
        KeyguardManager keyguardManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        return keyguardManager.isKeyguardLocked() && keyguardManager.isKeyguardSecure();
    }

    public boolean isScreenOn() {
        PowerManager manager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        return manager.isScreenOn();
    }

    public void wakeUp() {
        mWakeLock.acquire(200);
    }

    public void keepScreenOn(int timeout) {
        mWakeLock.acquire(timeout);
    }

    public void cancelKeepingAwake() {
        if (mWakeLock != null && mWakeLock.isHeld())
            mWakeLock.release();
    }

    public void vibrate(int duration) {
        SystemUtils.vibrate(mContext, duration);
    }

    public void cancelVibration() {
        SystemUtils.cancelVibration(mContext);
    }
}
