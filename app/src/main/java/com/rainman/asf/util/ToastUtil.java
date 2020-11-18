package com.rainman.asf.util;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;

public class ToastUtil {

    public static void show(Context context, int resId) {
        show(context, context.getString(resId));
    }

    public static void showLong(Context context, int resId) {
        showLong(context, context.getString(resId));
    }

    public static void show(final Context context, final String text) {
        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
                }
            });
            return;
        }
        Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
    }

    public static void showLong(final Context context, final String text) {
        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(context, text, Toast.LENGTH_LONG).show();
                }
            });
            return;
        }
        Toast.makeText(context, text, Toast.LENGTH_LONG).show();
    }
}
