package com.rainman.asf;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.rainman.asf.core.DeviceEventReceiver;
import com.rainman.asf.core.ScriptControlService;
import com.rainman.asf.core.SchedulerService;
import com.rainman.asf.core.ScriptEngine;
import com.rainman.asf.core.ScriptLogger;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.VisitorManager;

import org.xutils.x;

public class App extends Application {

    private static final String TAG = "App";
    private static App mInstance;

    public static App getInstance() {
        return mInstance;
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onCreate() {
        super.onCreate();

        Log.i(TAG, "onCreate");
        mInstance = this;
        x.Ext.init(this);
        AppSetting.init(this);
        VisitorManager.init(this);

        ScriptEngine.init(this);
        ScriptManager.init(this);
        ScriptLogger.init(this);

        // 启动脚本引擎
        ScriptEngine.getInstance().startup();

        // 启动任务计划服务
        startService(new Intent(this, SchedulerService.class));

        // 启动通知栏脚本控制服务
        ContextCompat.startForegroundService(this, new Intent(this, ScriptControlService.class));

        // 初始化摇一摇
        initShake();

        // 注册脚本控制热键
        GlobalHotKey.registerGlobalHotKey();

        // 注册设备事件监听
        DeviceEventReceiver.registerReceiver(this);
    }

    @Override
    public void onTerminate() {
        super.onTerminate();

        Log.i(TAG, "onTerminate");
        GlobalHotKey.unregisterGlobalHotKey();
        DeviceEventReceiver.unregisterReceiver(this);
    }

    private void initShake() {
        ShakeListener shakeListener = new ShakeListener(this);
        shakeListener.setOnShakeListener(new ShakeListener.OnShakeListener() {
            @Override
            public void onShake() {
                if (AppSetting.isStopWhenWaggling()) {
                    ScriptEngine.getInstance().stopScript();
                }
            }
        });
    }

    static class ShakeListener implements SensorEventListener {

        private static final int SPEED_SHRESHOLD = 3000;
        private static final int UPTATE_INTERVAL_TIME = 100;
        private long mLastUpdateTime;
        private float mLastX;
        private float mLastY;
        private float mLastZ;
        private Context mContext;
        private OnShakeListener mOnShakeListener;
        private SensorManager mSensorManager;

        ShakeListener(Context context) {
            this.mContext = context;
            start();
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {

        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            long curTime = System.currentTimeMillis();
            long elapsedTime = curTime - mLastUpdateTime;
            if (elapsedTime >= UPTATE_INTERVAL_TIME) {
                mLastUpdateTime = curTime;
                float f1 = event.values[0];
                float f2 = event.values[1];
                float f3 = event.values[2];
                float f4 = f1 - mLastX;
                float f5 = f2 - mLastY;
                float f6 = f3 - mLastZ;
                mLastX = f1;
                mLastY = f2;
                mLastZ = f3;
                if (Math.sqrt((f4 * f4 + f5 * f5 + f6 * f6)) / elapsedTime * 10000 >= SPEED_SHRESHOLD) {
                    mOnShakeListener.onShake();
                }
            }
        }

        void setOnShakeListener(OnShakeListener onShakeListener) {
            mOnShakeListener = onShakeListener;
        }

        public void start() {
            mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
            Sensor sensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            mSensorManager.registerListener(this, sensor, SensorManager.SENSOR_DELAY_GAME);
        }

        public void stop() {
            mSensorManager.unregisterListener(this);
        }

        public interface OnShakeListener {
            void onShake();
        }
    }
}
