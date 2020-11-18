package com.rainman.asf.core.database;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

import java.util.Date;

@Entity
public class Visitor {

    public static final int PERMISSION_GRANTED = 1;
    public static final int PERMISSION_PROMPT = 0;
    public static final int PERMISSION_DENIED = -1;
    @PrimaryKey(autoGenerate = true)
    private long id;
    private String name;
    private String signature;
    private int accessPermission;
    private long expireTime;
    private long lastAccessTime;

    public Visitor(String name, String signature) {
        this.name = name;
        this.signature = signature;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getSignature() {
        return signature;
    }

    public void setSignature(String signature) {
        this.signature = signature;
    }

    public int getAccessPermission() {
        return accessPermission;
    }

    public void setAccessPermission(int accessPermission) {
        this.accessPermission = accessPermission;
    }

    public long getExpireTime() {
        return expireTime;
    }

    public void setExpireTime(long expireTime) {
        this.expireTime = expireTime;
    }

    public long getLastAccessTime() {
        return lastAccessTime;
    }

    public void setLastAccessTime(long lastAccessTime) {
        this.lastAccessTime = lastAccessTime;
    }

    public boolean isPermissionExpired() {
        long now = new Date().getTime();
        return expireTime != 0 && now > expireTime;
    }
}
