package com.rainman.asf.core.database;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

@Entity
public class ScriptLog {

    @PrimaryKey(autoGenerate = true)
    private long id;
    private int time;
    private int color;
    private String text;

    public ScriptLog(int time, int color, String text) {
        this.time = time;
        this.color = color;
        this.text = text;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public int getTime() {
        return time;
    }

    public void setTime(int time) {
        this.time = time;
    }

    public int getColor() {
        return color;
    }

    public void setColor(int color) {
        this.color = color;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }
}
