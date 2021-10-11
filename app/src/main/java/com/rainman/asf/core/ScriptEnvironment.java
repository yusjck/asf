package com.rainman.asf.core;

import android.content.Context;

import java.io.File;

public class ScriptEnvironment {

    public static String getScriptRootDir() {
        Context context = ScriptActuator.getInstance().getContext();
        String scriptPath = context.getFilesDir().getPath() + "/scripts";
        File scriptDir = new File(scriptPath);
        if (!scriptDir.exists()) {
            scriptDir.mkdir();
        }
        return scriptPath;
    }

    public static String getTempDir() {
        Context context = ScriptActuator.getInstance().getContext();
        String tmpPath = context.getCacheDir().getPath() + "/tmp";
        File tmpDir = new File(tmpPath);
        if (!tmpDir.exists()) {
            tmpDir.mkdir();
        }
        return tmpPath;
    }

    public static String getPluginDir() {
        Context context = ScriptActuator.getInstance().getContext();
        String tmpPath = context.getFilesDir().getPath() + "/plugins";
        File tmpDir = new File(tmpPath);
        if (!tmpDir.exists()) {
            tmpDir.mkdir();
        }
        return tmpPath;
    }

    public static String getLogDir() {
        Context context = ScriptActuator.getInstance().getContext();
        String logPath = context.getCacheDir().getPath() + "/logs";
        File logDir = new File(logPath);
        if (!logDir.exists()) {
            logDir.mkdir();
        }
        return logPath;
    }

    public static String getStartupDir() {
        Context context = ScriptActuator.getInstance().getContext();
        return context.getCacheDir().getAbsolutePath();
    }
}
