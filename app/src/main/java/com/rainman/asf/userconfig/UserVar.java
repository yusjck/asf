package com.rainman.asf.userconfig;

import com.rainman.asf.util.FileUtil;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

public class UserVar {

    private Map<String, String> mVarDefine;
    private Map<String, String> mVarValue;
    private File mConfigFile;

    public UserVar(ConfigManager configManager, String configName) {
        mVarDefine = new LinkedHashMap<>();
        mVarValue = new LinkedHashMap<>();

        if (configName == null) {
            mConfigFile = new File(configManager.getConfigDir(), "uservar/default.json");
        } else {
            mConfigFile = new File(configManager.getConfigDir(), "uservar/" + configName + ".json");
        }

        ArrayList<ConfigGroup> groups = configManager.getGroups();
        for (ConfigGroup group : groups) {
            for (ConfigItem item : group.getItems()) {
                mVarDefine.put(item.getName(), item.getDefaultValue());
            }
        }

        load();
    }

    public UserVar(File configDir) {
        this(new ConfigManager(configDir), null);
    }

    public UserVar(File configDir, String configName) {
        this(new ConfigManager(configDir), configName);
    }

    public void save() {
        JSONObject jsonObject = new JSONObject();
        for (Map.Entry<String, String> entry : mVarValue.entrySet()) {
            try {
                jsonObject.put(entry.getKey(), entry.getValue());
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        if (!mConfigFile.getParentFile().exists()) {
            mConfigFile.getParentFile().mkdirs();
        }
        FileUtil.writeTextFile(mConfigFile, jsonObject.toString());
    }

    private void load() {
        if (mConfigFile.exists()) {
            String json = FileUtil.readTextFile(mConfigFile);
            try {
                JSONObject jsonObject = new JSONObject(json);
                if (jsonObject.length() > 0) {
                    JSONArray jsonArray = jsonObject.names();
                    for (int i = 0; i < jsonArray.length(); i++) {
                        String name = jsonArray.getString(i);
                        mVarValue.put(name, jsonObject.getString(name));
                    }
                }
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
    }

    public void put(String key, String value) {
        if (mVarDefine.containsKey(key)) {
            mVarValue.put(key, value);
        }
    }

    public String get(String key) {
        if (mVarValue.containsKey(key)) {
            return mVarValue.get(key);
        } else {
            return mVarDefine.get(key);
        }
    }

    public Map<String, String> getUserVars() {
        Map<String, String> userVars = new HashMap<>();
        for (String key : mVarDefine.keySet()) {
            String value = get(key);
            userVars.put(key, value);
        }
        return userVars;
    }

    public String getUserVarString() {
        StringBuilder varString = new StringBuilder();
        for (String key : mVarDefine.keySet()) {
            String value = get(key);
            value = value.replace("#", "#1");
            value = value.replace("|", "#2");
            value = value.replace("=", "#3");
            varString.append(String.format("%s=%s|", key, value));
        }
        return varString.toString();
    }
}
