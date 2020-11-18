package com.rainman.asf.userconfig;

import java.util.LinkedHashMap;
import java.util.Map;

public class ListItem implements ConfigItem {

    private String name;
    private String text;
    private String description;
    private String defaultValue;
    private Map<String, String> options = new LinkedHashMap<>();

    @Override
    public String getItemType() {
        return "List";
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getDefaultValue() {
        return defaultValue;
    }

    public void setDefaultValue(String defaultValue) {
        this.defaultValue = defaultValue;
    }

    public Map<String, String> getOptions() {
        return options;
    }

    public void setOptions(Map<String, String> options) {
        this.options = options;
    }

    String toOptionText(String optValue) {
        return options.get(optValue);
    }

    int toOptionIndex(String optValue) {
        int index = 0;
        for (String s : options.keySet()) {
            if (s.equals(optValue))
                return index;
            index++;
        }
        return -1;
    }

    String toOptionValue(String optText) {
        for (Map.Entry<String, String> entry : options.entrySet()) {
            if (optText.equals(entry.getValue()))
                return entry.getKey();
        }
        return "";
    }
}
