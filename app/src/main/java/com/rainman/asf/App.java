package com.rainman.asf;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.rainman.asf.core.ForegroundService;
import com.rainman.asf.core.SchedulerService;
import com.rainman.asf.core.ScriptActuator;
import com.rainman.asf.core.ScriptLogger;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.VisitorManager;
import com.rainman.asf.core.screenshot.ScreenCapture;
import com.rainman.asf.util.Compat;

import org.xutils.x;

public class App extends Application {

    private static final String TAG = "App";
    private static App mInstance;
    private boolean mAppInited = false;
    private ShakeListener mShakeListener;
    private FloatingWindow mFloatingWindow;

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

        ScriptActuator.init(this);
        ScriptManager.init(this);
        ScriptLogger.init(this);
        VisitorManager.init(this);

        // 初始化摇一摇
        initShake();

        // 注册脚本控制热键
        GlobalHotKey.registerGlobalHotKey();

        // 注册设备事件接收器
        DeviceEvent.registerReceiver(this);

        mFloatingWindow = new FloatingWindow();
    }

    @Override
    public void onTerminate() {
        super.onTerminate();

        Log.i(TAG, "onTerminate");
        mShakeListener.stop();
        GlobalHotKey.unregisterGlobalHotKey();
        DeviceEvent.unregisterReceiver(this);
    }

    public void switchFloatingWindow() {
        if (!Compat.canDrawOverlays(this)) {
            AppSetting.setFloatingWndEnabled(false);
        } else {
            if (AppSetting.isFloatingWndEnabled()) {
                mFloatingWindow.showWindow(this);
            } else {
                mFloatingWindow.dismissWindow();
            }
        }
    }

    public boolean isAppInited() {
        return mAppInited;
    }

    public void initApp() {
        if (mAppInited)
            return;

        // 启动脚本引擎
        ScriptActuator.getInstance().startup();

        // 启动任务计划服务
        startService(new Intent(this, SchedulerService.class));

        // 启动通知栏脚本控制服务
        ContextCompat.startForegroundService(this, new Intent(this, ForegroundService.class));

        mAppInited = true;
    }

    public void exitApp() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            ScreenCapture.freeMediaProjection();
        }

        mFloatingWindow.dismissWindow();
        ScriptActuator.getInstance().cleanup();
        stopService(new Intent(this, SchedulerService.class));
        stopService(new Intent(this, ForegroundService.class));

        mAppInited = false;
    }

    private void initShake() {
        mShakeListener = new ShakeListener(this);
        mShakeListener.setOnShakeListener(() -> {
            if (AppSetting.isStopWhenWaggling()) {
                ScriptActuator.getInstance().stopScript();
            }
        });
        mShakeListener.start();
    }

    private static class ShakeListener implements SensorEventListener {

        private static final int SPEED_SHRESHOLD = 3000;
        private static final int UPTATE_INTERVAL_TIME = 100;
        private long mLastUpdateTime;
        private float mLastX;
        private float mLastY;
        private float mLastZ;
        private final SensorManager mSensorManager;
        private OnShakeListener mOnShakeListener;

        ShakeListener(Context context) {
            mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
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
