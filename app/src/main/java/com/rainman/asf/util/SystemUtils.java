package com.rainman.asf.util;

import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.*;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.*;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;

import android.text.TextUtils;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.rainman.asf.R;

import dalvik.system.PathClassLoader;

import java.io.*;
import java.util.Locale;
import java.util.Objects;

public class SystemUtils {

    private static final String TAG = "SystemUtils";

    public static boolean putAssetsFile(Context context, String fileName, File saveFile) {
        InputStream inputStream = null;
        FileOutputStream outputStream = null;
        try {
            inputStream = context.getAssets().open(fileName);
            outputStream = new FileOutputStream(saveFile);
            byte[] buf = new byte[1024];
            int len;
            while ((len = inputStream.read(buf)) != -1) {
                outputStream.write(buf, 0, len);
            }
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static boolean putAssetsFile(Context context, String fileName, String savePath) {
        File saveFile = new File(savePath);
        if (saveFile.isDirectory())
            saveFile = new File(savePath, fileName);
        return putAssetsFile(context, fileName, saveFile);
    }

    /**
     * 将assets中的文件释放到cache目录中并返回缓存文件路径
     */
    public static String getAssetsCacheFile(Context context, String fileName) {
        String cacheFileName = fileName;
        int fileNameSp = fileName.lastIndexOf(File.separatorChar);
        if (fileNameSp != -1) {
            cacheFileName = fileName.substring(fileNameSp + 1);
        }
        File cacheFile = new File(context.getCacheDir(), cacheFileName);
        if (!putAssetsFile(context, fileName, cacheFile))
            return null;
        return cacheFile.getAbsolutePath();
    }

    public static String findNativeLibraryPath(Context context, String libraryName) {
        if (context == null) {
            return null;
        }

        if (TextUtils.isEmpty(libraryName)) {
            return null;
        }

        PathClassLoader classLoader = (PathClassLoader) context.getClassLoader();
        return classLoader.findLibrary(libraryName);
    }

    public static void openImage(Context context, String imagePath) {
        Intent photoIntent = new Intent();
        File file = new File(imagePath);
        Uri uri;
        photoIntent.setAction(Intent.ACTION_VIEW);
        if (Build.VERSION.SDK_INT >= 24) {
            uri = FileProvider.getUriForFile(context, context.getPackageName() + ".provider", file);
            photoIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } else {
            uri = Uri.fromFile(file);
        }
        photoIntent.setDataAndType(uri, "image/*");
        context.startActivity(photoIntent);
    }

    public static void messageBox(final Context context, String msg, int timeout) {
        if (Build.VERSION.SDK_INT >= 23) {
            if (!Settings.canDrawOverlays(context))
                return;
        }

        final Object signal = new Object();
        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(R.string.script_prompt)
                .setMessage(msg)
                .setPositiveButton(R.string.dlg_confirm, (dialog, which) -> dialog.cancel())
                .setCancelable(false)
                .setOnCancelListener(dialog -> {
                    synchronized (signal) {
                        signal.notify();
                    }
                });

        Handler handler = new Handler(Looper.getMainLooper()) {
            private AlertDialog mDialog = null;

            @Override
            public void handleMessage(@NonNull Message msg) {
                super.handleMessage(msg);

                if (mDialog == null) {
                    Log.i(TAG, "current thread id=" + Thread.currentThread().getId());
                    // 创建并显示脚本提示框
                    mDialog = builder.create();
                    Window dialogWindow = mDialog.getWindow();
                    if (dialogWindow != null) {
                        dialogWindow.setType(Build.VERSION.SDK_INT >= 26 ?
                                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                        dialogWindow.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    }
                    mDialog.show();
                }

                if (msg.what != -1) {
                    // 显示倒计时和超时处理
                    if (msg.what < 500) {
                        mDialog.cancel();
                    } else {
                        mDialog.setTitle(String.format(Locale.getDefault(), "%s(%d)", context.getString(R.string.script_prompt), (msg.what + 500) / 1000));
                        Message message = Message.obtain(this);
                        message.what = msg.what - 500;
                        sendMessageDelayed(message, 500);
                    }
                }
            }
        };

        Message message = Message.obtain(handler);
        message.what = timeout;
        handler.sendMessage(message);

        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
            // 等待用户点击提示框
            synchronized (signal) {
                try {
                    signal.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static void showMessage(final Context context, final String msg) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(() -> Toast.makeText(context, msg, Toast.LENGTH_SHORT).show());
    }

    public static void vibrate(Context context, int duration) {
        Vibrator vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        vibrator.vibrate(duration);
    }

    public static void cancelVibration(Context context) {
        Vibrator vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        vibrator.cancel();
    }

    public static boolean isScreenOn(Context context) {
        PowerManager manager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        return manager.isScreenOn();
    }

    public static void setClipboardText(Context context, String text) {
        ClipboardManager manager = (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        manager.setPrimaryClip(ClipData.newPlainText("", text));
    }

    public static String getForegroundPackage(Context context) {
        ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        return Objects.requireNonNull(manager.getRunningTasks(1).get(0).topActivity).getPackageName();
    }

    public static String getWifiIp(Context context) {
        WifiManager manager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        int ipAddress = manager.getConnectionInfo().getIpAddress();
        return (ipAddress & 0xFF) + "." + (ipAddress >> 8 & 0xFF) + "." + (ipAddress >> 16 & 0xFF) + "." + (ipAddress >> 24 & 0xFF);
    }

    public static boolean canDrawOverlays(Context context) {
        return Build.VERSION.SDK_INT < 23 || Settings.canDrawOverlays(context);
    }
}
