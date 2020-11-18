package com.rainman.asf.userconfig;

import java.util.ArrayList;

public class ConfigGroup {

    private String text;
    private ArrayList<ConfigItem> items = new ArrayList<>();

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public ArrayList<ConfigItem> getItems() {
        return items;
    }

    public void setItems(ArrayList<ConfigItem> items) {
        this.items = items;
    }
}
