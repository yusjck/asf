package com.rainman.asf.activity;

import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.rainman.asf.App;
import com.rainman.asf.R;
import com.rainman.asf.accessibility.AccessibilityHelper;

public class SplashActivity extends AppCompatActivity {

    private static final int ENTRY_HOME = 100;
    private static final int REQUEST_ACCESSIBILITY = 101;
    private RelativeLayout rlSplashRoot;
    private TextView tvVersionName;

    private final Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);

            switch (msg.what) {
                case ENTRY_HOME:
                    entryHome();
                    break;

                case REQUEST_ACCESSIBILITY:
                    break;
            }
        }
    };

    private void entryHome() {
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        startActivity(intent);

        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);

        // 防止重复启动
        if (!isTaskRoot()) {
            finish();
            return;
        }

        if (App.getInstance().isAppInited()) {
            entryHome();
            return;
        }

        App.getInstance().initApp();
        initView();
        initData();
        initAnimation();
        initPermission();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        mHandler.removeCallbacksAndMessages(null);
    }

    private void initView() {
        // 全屏显示
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // 隐藏标题栏
        if (getSupportActionBar() != null) {
            getSupportActionBar().hide();
        }

        rlSplashRoot = findViewById(R.id.rl_splash_root);
        tvVersionName = findViewById(R.id.tv_version_name);
    }

    private void initData() {
        PackageManager packageManager = getPackageManager();
        try {
            PackageInfo packageInfo = packageManager.getPackageInfo(getPackageName(), 0);
            // 显示当前版本
            tvVersionName.setText(packageInfo.versionName);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
    }

    private void initAnimation() {
        AlphaAnimation alphaAnimation = new AlphaAnimation(0, 1);
        alphaAnimation.setDuration(1000);
        rlSplashRoot.setAnimation(alphaAnimation);
    }

    private void initPermission() {
        new Thread() {
            @Override
            public void run() {
                super.run();
                long startTime = System.currentTimeMillis();

                Message message = Message.obtain();
                message.what = ENTRY_HOME;

                AccessibilityHelper accessibilityHelper = new AccessibilityHelper(SplashActivity.this);
                if (!accessibilityHelper.isAccessibilityServiceEnabled()) {
                    // 如果未开启无障碍服务，尝试通过ROOT命令来开启
                    accessibilityHelper.enableAccessibilityServiceByRoot();
                }

                long elapsedTime = System.currentTimeMillis() - startTime;
                if (elapsedTime < 2000) {
                    try {
                        Thread.sleep(2000 - elapsedTime);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

                mHandler.sendMessage(message);
            }
        }.start();
    }
}
