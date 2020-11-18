package com.rainman.asf.core.database;

import androidx.room.*;

import android.content.Context;

import com.rainman.asf.R;

import java.util.Calendar;
import java.util.Date;

@Entity(foreignKeys = @ForeignKey(entity = Script.class, parentColumns = "id", childColumns = "scriptId", onDelete = ForeignKey.CASCADE), indices = {@Index("scriptId")})
public class Scheduler {

    @PrimaryKey(autoGenerate = true)
    private long id;
    private int hour;
    private int minute;
    private int repeat;
    private long scriptId;
    private String configName;
    private boolean configEnabled;
    private boolean enabled;
    @Ignore
    private Date runTime;

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public int getHour() {
        return hour;
    }

    public void setHour(int hour) {
        this.hour = hour;
    }

    public int getMinute() {
        return minute;
    }

    public void setMinute(int minute) {
        this.minute = minute;
    }

    public int getRepeat() {
        return repeat;
    }

    public void setRepeat(int repeat) {
        this.repeat = repeat;
    }

    public long getScriptId() {
        return scriptId;
    }

    public void setScriptId(long scriptId) {
        this.scriptId = scriptId;
    }

    public String getConfigName() {
        return configName;
    }

    public void setConfigName(String configName) {
        this.configName = configName;
    }

    public boolean isConfigEnabled() {
        return configEnabled;
    }

    public void setConfigEnabled(boolean configEnabled) {
        this.configEnabled = configEnabled;
    }

    public boolean isEnabled() {
        return enabled;
    }

    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    public String getRepeatDesc(Context context) {
        String desc;
        switch (repeat) {
            case 0:
                desc = context.getString(R.string.only_once);
                break;
            case 127:
                desc = context.getString(R.string.every_day);
                break;
            default:
                String[] items = context.getResources().getStringArray(R.array.week);
                StringBuilder text = new StringBuilder();
                for (int i = 0; i < 7; i++) {
                    if ((repeat & (1 << i)) != 0) {
                        if (text.length() > 0) {
                            text.append(",");
                        }
                        text.append(items[i]);
                    }
                }
                desc = text.toString();
                break;
        }
        return desc;
    }

    public Date getNextRunTime() {
        if (!enabled)
            return null;

        Calendar calendar = Calendar.getInstance();
        calendar.setTime(new Date());
        calendar.get(Calendar.DAY_OF_WEEK);

        calendar.set(Calendar.SECOND, 0);
        calendar.set(Calendar.MILLISECOND, 0);

        if (repeat == 0 || repeat == 127) {
            if (calendar.get(Calendar.HOUR_OF_DAY) > hour ||
                    (calendar.get(Calendar.HOUR_OF_DAY) == hour && calendar.get(Calendar.MINUTE) >= minute)) {
                // 今天的触发时间已过，改到明天触发
                calendar.add(Calendar.DAY_OF_YEAR, 1);
            }
            calendar.set(Calendar.HOUR_OF_DAY, hour);
            calendar.set(Calendar.MINUTE, minute);
            return calendar.getTime();
        }

        for (int i = 0; i < 7; i++) {
            if ((repeat & (1 << (calendar.get(Calendar.DAY_OF_WEEK)) - 1)) == 0) {
                calendar.add(Calendar.DAY_OF_WEEK, 1);
                continue;
            }

            if (calendar.get(Calendar.HOUR_OF_DAY) > hour ||
                    (calendar.get(Calendar.HOUR_OF_DAY) == hour && calendar.get(Calendar.MINUTE) >= minute)) {
                if (i == 0) {
                    calendar.add(Calendar.DAY_OF_WEEK, 1);
                    continue;
                }
            }

            calendar.set(Calendar.HOUR_OF_DAY, hour);
            calendar.set(Calendar.MINUTE, minute);
            return calendar.getTime();
        }

        return null;
    }

    public Date getRunTime() {
        return runTime;
    }

    public void setRunTime(Date runTime) {
        this.runTime = runTime;
    }

    public static class SchedulerInfo extends Scheduler {

        private String scriptName;

        public String getScriptName() {
            return scriptName;
        }

        public void setScriptName(String scriptName) {
            this.scriptName = scriptName;
        }
    }
}
