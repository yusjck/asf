package com.rainman.asf.util;

import android.content.Context;

import com.rainman.asf.R;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

public class DateUtil {

    public static String getLastTimeDescription(Context context, long lastTime) {
        // 取当前时间，只保留年月日
        Calendar calendar = Calendar.getInstance();
        Calendar calendar1 = Calendar.getInstance();
        calendar1.clear();
        calendar1.set(calendar.get(Calendar.YEAR), calendar.get(Calendar.MONTH), calendar.get(Calendar.DATE));

        // 取传入参数时间，只保留年月日
        calendar.setTime(new Date(lastTime));
        Calendar calendar2 = Calendar.getInstance();
        calendar2.clear();
        calendar2.set(calendar.get(Calendar.YEAR), calendar.get(Calendar.MONTH), calendar.get(Calendar.DATE));

        // 计算两个时间之间相隔天数
        int dayDiff = (int) ((calendar1.getTimeInMillis() - calendar2.getTimeInMillis()) / (24 * 60 * 60 * 1000));

        SimpleDateFormat sdf = (SimpleDateFormat) DateFormat.getDateTimeInstance();
        sdf.applyPattern("HH:mm:ss");
        switch (dayDiff) {
            case 0:
                return context.getString(R.string.today) + " " + sdf.format(new Date(lastTime));
            case 1:
                return context.getString(R.string.yesterday) + " " + sdf.format(new Date(lastTime));
            case 2:
                return context.getString(R.string.two_days_ago) + " " + sdf.format(new Date(lastTime));
            default:
                return String.format(context.getString(R.string.n_days_ago), dayDiff) + " " + sdf.format(new Date(lastTime));
        }
    }
}
