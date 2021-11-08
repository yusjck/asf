package com.rainman.asf.activity;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.Html;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.fragment.app.Fragment;
import androidx.core.content.ContextCompat;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.rainman.asf.App;
import com.rainman.asf.fragment.DrawerFragment;
import com.rainman.asf.R;
import com.rainman.asf.fragment.LogFragment;
import com.rainman.asf.fragment.SchedulerFragment;
import com.rainman.asf.fragment.ScriptFragment;
import com.rainman.asf.core.ScriptActuator;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.util.DisplayUtil;
import com.rainman.asf.util.ToastUtil;

public class MainActivity extends AppCompatActivity implements ScriptManager.ScriptListener, ScriptActuator.EngineStateListener {

    private static final String TAG = "MainActivity";
    private static final int PERMISSION_REQUEST_CODE = 1000;
    private TextView tvScriptState;
    private TextView tvEngineState;
    private FloatingActionButton btScriptControl;
    private DrawerLayout mDrawerLayout;
    private ScriptManager mScriptManager;
    private ScriptActuator mScriptActuator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initView();

        mScriptActuator = ScriptActuator.getInstance();
        // 设置引擎状态监听器
        mScriptActuator.setEngineStateListener(this);

        mScriptManager = ScriptManager.getInstance();
        // 注册脚本状态监听器
        mScriptManager.registerScriptListener(this);

        checkPermission();
        loadFrameLayout();

        if (mScriptManager.getScriptState() != ScriptActuator.ScriptState.SCRIPT_STOPPED) {
            btScriptControl.setImageResource(R.drawable.ic_stop);
        } else {
            btScriptControl.setImageResource(R.drawable.ic_start);
        }
        // 更新状态条上的脚本信息
        updateStatusBar(false);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mScriptActuator.setEngineStateListener(null);
        mScriptManager.unregisterScriptListener(this);
        Log.i(TAG, "destroy MainActivity");
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        loadFrameLayout();
    }

    @SuppressLint("ClickableViewAccessibility")
    private void initView() {
        getSupportFragmentManager().beginTransaction().replace(R.id.fragment_drawer, new DrawerFragment()).commit();

        tvScriptState = findViewById(R.id.tv_script_state);
        tvEngineState = findViewById(R.id.tv_engine_state);
        btScriptControl = findViewById(R.id.bt_script_control);
        mDrawerLayout = findViewById(R.id.drawer_layout);

        btScriptControl.setOnTouchListener(new View.OnTouchListener() {
            private int downX, downY;
            private int lastX, lastY;
            private Rect movableRegion;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        lastX = downX = (int) event.getRawX();
                        lastY = downY = (int) event.getRawY();
                        if (movableRegion == null) {
                            movableRegion = new Rect();
                            movableRegion.left = v.getLeft() - DisplayUtil.dip2px(70);
                            movableRegion.top = v.getTop() - DisplayUtil.dip2px(70);
                            movableRegion.right = v.getRight();
                            movableRegion.bottom = v.getBottom();
                        }
                        break;
                    case MotionEvent.ACTION_MOVE:
                        int dx = (int) event.getRawX() - lastX;
                        int dy = (int) event.getRawY() - lastY;
                        lastX = (int) event.getRawX();
                        lastY = (int) event.getRawY();
                        int left = v.getLeft() + dx;
                        int top = v.getTop() + dy;
                        int right = v.getRight() + dx;
                        int bottom = v.getBottom() + dy;
                        if (left < movableRegion.left) {
                            left = movableRegion.left;
                            right = left + v.getWidth();
                        }
                        if (right > movableRegion.right) {
                            right = movableRegion.right;
                            left = right - v.getWidth();
                        }
                        if (top < movableRegion.top) {
                            top = movableRegion.top;
                            bottom = top + v.getHeight();
                        }
                        if (bottom > movableRegion.bottom) {
                            bottom = movableRegion.bottom;
                            top = bottom - v.getHeight();
                        }
                        v.layout(left, top, right, bottom);
                        break;
                    case MotionEvent.ACTION_UP:
                        if (Math.abs((int) (event.getRawX() - downX)) <= 5 && Math.abs((int) (event.getRawY() - downY)) <= 5) {
                            onScriptControl();
                        }
                        break;
                }
                return false;
            }
        });
    }

    private void loadFrameLayout() {
        String action = getIntent().getAction();
        if ("showLogs".equals(action)) {
            // 点击通知栏消息进入时打开日志列表
            int initPos = getIntent().getIntExtra("init_pos", -1);
            Bundle bundle = new Bundle();
            bundle.putInt("init_pos", initPos);
            LogFragment fragment = new LogFragment();
            fragment.setArguments(bundle);
            displayFragment(fragment);
        } else if ("showSchedulers".equals(action)) {
            // 点击通知栏脚本大师进入脚本运行计划列表
            displayFragment(new SchedulerFragment());
        } else {
            // 默认打开脚本列表
            displayFragment(new ScriptFragment());
        }
    }

    public void setToolbar(Toolbar toolbar) {
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(this, mDrawerLayout, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        mDrawerLayout.addDrawerListener(toggle);
        toggle.syncState();
    }

    public void displayFragment(Fragment fragment) {
        getSupportFragmentManager().beginTransaction().replace(R.id.fragment_holder, fragment).commitAllowingStateLoss();
        mDrawerLayout.closeDrawers();
    }

    private void checkPermission() {
        String[] permissions = {
                Manifest.permission.READ_PHONE_STATE,
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.ACCESS_FINE_LOCATION,
        };
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            permissions = new String[]{
                    Manifest.permission.READ_PHONE_STATE,
                    Manifest.permission.ACCESS_COARSE_LOCATION,
                    Manifest.permission.ACCESS_FINE_LOCATION,
                    Manifest.permission.ACCESS_BACKGROUND_LOCATION,
            };
        }
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST_CODE);
                return;
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean isAllGranted = true;

            // 判断是否所有的权限都已经授予了
            for (int grant : grantResults) {
                if (grant != PackageManager.PERMISSION_GRANTED) {
                    isAllGranted = false;
                    break;
                }
            }

            if (!isAllGranted) {
                // 弹出对话框告诉用户需要权限的原因，并引导用户去应用权限管理中手动打开权限按钮
                openAppDetails();
            }
        }
    }

    private void openAppDetails() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(R.string.request_permission_prompt);
        builder.setPositiveButton(R.string.dlg_manual_authorization, (dialog, which) -> {
            Intent intent = new Intent();
            intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
            intent.addCategory(Intent.CATEGORY_DEFAULT);
            intent.setData(Uri.parse("package:" + getPackageName()));
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
            intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            startActivity(intent);
        });
        builder.setNegativeButton(R.string.dlg_cancel, null);
        builder.show();
    }

    private void updateStatusBar(boolean exceptFlag) {
        int stateResId = R.string.script_state_unknown;
        switch (mScriptManager.getScriptState()) {
            case ScriptActuator.ScriptState.SCRIPT_STARTING:
                stateResId = R.string.script_state_starting;
                break;
            case ScriptActuator.ScriptState.SCRIPT_RUNNING:
                stateResId = R.string.script_state_running;
                break;
            case ScriptActuator.ScriptState.SCRIPT_STOPPING:
                stateResId = R.string.script_state_stopping;
                break;
            case ScriptActuator.ScriptState.SCRIPT_STOPPED:
                stateResId = R.string.script_state_stopped;
                if (exceptFlag) {
                    stateResId = R.string.script_state_exception;
                }
                break;
            case ScriptActuator.ScriptState.SCRIPT_ALERTING:
                stateResId = R.string.script_state_alerting;
                break;
            case ScriptActuator.ScriptState.SCRIPT_BLOCKING:
                stateResId = R.string.script_state_blocking;
                break;
            case ScriptActuator.ScriptState.SCRIPT_SUSPENDING:
                stateResId = R.string.script_state_suspending;
                break;
            case ScriptActuator.ScriptState.SCRIPT_RESETING:
                stateResId = R.string.script_state_reseting;
                break;
        }

        String stateText;
        if (!exceptFlag) {
            stateText = String.format(" <font color='#1CBD1C'>[%s]</font>", getResources().getString(stateResId));
        } else {
            stateText = String.format(" <font color='#FF0000'>[%s]</font>", getResources().getString(stateResId));
        }

        String scriptName = mScriptManager.getCurrentScriptName();
        if (scriptName != null) {
            tvScriptState.setText(Html.fromHtml(scriptName + stateText));
        } else {
            if (mScriptManager.getAllScripts().size() == 0) {
                tvScriptState.setText(R.string.no_scripts_available);
            } else {
                tvScriptState.setText(R.string.script_state_ready);
            }
        }
    }

    @Override
    public void onScriptListUpdated() {
        updateStatusBar(false);
    }

    @Override
    public void onScriptSwitched() {
        updateStatusBar(false);
    }

    @Override
    public void onScriptStateChanged(int state, boolean exceptFlag) {
        Log.i(TAG, "scriptState=" + state + ",exceptFlag=" + exceptFlag);

        if (state == ScriptActuator.ScriptState.SCRIPT_STOPPED) {
            btScriptControl.setImageResource(R.drawable.ic_start);
        } else {
            btScriptControl.setImageResource(R.drawable.ic_stop);
        }

        updateStatusBar(exceptFlag);
    }

    public void onScriptControl() {
        if (mScriptManager.getScriptState() == ScriptActuator.ScriptState.SCRIPT_STOPPED) {
            if (mScriptManager.getCurrentScript() == null) {
                ToastUtil.show(this, R.string.run_script_prompt);
                return;
            }
            mScriptActuator.startScript();
        } else {
            mScriptActuator.stopScript();
        }
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = findViewById(R.id.drawer_layout);
        if (drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            // 如果当前打开的不是ScriptFragment就先返回ScriptFragment
            Fragment current = getSupportFragmentManager().findFragmentById(R.id.fragment_holder);
            if (!(current instanceof ScriptFragment)) {
                displayFragment(new ScriptFragment());
                return;
            }
            finish();
        }
    }

    public void setting() {
        Intent intent = new Intent(this, SettingActivity.class);
        startActivity(intent);
        mDrawerLayout.closeDrawers();
    }

    public void exit() {
        mScriptActuator.stopScript();
        finish();
        App.getInstance().exitApp();
    }

    @Override
    public void onEngineStateChanged(int engineState) {
        switch (engineState) {
            case ScriptActuator.ENGINE_CONNECTED:
                tvScriptState.setVisibility(View.VISIBLE);
                tvEngineState.setVisibility(View.GONE);
                tvEngineState.setText(R.string.engine_connected);
                break;
            case ScriptActuator.ENGINE_DISCONNECTED:
                tvScriptState.setVisibility(View.GONE);
                tvEngineState.setVisibility(View.VISIBLE);
                tvEngineState.setText(R.string.engine_disconnected);
                break;
            case ScriptActuator.ENGINE_CONNECTION_FAIL:
                tvScriptState.setVisibility(View.GONE);
                tvEngineState.setVisibility(View.VISIBLE);
                tvEngineState.setText(R.string.engine_connection_fail);
                showEngineError();
                break;
            case ScriptActuator.ENGINE_CONNECTION_LOSE:
                tvScriptState.setVisibility(View.GONE);
                tvEngineState.setVisibility(View.VISIBLE);
                tvEngineState.setText(R.string.engine_connection_lose);
                break;
        }
    }

    private void showEngineError() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.app_name)
                .setMessage(R.string.connect_engine_failed_prompt)
                .setPositiveButton(R.string.dlg_retry, (dialog, which) -> {
                    mScriptActuator.reconnectNativeEngine();
                    dialog.dismiss();
                })
                .setNegativeButton(R.string.dlg_cancel, (dialog, which) -> dialog.dismiss());
        builder.create().show();
    }
}
