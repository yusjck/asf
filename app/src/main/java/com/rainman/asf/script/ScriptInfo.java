package com.rainman.asf.script;

import java.util.ArrayList;
import java.util.List;

public class ScriptInfo {

    private String iconFile;
    private String name;
    private String description;
    private String mainFile;
    private String guid;
    private String version;
    private String optionViewFile;
    private List<String> resFileList = new ArrayList<>();

    public String getIconFile() {
        return iconFile;
    }

    public void setIconFile(String iconFile) {
        this.iconFile = iconFile;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getMainFile() {
        return mainFile;
    }

    public void setMainFile(String mainFile) {
        this.mainFile = mainFile;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public String getOptionViewFile() {
        return optionViewFile;
    }

    public void setOptionViewFile(String optionViewFile) {
        this.optionViewFile = optionViewFile;
    }

    List<String> getResFileList() {
        return resFileList;
    }
}
