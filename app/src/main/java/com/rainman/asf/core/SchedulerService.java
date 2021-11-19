package com.rainman.asf.core;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import com.rainman.asf.activity.MainActivity;
import com.rainman.asf.R;
import com.rainman.asf.core.database.CoreDatabase;
import com.rainman.asf.core.database.Scheduler;
import com.rainman.asf.core.database.SchedulerDao;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.util.Compat;
import com.rainman.asf.util.Constant;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.Date;
import java.util.List;

public class SchedulerService extends Service {

    private SchedulerDao mSchedulerDao;
    private ScriptManager mScriptManager;
    private AlarmManager mAlarmManager;
    private PendingIntent mPendingIntent;
    private ScriptManager.ScriptListener mScriptListener;
    private List<Scheduler> mPendingSchedulers;

    public class ServiceBinder extends Binder {
        public SchedulerService getService() {
            return SchedulerService.this;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mSchedulerDao = CoreDatabase.getInstance(this).getSchedulerDao();
        mScriptManager = ScriptManager.getInstance();
        mAlarmManager = (AlarmManager) getSystemService(Service.ALARM_SERVICE);

        Intent intent = new Intent(this, this.getClass());
        intent.setAction("runScript");
        mPendingIntent = PendingIntent.getService(this, 0, intent, Compat.getImmutableFlags(0));

        createNotificationChannel();
        reloadPendingSchedulerList();

        mScriptListener = new ScriptManager.ScriptListener() {
            @Override
            public void onScriptListUpdated() {

            }

            @Override
            public void onScriptSwitched() {

            }

            @Override
            public void onScriptStateChanged(int state, boolean exceptFlag) {
                if (state == ScriptActuator.ScriptState.SCRIPT_STOPPED) {
                    mScriptManager.unregisterScriptListener(this);
                    // 当脚本停止时判断列表中是否存在已到执行时间的计划，是就触发下一个执行计划
                    if (mPendingSchedulers.size() > 0) {
                        Scheduler scheduler = mPendingSchedulers.get(0);
                        if (new Date().after(scheduler.getRunTime())) {
                            try {
                                mPendingIntent.send();
                            } catch (PendingIntent.CanceledException e) {
                                e.printStackTrace();
                            }
                            return;
                        }
                    }
                    // 没有需要立刻执行的计划，重新加载计划列表
                    reloadPendingSchedulerList();
                }
            }
        };
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mAlarmManager.cancel(mPendingIntent);
        hideNotification();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return new ServiceBinder();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent == null ? null : intent.getAction();
        if (action != null) {
            switch (action) {
                case "runScript":
                    if (mPendingSchedulers.size() > 0 && mScriptManager.getRunningScriptId() == 0) {
                        Scheduler scheduler = mPendingSchedulers.get(0);
                        // 禁用仅执行一次的计划
                        if (scheduler.getRepeat() == 0) {
                            scheduler.setEnabled(false);
                            mSchedulerDao.addOrUpdate(scheduler);
                        }

                        // 切换到计划指定的脚本
                        if (mScriptManager.switchScript(scheduler.getScriptId())) {
                            mScriptManager.registerScriptListener(mScriptListener);
                            // 启动目标脚本
                            ScriptLogger.addLog(getString(R.string.start_scheduler));
                            if (scheduler.isConfigEnabled()) {
                                ScriptActuator.getInstance().startScript(scheduler.getConfigName());
                            } else {
                                ScriptActuator.getInstance().startScript();
                            }
                        }

                        hideNotification();
                        // 把当前执行的计划从等待列表中移除
                        mPendingSchedulers.remove(0);
                    }
                    break;
                case "deleteScriptScheduler":
                    long scriptId = intent.getLongExtra("script_id", 0);
                    deleteSchedulersByScriptId(scriptId);
                    break;
            }
        }
        return super.onStartCommand(intent, flags, startId);
    }

    public void addOrUpdateScheduler(Scheduler scheduler) {
        long id = mSchedulerDao.addOrUpdate(scheduler);
        if (scheduler.getId() == 0) {
            scheduler.setId(id);
        }
        reloadPendingSchedulerList();
    }

    public void deleteScheduler(Scheduler scheduler) {
        mSchedulerDao.delete(scheduler);
        reloadPendingSchedulerList();
    }

    public void deleteSchedulersByScriptId(long scriptId) {
        if (mSchedulerDao.deleteByScriptId(scriptId) > 0) {
            reloadPendingSchedulerList();
        }
    }

    public Scheduler findSchedulerById(long id) {
        return mSchedulerDao.findById(id);
    }

    public List<Scheduler.SchedulerInfo> getSchedulersByScriptId(long scriptId) {
        return mSchedulerDao.getSchedulersByScriptId(scriptId);
    }

    public List<Scheduler.SchedulerInfo> getAllSchedulers() {
        return mSchedulerDao.getAllSchedulers();
    }

    private void reloadPendingSchedulerList() {
        mAlarmManager.cancel(mPendingIntent);
        hideNotification();

        // 初始化计划列表
        mPendingSchedulers = mSchedulerDao.getEnabledSchedulers();
        for (Scheduler scheduler : mPendingSchedulers) {
            scheduler.setRunTime(scheduler.getNextRunTime());
        }

        // 通过计划执行时间对列表中的计划进行排序
        Collections.sort(mPendingSchedulers, (o1, o2) -> (int) (o1.getRunTime().getTime() - o2.getRunTime().getTime()));

        if (mPendingSchedulers.size() > 0) {
            Scheduler scheduler = mPendingSchedulers.get(0);
            long runTime = scheduler.getRunTime().getTime();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                mAlarmManager.setAlarmClock(new AlarmManager.AlarmClockInfo(runTime, mPendingIntent), mPendingIntent);
            } else {
                mAlarmManager.setExact(AlarmManager.RTC_WAKEUP, runTime, mPendingIntent);
            }
            SimpleDateFormat sdf = (SimpleDateFormat) DateFormat.getDateTimeInstance();
            sdf.applyPattern("MM/dd HH:mm");
            String info = String.format(getString(R.string.scheduler_next_run_time), sdf.format(runTime));
            ScriptLogger.addLog(info);
            Script script = ScriptManager.getInstance().findScriptById(scheduler.getScriptId());
            assert script != null;
            info = String.format(getString(R.string.script_scheduler_tip), sdf.format(runTime), script.getName());
            showNotification(info, getString(R.string.script_scheduler_tip2));
        }
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= 26) {
            NotificationChannel chan = new NotificationChannel("ScriptScheduler", getText(R.string.script_scheduler), NotificationManager.IMPORTANCE_LOW);
            chan.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
            NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            manager.createNotificationChannel(chan);
        }
    }

    private void showNotification(String title, String text) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, "ScriptScheduler");
        builder.setWhen(System.currentTimeMillis());
        builder.setSmallIcon(R.mipmap.ic_launcher);
        builder.setContentTitle(title);
        builder.setContentText(text);
        builder.setPriority(NotificationCompat.PRIORITY_LOW);
        builder.setAutoCancel(true);

        Intent intent = new Intent(this, MainActivity.class);
        intent.setAction("showSchedulers");
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 2, intent, Compat.getImmutableFlags(PendingIntent.FLAG_UPDATE_CURRENT));

        builder.setContentIntent(pendingIntent);
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(Constant.NOTIFICATION_KIND_2, builder.build());
    }

    private void hideNotification() {
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.cancel(Constant.NOTIFICATION_KIND_2);
    }
}
