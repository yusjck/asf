package com.rainman.asf.userconfig;

public class ViewData {

    private int type;
    private String groupName;
    private ConfigItem ext;

    public int getType() {
        return type;
    }

    public void setType(int type) {
        this.type = type;
    }

    public String getGroupName() {
        return groupName;
    }

    public void setGroupName(String groupName) {
        this.groupName = groupName;
    }

    public ConfigItem getExt() {
        return ext;
    }

    public void setExt(ConfigItem ext) {
        this.ext = ext;
    }
}
