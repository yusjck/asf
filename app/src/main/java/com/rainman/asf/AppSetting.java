package com.rainman.asf;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Point;

public class AppSetting {

    private static SharedPreferences mSetting;

    public static void init(Context context) {
        mSetting = context.getSharedPreferences("setting", Context.MODE_PRIVATE);
    }

    public static boolean isVibrateWhenStart() {
        return mSetting.getBoolean("vibrate_when_start", true);
    }

    public static boolean isStopWhenCalling() {
        return mSetting.getBoolean("stop_when_calling", true);
    }

    public static boolean isStopWhenWaggling() {
        return mSetting.getBoolean("stop_when_waggling", true);
    }

    public static boolean isVolumnKeyControl() {
        return mSetting.getBoolean("volume_key_control", false);
    }

    public static boolean isRunByRoot() {
        return mSetting.getBoolean("run_by_root", false);
    }

    public static boolean isFloatingWndEnabled() {
        return mSetting.getBoolean("enable_floating_wnd", false);
    }

    public static void setFloatingWndEnabled(boolean enabled) {
        mSetting.edit().putBoolean("enable_floating_wnd", enabled).apply();
    }

    public static boolean isCmdServerEnabled() {
        return mSetting.getBoolean("enable_cmd_server", false);
    }

    public static void setCmdServerEnabled(boolean enabled) {
        mSetting.edit().putBoolean("enable_cmd_server", enabled).apply();
    }

    public static boolean isAccessAuthorizationEnabled() {
        return mSetting.getBoolean("enable_accesss_authorization", true);
    }

    public static void setFloatingWndPos(Point pos) {
        mSetting.edit().putInt("floating_wnd_x", pos.x).apply();
        mSetting.edit().putInt("floating_wnd_y", pos.y).apply();
    }

    public static Point getFloatingWndPos() {
        Point pos = new Point();
        pos.x = mSetting.getInt("floating_wnd_x", 0);
        pos.y = mSetting.getInt("floating_wnd_y", 0);
        return pos;
    }
}
