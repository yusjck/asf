package com.rainman.asf.core.screenshot;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.IBinder;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

public class ScreenCaptureService extends Service {

    private static ScreenCaptureService mInstance;
    private MediaProjection mMediaProjection;

    @Override
    public void onCreate() {
        super.onCreate();
        mInstance = this;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mInstance = null;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        int code = intent.getIntExtra("code", 0);
        Intent data = intent.getParcelableExtra("data");
        if (data != null) {
            MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) getSystemService(Context.MEDIA_PROJECTION_SERVICE);
            mMediaProjection = mediaProjectionManager.getMediaProjection(code, data);
        }
        if (mMediaProjection == null) {
            stopSelf();
        }
        return Service.START_REDELIVER_INTENT;      // 服务如果被系统重启必须重新传递Intent参数不然无法使用屏幕录像功能
    }

    public static ScreenCaptureService getInstance() {
        return mInstance;
    }

    public MediaProjection getMediaProjection() {
        return mMediaProjection;
    }
}
