package com.rainman.asf.core.api;

import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.util.Base64;

import androidx.annotation.Keep;
import androidx.core.app.NotificationCompat;

import com.rainman.asf.R;
import com.rainman.asf.accessibility.AccessibilityHelperService;
import com.rainman.asf.activity.MainActivity;
import com.rainman.asf.core.ScriptLogger;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.core.screenshot.ScreenCaptureService;
import com.rainman.asf.util.FileUtil;
import com.rainman.asf.util.RootUtil;
import com.rainman.asf.util.SystemUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Objects;

@Keep
public class System {

    private static final String TAG = "System";
    private final Context mContext;

    public System(Context context) {
        mContext = context;
        createNotificationChannel();
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= 26) {
            NotificationChannel chan = new NotificationChannel("ScriptNotification", mContext.getText(R.string.script_notification), NotificationManager.IMPORTANCE_DEFAULT);
            NotificationManager manager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
            manager.createNotificationChannel(chan);
        }
    }

    public String getAsfVersion() {
        PackageManager packageManager = mContext.getPackageManager();
        try {
            PackageInfo packageInfo = packageManager.getPackageInfo(mContext.getPackageName(), 0);
            return packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return "";
    }

    public String getAppVersion() {
        return "1.4.5";
    }

    public boolean isCaptureAvailable() {
        return ScreenCaptureService.getInstance() != null;
    }

    public boolean isAccessibilityAvailable() {
        return AccessibilityHelperService.getInstance() != null;
    }

    public void showMessage(String msg) {
        SystemUtils.showMessage(mContext, msg);
    }

    public void pushNotification(String msg) {
        Script script = ScriptManager.getInstance().getCurrentScript();
        Intent intent = new Intent(mContext, MainActivity.class);
        intent.setAction("showLogs");
        intent.putExtra("init_pos", ScriptLogger.getInstance().getLogCount() - 1);
        PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 10 + (int) script.getId(), intent, PendingIntent.FLAG_UPDATE_CURRENT);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(mContext, "ScriptNotification")
                .setContentTitle(script.getName())
                .setContentText(msg)
                .setWhen(new Date().getTime())
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .setAutoCancel(true);

        Notification notification = builder.build();
        NotificationManager manager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(10 + (int) script.getId(), notification);
    }

    public void removeNotification() {
        Script script = ScriptManager.getInstance().getCurrentScript();
        NotificationManager manager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.cancel(10 + (int) script.getId());
    }

    public boolean startActivity(String uri) {
        try {
            Intent intent = new Intent();
            intent.setData(Uri.parse(uri));
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mContext.startActivity(intent);
            return true;
        } catch (ActivityNotFoundException e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean checkAppInstalled(String pkgName) {
        PackageInfo packageInfo;
        try {
            packageInfo = mContext.getPackageManager().getPackageInfo(pkgName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            packageInfo = null;
            e.printStackTrace();
        }
        return packageInfo != null;
    }

    public boolean startApp(String pkgName, String clsName) {
        if (!clsName.equals("")) {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
            ComponentName componentName = new ComponentName(pkgName, clsName);
            intent.setComponent(componentName);
            mContext.startActivity(intent);
        } else {
            Intent intent = mContext.getPackageManager().getLaunchIntentForPackage(pkgName);
            mContext.startActivity(intent);
        }
        return true;
    }

    public void killBackgroundApp(String pkgName) {
        ActivityManager manager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        manager.killBackgroundProcesses(pkgName);
    }

    public boolean isHome() {
        List<String> names = new ArrayList<>();
        PackageManager packageManager = mContext.getPackageManager();
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);
        List<ResolveInfo> resolveInfo = packageManager.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY);
        for (ResolveInfo ri : resolveInfo) {
            names.add(ri.activityInfo.packageName);
        }
        ActivityManager mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningTaskInfo> rti = mActivityManager.getRunningTasks(1);
        return names.contains(Objects.requireNonNull(rti.get(0).topActivity).getPackageName());
    }

    public int getSdkInt() {
        return Build.VERSION.SDK_INT;
    }

    public boolean haveRoot() {
        return RootUtil.haveRoot();
    }

    public String getClipText() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        ClipboardManager cm = (ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = cm.getPrimaryClip();
        if (clipData != null) {
            ClipData.Item item = clipData.getItemAt(0);
            if (item != null) {
                return item.getText().toString();
            }
        }
        return "";
    }

    public void setClipText(String text) {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        ClipboardManager cm = (ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = ClipData.newPlainText("Label", text);
        cm.setPrimaryClip(clipData);
    }

    public String readFile(String filePath) {
        byte[] content = FileUtil.readFile(new File(filePath));
        if (content == null)
            return "";
        return Base64.encodeToString(content, Base64.NO_WRAP);
    }

    public boolean writeFile(String filePath, String content) {
        File file = new File(filePath);
        File parentFile = file.getParentFile();
        if (parentFile != null) {
            if (!parentFile.exists() && !parentFile.mkdirs())
                return false;
        }
        return FileUtil.writeFile(file, Base64.decode(content, Base64.NO_WRAP));
    }
}
