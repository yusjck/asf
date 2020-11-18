package com.rainman.asf.core;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.Nullable;

import com.rainman.asf.R;
import com.rainman.asf.core.database.CoreDatabase;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.core.database.ScriptDao;
import com.rainman.asf.util.FileUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class ScriptManager {

    private static ScriptManager mInstance;
    private ScriptDao mScriptDao;
    private SharedPreferences mSettingData;
    private final List<Script> mScripts = new ArrayList<>();
    private final List<ScriptListener> mScriptListeners = new ArrayList<>();
    private int mLastScriptState = ScriptEngine.ScriptState.SCRIPT_STOPPED;

    public static ScriptManager getInstance() {
        if (mInstance == null) {
            throw new RuntimeException("init first");
        }
        return mInstance;
    }

    /**
     * 该类在使用前必需先初始化
     *
     * @param context app上下文
     */
    public static void init(Context context) {
        mInstance = new ScriptManager();
        mInstance.mScriptDao = CoreDatabase.getInstance(context).getScriptDao();
        mInstance.mSettingData = context.getSharedPreferences("script_list_data", Context.MODE_PRIVATE);
        mInstance.refreshScriptList();
    }

    /**
     * 重新从文件中加载脚本列表
     */
    private void refreshScriptList() {
        mScripts.clear();
        mScripts.addAll(mScriptDao.getAll());
        onScriptListChanged();
    }

    public void addScript(Script script) {
        mScriptDao.insert(script);
        refreshScriptList();

        // 如果列表中只有一个脚本，默认选中它
        if (mScripts.size() == 1) {
            switchScript(mScripts.get(0).getId());
        }
    }

    public void updateScript(Script script) {
        mScriptDao.update(script);
        refreshScriptList();
    }

    /**
     * 查找指定ID对应的脚本对象
     *
     * @param id 脚本ID
     * @return 脚本对象
     */
    @Nullable
    public Script findScriptById(long id) {
        for (Script script : mScripts) {
            if (script.getId() == id) {
                return script;
            }
        }
        return null;
    }

    @Nullable
    public Script findScriptByGuid(String guid) {
        for (Script script : mScripts) {
            if (script.getGuid() != null && script.getGuid().equals(guid)) {
                return script;
            }
        }
        return null;
    }

    /**
     * 返回所有可用的脚本列表
     *
     * @return 脚本列表
     */
    public List<Script> getAllScripts() {
        return mScripts;
    }

    /**
     * 返回当前选中的脚本对象
     *
     * @return 脚本对象
     */
    public Script getCurrentScript() {
        long id = mSettingData.getLong("current_script_id", 0);
        for (Script script : mScripts) {
            if (script.getId() == id) {
                return script;
            }
        }
        return null;
    }

    /**
     * 返回当前选中的脚本ID
     *
     * @return 脚本ID
     */
    public long getCurrentScriptId() {
        Script script = getCurrentScript();
        return script == null ? 0 : script.getId();
    }

    /**
     * 返回当前选中的脚本名称
     *
     * @return 脚本名称
     */
    @Nullable
    public String getCurrentScriptName() {
        Script script = getCurrentScript();
        return script == null ? null : script.getName();
    }

    /**
     * 切换到指定脚本
     *
     * @param id 目标脚本ID
     * @return 返回是否切换成功
     */
    public boolean switchScript(long id) {
        if (id == getCurrentScriptId()) {
            return true;
        }

        // 运行中不允许切换
        if (mLastScriptState != ScriptEngine.ScriptState.SCRIPT_STOPPED) {
            return false;
        }

        for (Script script : mScripts) {
            if (script.getId() == id) {
                mSettingData.edit().putLong("current_script_id", id).apply();
                onScriptSwitched();
                return true;
            }
        }
        return false;
    }

    /**
     * 删除指定脚本
     *
     * @param id 要删除的脚本ID
     */
    public boolean deleteScript(long id) {
        if (mScripts.size() == 0) {
            return false;
        }

        // 运行中不允许删除
        if (mLastScriptState != ScriptEngine.ScriptState.SCRIPT_STOPPED) {
            return false;
        }

        Script script = findScriptById(id);
        if (script == null) {
            return false;
        }

        long currentId = getCurrentScriptId();

        // 删除脚本文件夹
        File scriptDir = new File(script.getScriptDir());
        FileUtil.deleteFile(scriptDir);

        // 删除数据库记录
        mScriptDao.delete(script);
        refreshScriptList();

        if (id == currentId) {
            mSettingData.edit().putLong("current_script_id", 0).apply();
            onScriptSwitched();
        }
        return true;
    }

    /**
     * 获取运行中的脚本ID
     *
     * @return 脚本ID
     */
    public long getRunningScriptId() {
        if (mLastScriptState == ScriptEngine.ScriptState.SCRIPT_STOPPED) {
            return 0;
        }
        return getCurrentScriptId();
    }

    public int getScriptState() {
        return mLastScriptState;
    }

    public String getScriptStateString(Context context) {
        int stateResId = R.string.script_state_unknown;
        switch (getScriptState()) {
            case ScriptEngine.ScriptState.SCRIPT_STARTING:
                stateResId = R.string.script_state_starting;
                break;
            case ScriptEngine.ScriptState.SCRIPT_RUNNING:
                stateResId = R.string.script_state_running;
                break;
            case ScriptEngine.ScriptState.SCRIPT_STOPPING:
                stateResId = R.string.script_state_stopping;
                break;
            case ScriptEngine.ScriptState.SCRIPT_STOPPED:
                stateResId = R.string.script_state_stopped;
                break;
            case ScriptEngine.ScriptState.SCRIPT_ALERTING:
                stateResId = R.string.script_state_alerting;
                break;
            case ScriptEngine.ScriptState.SCRIPT_BLOCKING:
                stateResId = R.string.script_state_blocking;
                break;
            case ScriptEngine.ScriptState.SCRIPT_SUSPENDING:
                stateResId = R.string.script_state_suspending;
                break;
            case ScriptEngine.ScriptState.SCRIPT_RESETING:
                stateResId = R.string.script_state_reseting;
                break;
        }
        return context.getString(stateResId);
    }

    public void registerScriptListener(ScriptListener listener) {
        mScriptListeners.add(listener);
    }

    public void unregisterScriptListener(ScriptListener listener) {
        mScriptListeners.remove(listener);
    }

    private void onScriptListChanged() {
        // 调用已注册的脚本状态监听器
        for (ScriptListener listener : mScriptListeners) {
            listener.onScriptListUpdated();
        }
    }

    private void onScriptSwitched() {
        // 调用已注册的脚本状态监听器
        for (ScriptListener listener : mScriptListeners) {
            listener.onScriptSwitched();
        }
    }

    void onScriptStateChanged(int state, boolean exceptFlag) {
        mLastScriptState = state;

        // 调用已注册的脚本状态监听器
        for (ScriptListener listener : mScriptListeners) {
            listener.onScriptStateChanged(state, exceptFlag);
        }
    }

    public interface ScriptListener {

        void onScriptListUpdated();

        void onScriptSwitched();

        void onScriptStateChanged(int state, boolean exceptFlag);
    }
}
