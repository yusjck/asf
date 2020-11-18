package com.rainman.asf.util;

import android.content.Context;

import com.rainman.asf.R;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

public class DateUtil {

    public static String getLastTimeDescription(Context context, long time) {
        Calendar calendar1 = Calendar.getInstance();
        calendar1.setTime(new Date());
        Calendar calendar2 = Calendar.getInstance();
        calendar2.setTime(new Date(time));

        SimpleDateFormat sdf = (SimpleDateFormat) DateFormat.getDateTimeInstance();
        sdf.applyPattern("HH:mm:ss");
        int dayDiff = (int) ((calendar1.getTimeInMillis() - calendar2.getTimeInMillis()) / (24 * 60 * 60 * 1000));
        switch (dayDiff) {
            case 0:
                return context.getString(R.string.today) + " " + sdf.format(calendar2.getTime());
            case 1:
                return context.getString(R.string.yesterday) + " " + sdf.format(calendar2.getTime());
            case 2:
                return context.getString(R.string.two_days_ago) + " " + sdf.format(calendar2.getTime());
            default:
                return String.format(context.getString(R.string.n_days_ago), dayDiff) + " " + sdf.format(calendar2.getTime());
        }
    }
}
