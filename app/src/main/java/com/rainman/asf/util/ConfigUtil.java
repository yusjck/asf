package com.rainman.asf.util;

import java.util.HashMap;
import java.util.Map;

public class ConfigUtil {

    private Map<String, String> mConfigs = new HashMap<>();

    public String getString(String name) {
        return getString(name, "");
    }

    public String getString(String name, String defaultValue) {
        if (mConfigs.containsKey(name))
            return mConfigs.get(name);
        return defaultValue;
    }

    public void setString(String name, String value) {
        mConfigs.put(name, value);
    }

    public String toConfigString() {
        StringBuilder varString = new StringBuilder();
        for (Map.Entry<String, String> entry : mConfigs.entrySet()) {
            String name = entry.getKey();
            String value = entry.getValue();
            value = value.replace("#", "#1");
            value = value.replace("|", "#2");
            value = value.replace("=", "#3");
            varString.append(String.format("%s=%s|", name, value));
        }
        return varString.toString();
    }
}
