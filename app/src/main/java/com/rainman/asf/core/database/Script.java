package com.rainman.asf.core.database;

import androidx.room.ColumnInfo;
import androidx.room.Entity;
import androidx.room.PrimaryKey;

import java.io.File;
import java.util.Locale;

@Entity
public class Script {

    @PrimaryKey(autoGenerate = true)
    private long id;
    @ColumnInfo(name = "icon")
    private String iconFile;
    private String name;
    private String description;
    @ColumnInfo(name = "main")
    private String mainFile;
    private String scriptDir;
    private String guid;
    private int versionCode;
    private String updateUrl;
    private long lastRunTime;
    private String lastQuitReason;
    @ColumnInfo(name = "optionView")
    private String optionViewFile;

    public Script(String iconFile, String name, String description, String mainFile, String scriptDir, String guid, int versionCode, String updateUrl, String optionViewFile) {
        this.iconFile = iconFile;
        this.name = name;
        this.description = description;
        this.mainFile = mainFile;
        this.scriptDir = scriptDir;
        this.guid = guid;
        this.versionCode = versionCode;
        this.updateUrl = updateUrl;
        this.optionViewFile = optionViewFile;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

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

    public String getScriptDir() {
        return scriptDir;
    }

    public void setScriptDir(String scriptDir) {
        this.scriptDir = scriptDir;
    }

    public String getGuid() {
        return guid;
    }

    public void setGuid(String guid) {
        this.guid = guid;
    }

    public int getVersionCode() {
        return versionCode;
    }

    public void setVersionCode(int versionCode) {
        this.versionCode = versionCode;
    }

    public String getUpdateUrl() {
        return updateUrl;
    }

    public void setUpdateUrl(String updateUrl) {
        this.updateUrl = updateUrl;
    }

    public String getVersionName() {
        return getVersionName(versionCode);
    }

    public static String getVersionName(int versionCode) {
        int majorVer = (versionCode >> 16) & 0xff;
        int minorVer = (versionCode >> 8) & 0xff;
        int patchVer = versionCode & 0xff;
        return String.format(Locale.getDefault(), "%d.%d.%d", majorVer, minorVer, patchVer);
    }

    public long getLastRunTime() {
        return lastRunTime;
    }

    public void setLastRunTime(long lastRunTime) {
        this.lastRunTime = lastRunTime;
    }

    public String getLastQuitReason() {
        return lastQuitReason;
    }

    public void setLastQuitReason(String lastQuitReason) {
        this.lastQuitReason = lastQuitReason;
    }

    public String getOptionViewFile() {
        return optionViewFile;
    }

    public void setOptionViewFile(String optionViewFile) {
        this.optionViewFile = optionViewFile;
    }

    public String getIconPath() {
        if (iconFile == null)
            return null;
        return scriptDir + File.separator + iconFile;
    }

    public String getMainPath() {
        return scriptDir + File.separator + mainFile;
    }

    public String getOptionViewPath() {
        if (optionViewFile == null)
            return null;
        return scriptDir + File.separator + optionViewFile;
    }

    public boolean hasOptionView() {
        return optionViewFile != null && !optionViewFile.equals("");
    }
}
