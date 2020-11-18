package com.rainman.asf.core;

import android.app.Application;
import android.content.Context;
import android.graphics.Point;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import com.rainman.asf.AppSetting;
import com.rainman.asf.R;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.core.database.ScriptLog;
import com.rainman.asf.userconfig.UserVar;
import com.rainman.asf.util.ConfigUtil;
import com.rainman.asf.util.RootUtil;
import com.rainman.asf.util.SystemUtils;
import com.rainman.asf.util.ToastUtil;

import java.io.File;
import java.util.Date;

public class ScriptEngine {

    private static final String TAG = "ScriptEngine";
    private static final int MSG_UPDATE_SCRIPT_STATE = 1;
    private static final int MSG_OUTPUT_SCRIPT_LOG = 2;
    private static final int MSG_UPDATE_CMD_SERVER_STATE = 3;
    private static final int MSG_ENGINE_DISCONNECTED = 4;

    private static ScriptEngine mInstance;
    private Application mApp;
    private String mEngineWorkDir;
    private boolean mRootModeEnabled;
    private int mEngineState = ENGINE_DISCONNECTED;
    private EngineStateListener mEngineStateListener;

    private Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);

            switch (msg.what) {
                case MSG_UPDATE_SCRIPT_STATE:
                    onScriptStateChanged(msg);
                    break;
                case MSG_OUTPUT_SCRIPT_LOG:
                    onScriptLogOutput(msg);
                    break;
                case MSG_UPDATE_CMD_SERVER_STATE:
                    onCmdServerStateChanged(msg);
                    break;
                case MSG_ENGINE_DISCONNECTED:
                    onEngineDisconnected();
                    break;
            }
        }
    };

    public static ScriptEngine getInstance() {
        if (mInstance == null) {
            throw new RuntimeException("init first");
        }
        return mInstance;
    }

    public Context getContext() {
        return mApp;
    }

    public static void init(Application app) {
        mInstance = new ScriptEngine();
        mInstance.mApp = app;
    }

    public void startup() {
        mEngineWorkDir = mApp.getCacheDir().toString();

        initNativeLib();
        nativeLoadEngine(mEngineWorkDir);
        reconnectNativeEngine();

        Log.i(TAG, "script engine started");
    }

    public void cleanup() {
        disconnectEngine();
        nativeUnloadEngine();
        Log.i(TAG, "script engine stopped");
    }

    static {
        System.loadLibrary("native-lib");
    }

    private native void initNativeLib();

    private native boolean connectEngine(String connName);

    private native void disconnectEngine();

    private native boolean runScript(String scriptCfg, String userVar);

    private native void abort();

    private native void reportDisplayInfo(int width, int height, int rotation);

    private native void enableCmdServer(boolean enable);

    private native void sendEvent(String event);

    private native void setPluginDir(String pluginDir);

    private native void nativeLoadEngine(String workDir);

    private native void nativeUnloadEngine();

    private void loadNativeEngineByRoot() {
        Log.i(TAG, "CPU_ABI=" + Build.CPU_ABI);
        String minicapResPath = "minicap/android-" + Build.VERSION.SDK_INT + "/" + Build.CPU_ABI + "/minicap.so";
        String minicapPath = SystemUtils.getAssetsCacheFile(mApp, minicapResPath);
        String cmdhostResPath = "cmdhost/" + Build.CPU_ABI + "/cmdhost";
        String cmdhostPath = SystemUtils.getAssetsCacheFile(mApp, cmdhostResPath);
        String enginePath = SystemUtils.findNativeLibraryPath(mApp, "engine");
        String soDir = enginePath.substring(0, enginePath.lastIndexOf(File.separator));

        String cmd = "killall cmdhost\n";
        if (minicapPath != null) {
            cmd += "chmod 777 " + minicapPath + "\n";
        }
        cmd += "chmod 777 " + cmdhostPath + "\n";
        cmd += "LD_LIBRARY_PATH=" + soDir + "\n";
        cmd += "nohup " + cmdhostPath + " " + enginePath + " " + mEngineWorkDir + " > /dev/null &";
        Log.i(TAG, cmd);
        RootUtil.executeRootCmdSilent(cmd);
    }

    private void connectNativeEngine(final String connName, long waitTime) {
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (!connectEngine(connName)) {
                    changeEngineState(ENGINE_CONNECTION_FAIL);
                } else {
                    changeEngineState(ENGINE_CONNECTED);
                    setPluginDir(ScriptEnvironment.getPluginDir());
                    switchCmdServer();
                }
            }
        }, waitTime);
    }

    public void reconnectNativeEngine() {
        disconnectEngine();
        changeEngineState(ENGINE_DISCONNECTED);
        if (AppSetting.isRunByRoot()) {
            loadNativeEngineByRoot();
            connectNativeEngine("@asf_engine_by_root", 1000);
            mRootModeEnabled = true;
        } else {
            connectNativeEngine("@asf_engine", 100);
            mRootModeEnabled = false;
        }
    }

    public void switchEngine() {
        boolean enabled = AppSetting.isRunByRoot();
        if (enabled != mRootModeEnabled) {
            if (enabled) {
                ToastUtil.show(mApp, R.string.run_by_root_enabled);
            } else {
                ToastUtil.show(mApp, R.string.run_by_root_disabled);
            }
            reconnectNativeEngine();
            mRootModeEnabled = enabled;
        }
    }

    public void switchCmdServer() {
        enableCmdServer(AppSetting.isCmdServerEnabled());
    }

    public static final int ENGINE_CONNECTED = 0;
    public static final int ENGINE_DISCONNECTED = 1;
    public static final int ENGINE_CONNECTION_FAIL = -1;
    public static final int ENGINE_CONNECTION_LOSE = -2;

    public interface EngineStateListener {
        void onEngineStateChanged(int engineState);
    }

    public void setEngineStateListener(EngineStateListener listener) {
        mEngineStateListener = listener;
        if (mEngineStateListener != null) {
            mEngineStateListener.onEngineStateChanged(mEngineState);
        }
    }

    private void changeEngineState(int engineState) {
        mEngineState = engineState;
        if (mEngineStateListener != null) {
            mEngineStateListener.onEngineStateChanged(engineState);
        }
    }

    public void startScript() {
        startScript(null);
    }

    public void startScript(String configName) {
        ScriptManager scriptManager = ScriptManager.getInstance();
        Script script = scriptManager.getCurrentScript();
        if (script == null) {
            ToastUtil.show(mApp, R.string.script_not_exist);
            return;
        }

        if (scriptManager.getRunningScriptId() != 0) {
            ToastUtil.show(mApp, R.string.script_already_running);
            return;
        }

        ConfigUtil configs = new ConfigUtil();
        configs.setString("ScriptDir", script.getScriptDir());
        configs.setString("TempDir", ScriptEnvironment.getTempDir());
        configs.setString("PluginDir", ScriptEnvironment.getPluginDir());
        configs.setString("LogDir", ScriptEnvironment.getLogDir());
        configs.setString("MainPath", script.getMainPath());

        reportDisplayInfo();

        UserVar userVar = new UserVar(new File(script.getScriptDir()), configName);
        boolean success = runScript(configs.toConfigString(), userVar.getUserVarString());
        if (success) {
            String log = String.format(mApp.getString(R.string.start_script_desc), script.getName());
            if (mRootModeEnabled)
                log += "[ROOT]";
            ScriptLogger.addLog(0xaaaaaa, log);
            if (AppSetting.isVibrateWhenStart()) {
                SystemUtils.vibrate(mApp, 300);
            }
        } else {
            ToastUtil.show(mApp, R.string.script_start_failed);
        }
    }

    void reportDisplayInfo() {
        Display defaultDisplay = ((WindowManager) mApp.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
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
        reportDisplayInfo(displayPoint.x, displayPoint.y, rotation);
    }

    public void stopScript() {
        abort();
    }

    public static class ScriptState {
        public static final int SCRIPT_STARTING = 0;
        public static final int SCRIPT_RUNNING = 1;
        public static final int SCRIPT_STOPPING = 2;
        public static final int SCRIPT_STOPPED = 3;
        public static final int SCRIPT_ALERTING = 4;
        public static final int SCRIPT_BLOCKING = 5;
        public static final int SCRIPT_SUSPENDING = 6;
        public static final int SCRIPT_RESETING = 7;
    }

    public static final int SCRIPT_EXCEPTION_FLAG = 1;
    public static final int SCRIPT_TASK_COMPLETION_FLAG = 2;
    public static final int SCRIPT_TASK_FAILURE_FLAG = 4;
    public static final int SCRIPT_TASK_PENDING_FLAG = 8;
    public static final int SCRIPT_TASK_FLAG_MASK = SCRIPT_TASK_COMPLETION_FLAG | SCRIPT_TASK_FAILURE_FLAG | SCRIPT_TASK_PENDING_FLAG;

    private void onScriptStateChanged(Message msg) {
        int state = msg.arg1;
        int flags = msg.arg2;
        boolean exceptFlag = (flags & SCRIPT_EXCEPTION_FLAG) != 0;
        ScriptManager scriptManager = ScriptManager.getInstance();

        if (state == ScriptState.SCRIPT_STARTING) {
            Script script = scriptManager.getCurrentScript();
            script.setLastRunTime(new Date().getTime());
            scriptManager.updateScript(script);
        } else if (state == ScriptState.SCRIPT_STOPPED) {
            Script script = scriptManager.getCurrentScript();
            if (exceptFlag) {
                script.setLastQuitReason("exception");
                scriptManager.updateScript(script);
            } else if ((flags & SCRIPT_TASK_FLAG_MASK) != 0) {
                if ((flags & SCRIPT_TASK_COMPLETION_FLAG) != 0) {
                    script.setLastQuitReason("completion");
                } else if ((flags & SCRIPT_TASK_FAILURE_FLAG) != 0) {
                    script.setLastQuitReason("failure");
                } else if ((flags & SCRIPT_TASK_PENDING_FLAG) != 0) {
                    script.setLastQuitReason("pending");
                }
                scriptManager.updateScript(script);
            } else {
                script.setLastQuitReason("");
                scriptManager.updateScript(script);
            }
        }

        scriptManager.onScriptStateChanged(state, exceptFlag);

        switch (state) {
            case ScriptState.SCRIPT_RESETING:
                ScriptLogger.addLog(0xaaaaaa, mApp.getString(R.string.script_stopping));
                break;
            case ScriptState.SCRIPT_STOPPED:
                ScriptLogger.addLog(0xaaaaaa, mApp.getString(R.string.script_stopped));
                if (exceptFlag) {
                    ToastUtil.show(mApp, R.string.script_exception);
                } else {
                    ToastUtil.show(mApp, R.string.script_stopped);
                }
                break;
        }
    }

    private void onScriptLogOutput(Message msg) {
        ScriptLog log = (ScriptLog) msg.obj;
        ScriptLogger.getInstance().onScriptLogOutput(log);
    }

    private static final int CMDSERVER_STARTED = 1;
    private static final int CMDSERVER_START_FAILED = 2;
    private static final int CMDSERVER_STOPPED = 3;

    private void onCmdServerStateChanged(Message msg) {
        int state = msg.arg1;
        switch (state) {
            case CMDSERVER_STARTED:
                ToastUtil.show(mApp, R.string.cmd_server_started);
                break;
            case CMDSERVER_START_FAILED:
                ToastUtil.show(mApp, R.string.cmd_server_start_failed);
                break;
            case CMDSERVER_STOPPED:
                ToastUtil.show(mApp, R.string.cmd_server_stopped);
                break;
        }
    }

    private void onEngineDisconnected() {
        if (mEngineState == ENGINE_CONNECTED) {
            changeEngineState(ENGINE_CONNECTION_LOSE);
        }
    }

    /**
     * 引擎回调接口，用于报告当前脚本运行状态
     *
     * @param state 脚本状态
     * @param flags 异常标志
     */
    @Keep
    private void updateScriptState(int state, int flags) {
        Message msg = Message.obtain();
        msg.what = MSG_UPDATE_SCRIPT_STATE;
        msg.arg1 = state;
        msg.arg2 = flags;
        mHandler.sendMessage(msg);
    }

    /**
     * 引擎回调接口，用于输出当前脚本日志
     *
     * @param time  日间
     * @param color 显示颜色
     * @param text  内容
     */
    @Keep
    private void outputScriptLog(int time, int color, String text) {
        ScriptLog log = new ScriptLog(time, color, text);

        Message msg = Message.obtain();
        msg.what = MSG_OUTPUT_SCRIPT_LOG;
        msg.obj = log;
        mHandler.sendMessage(msg);
    }

    /**
     * 引擎回调接口，用于报告CmdServer当前的状态
     *
     * @param state 当前CmdServer状态
     */
    @Keep
    private void updateCmdServerState(int state) {
        Message msg = Message.obtain();
        msg.what = MSG_UPDATE_CMD_SERVER_STATE;
        msg.arg1 = state;
        mHandler.sendMessage(msg);
    }

    /**
     * 引擎回调接口，脚本引擎断开时调用
     */
    @Keep
    private void engineDisconnectNotify() {
        Message msg = Message.obtain();
        msg.what = MSG_ENGINE_DISCONNECTED;
        mHandler.sendMessage(msg);
    }
}
