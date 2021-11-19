package com.rainman.asf.util;

import android.app.PendingIntent;
import android.content.Context;
import android.os.Build;
import android.provider.Settings;

public class Compat {

    public static boolean canDrawOverlays(Context context) {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.M || Settings.canDrawOverlays(context);
    }

    public static int getImmutableFlags(int flags) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return flags | PendingIntent.FLAG_IMMUTABLE;
        } else {
            return flags;
        }
    }
}
