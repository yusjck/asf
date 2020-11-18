package com.rainman.asf.core;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.rainman.asf.core.database.CoreDatabase;
import com.rainman.asf.core.database.ScriptLog;
import com.rainman.asf.core.database.ScriptLogDao;

import java.util.Date;
import java.util.LinkedList;
import java.util.List;

public class ScriptLogger {

    private static ScriptLogger mInstance;
    private ScriptLogDao mScriptLogDao;
    private final List<LogListener> mLogListeners = new LinkedList<>();

    public static ScriptLogger getInstance() {
        if (mInstance == null) {
            throw new RuntimeException("init first");
        }
        return mInstance;
    }

    public static void init(Context context) {
        mInstance = new ScriptLogger();
        mInstance.mScriptLogDao = CoreDatabase.getInstance(context).getScriptLogDao();
    }

    static void addLog(int color, final String text) {
        int time = (int) (new Date().getTime() / 1000);
        final ScriptLog log = new ScriptLog(time, color, text);
        if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    getInstance().onScriptLogOutput(log);
                }
            });
            return;
        }
        getInstance().onScriptLogOutput(log);
    }

    static void addLog(String text) {
        addLog(0xffffff, text);
    }

    public static void addError(String text) {
        addLog(0xff0000, text);
    }

    private void addLog(ScriptLog log) {
        mScriptLogDao.insert(log);
    }

    public int getLogCount() {
        return mScriptLogDao.getCount();
    }

    public long getLastLogId() {
        return mScriptLogDao.getMaxId();
    }

    public List<ScriptLog> queryLogInfo(int startIndex, int count) {
        return mScriptLogDao.getPage(startIndex, count);
    }

    public void clearAllLogs() {
        mScriptLogDao.deleteAll();
    }

    public void registerLogListener(LogListener listener) {
        mLogListeners.add(listener);
    }

    public void unregisterLogListener(LogListener listener) {
        mLogListeners.remove(listener);
    }

    void onScriptLogOutput(ScriptLog log) {
        // 将日志信息写入数据库
        addLog(log);

        // 调用已注册的日志监听器
        for (LogListener listener : mLogListeners) {
            listener.onScriptLogOutput(log);
        }
    }

    public interface LogListener {

        void onScriptLogOutput(ScriptLog log);
    }
}
