package com.rainman.asf.core;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import com.rainman.asf.R;
import com.rainman.asf.activity.MainActivity;
import com.rainman.asf.activity.OptionActivity;
import com.rainman.asf.core.database.Script;

import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

public class ForegroundService extends Service {

    private ScriptManager.ScriptListener mScriptListener;
    private ScriptManager mScriptManager;
    private NotificationCompat.Builder mNotificationBuilder;
    private boolean mTicking = false;
    private long mStartTime = 0;
    private long mTotalRunTime = 0;
    private Timer mNotificationUpdateTimer;

    @Override
    public void onCreate() {
        super.onCreate();

        mScriptListener = new ScriptManager.ScriptListener() {
            @Override
            public void onScriptListUpdated() {
                updateNotification();
            }

            @Override
            public void onScriptSwitched() {
                mTotalRunTime = 0;
                updateNotification();
            }

            @Override
            public void onScriptStateChanged(int state, boolean exceptFlag) {
                if (state == ScriptActuator.ScriptState.SCRIPT_STOPPED) {
                    stopTick();
                } else {
                    startTick();
                }
            }
        };
        mScriptManager = ScriptManager.getInstance();
        mScriptManager.registerScriptListener(mScriptListener);

        createNotificationChannel();
        mNotificationBuilder = createNotificationBuilder();
        updateNotification();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if ("startScript".equals(intent.getAction())) {
            collapseStatusBar();
            ScriptActuator.getInstance().startScript();
        } else if ("stopScript".equals(intent.getAction())) {
            collapseStatusBar();
            ScriptActuator.getInstance().stopScript();
        } else if ("setting".equals(intent.getAction())) {
            collapseStatusBar();
            Script script = mScriptManager.getCurrentScript();
            if (script != null) {
                Intent intent1 = new Intent(this, OptionActivity.class);
                intent1.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent1.putExtra("script_id", script.getId());
                startActivity(intent1);
            }
        }
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mScriptManager.unregisterScriptListener(mScriptListener);
        removeNotification();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void startTick() {
        if (!mTicking) {
            mStartTime = System.currentTimeMillis();
            mTicking = true;
            TimerTask timerTask = new TimerTask() {
                @Override
                public void run() {
                    updateNotification();
                }
            };
            mNotificationUpdateTimer = new Timer();
            mNotificationUpdateTimer.schedule(timerTask, 0, 1000);
        }
    }

    private void stopTick() {
        if (mTicking) {
            mTicking = false;
            mNotificationUpdateTimer.cancel();
            mTotalRunTime = System.currentTimeMillis() - mStartTime;
            mStartTime = 0;
            updateNotification();
        }
    }

    private String getTotalRunningTime() {
        long time = mTotalRunTime;
        if (mStartTime > 0) {
            time = (System.currentTimeMillis() - mStartTime);
        }
        long hour = time / 3600 / 1000;
        long minute = (time / 60 / 1000) % 60;
        long second = (time / 1000) % 60;
        return String.format(Locale.getDefault(), "%02d:%02d:%02d", hour, minute, second);
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= 26) {
            NotificationChannel chan = new NotificationChannel("ScriptControl", getText(R.string.script_control), NotificationManager.IMPORTANCE_LOW);
            chan.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            manager.createNotificationChannel(chan);
        }
    }

    private NotificationCompat.Builder createNotificationBuilder() {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, "ScriptControl");
        builder.setWhen(0);
        builder.setSmallIcon(R.mipmap.ic_launcher);
        builder.setPriority(NotificationCompat.PRIORITY_MIN);
        Intent intent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 1, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.setContentIntent(pendingIntent);

        intent = new Intent(this, ForegroundService.class);
        intent.setAction("startScript");
        pendingIntent = PendingIntent.getService(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.addAction(R.drawable.ic_start, getString(R.string.start_script), pendingIntent);

        intent = new Intent(this, ForegroundService.class);
        intent.setAction("stopScript");
        pendingIntent = PendingIntent.getService(this, 1, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.addAction(R.drawable.ic_stop, getString(R.string.stop_script), pendingIntent);

        intent = new Intent(this, ForegroundService.class);
        intent.setAction("setting");
        pendingIntent = PendingIntent.getService(this, 2, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        builder.addAction(R.drawable.ic_script_setting, getString(R.string.script_setting), pendingIntent);
        return builder;
    }

    private void updateNotification() {
        Script script = mScriptManager.getCurrentScript();
        if (script != null) {
            mNotificationBuilder.setContentTitle(script.getName());
            if (!script.getIconPath().isEmpty()) {
                mNotificationBuilder.setLargeIcon(BitmapFactory.decodeFile(script.getIconPath()));
            } else {
                mNotificationBuilder.setLargeIcon(null);
            }
        } else {
            mNotificationBuilder.setContentTitle(getString(R.string.no_script_selected));
            mNotificationBuilder.setLargeIcon(null);
        }

        String scriptStatusTip;
        if (mScriptManager.getScriptState() == ScriptActuator.ScriptState.SCRIPT_STOPPED) {
            scriptStatusTip = String.format(getString(R.string.script_state_tip), getString(R.string.script_state_stopped), getTotalRunningTime());
        } else {
            scriptStatusTip = String.format(getString(R.string.script_state_tip), getString(R.string.script_state_running), getTotalRunningTime());
        }
        mNotificationBuilder.setContentText(scriptStatusTip);
        startForeground(1, mNotificationBuilder.build());
    }

    private void removeNotification() {
        stopForeground(true);
    }

    private void collapseStatusBar() {
        sendBroadcast(new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
    }
}
