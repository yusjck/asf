package com.rainman.asf.fragment;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AlertDialog;

import com.rainman.asf.App;
import com.rainman.asf.AppSetting;
import com.rainman.asf.R;
import com.rainman.asf.accessibility.AccessibilityHelper;
import com.rainman.asf.accessibility.AccessibilityHelperService;
import com.rainman.asf.activity.MainActivity;
import com.rainman.asf.core.ScriptActuator;
import com.rainman.asf.core.screenshot.ScreenCapture;
import com.rainman.asf.util.ToastUtil;
import com.rainman.asf.view.DrawerMenuItem;

public class DrawerFragment extends Fragment {

    private MainActivity mMainActivity;

    private DrawerMenuItem mRequestAccessibility;
    private DrawerMenuItem mRequestScreenCapture;
    private DrawerMenuItem mRequestDrawOverlay;
    private DrawerMenuItem mFloatingWindowSwitch;
    private DrawerMenuItem mRemoteControlSwitch;

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
        mMainActivity = (MainActivity) context;
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_drawer, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        initView(view);
        App.getInstance().switchFloatingWindow();
    }

    @Override
    public void onResume() {
        super.onResume();

        if (AccessibilityHelperService.getInstance() != null) {
            mRequestAccessibility.setChecked(true);
            mRequestAccessibility.setClickable(false);
        } else {
            mRequestAccessibility.setChecked(false);
            mRequestAccessibility.setClickable(true);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mRequestScreenCapture.setChecked(ScreenCapture.isMediaProjectionAvailable());
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (Settings.canDrawOverlays(mMainActivity)) {
                mRequestDrawOverlay.setChecked(true);
                mRequestDrawOverlay.setClickable(false);
            } else {
                mRequestDrawOverlay.setChecked(false);
                mRequestDrawOverlay.setClickable(true);
            }
        }

        mFloatingWindowSwitch.setChecked(AppSetting.isFloatingWndEnabled());
        mRemoteControlSwitch.setChecked(AppSetting.isCmdServerEnabled());
    }

    private void initView(View view) {
        mRequestAccessibility = view.findViewById(R.id.request_accessibility);
        mRequestScreenCapture = view.findViewById(R.id.request_screen_capture);
        mRequestDrawOverlay = view.findViewById(R.id.request_draw_overlay);
        mFloatingWindowSwitch = view.findViewById(R.id.floating_window_switch);
        mRemoteControlSwitch = view.findViewById(R.id.remote_control_switch);

        mRequestAccessibility.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                requestAccessibilityPermission();
            }
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mRequestScreenCapture.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (isChecked) {
                    requestScreenCapturePermission();
                } else {
                    cancelScreenCapturePermission();
                }
            });
        } else {
            mRequestScreenCapture.setEnabled(false);
            mRequestScreenCapture.setChecked(false);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mRequestDrawOverlay.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (isChecked) {
                    requestDrawOverlayPermission();
                }
            });
        } else {
            mRequestDrawOverlay.setEnabled(false);
            mRequestDrawOverlay.setChecked(true);
        }

        mFloatingWindowSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            AppSetting.setFloatingWndEnabled(isChecked);
            if (!isChecked) {
                // 关闭悬浮窗
                App.getInstance().switchFloatingWindow();
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(mMainActivity)) {
                    // 用户打开悬浮窗但没有权限时先申请权限
                    requestDrawOverlayPermission();
                } else {
                    // 打开悬浮窗
                    App.getInstance().switchFloatingWindow();
                }
            }
        });

        mRemoteControlSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            AppSetting.setCmdServerEnabled(isChecked);
            ScriptActuator.getInstance().switchCmdServer();
        });

        view.findViewById(R.id.switch_to_script_view).setOnClickListener(v -> mMainActivity.displayFragment(new ScriptFragment()));
        view.findViewById(R.id.switch_to_log_view).setOnClickListener(v -> mMainActivity.displayFragment(new LogFragment()));
        view.findViewById(R.id.switch_to_scheduler_view).setOnClickListener(v -> mMainActivity.displayFragment(new SchedulerFragment()));
        view.findViewById(R.id.app_setting).setOnClickListener(v -> mMainActivity.setting());
        view.findViewById(R.id.app_exit).setOnClickListener(v -> mMainActivity.exit());
    }

    private final ActivityResultLauncher<Intent> mRequestAccessibilityLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                try {
                    AccessibilityHelper accessibilityHelper = new AccessibilityHelper(mMainActivity);
                    accessibilityHelper.ensureAccessibilityServiceEnabled();
                    mRequestAccessibility.setClickable(false);
                } catch (Exception e) {
                    ToastUtil.show(mMainActivity, e.getMessage());
                    mRequestAccessibility.setChecked(false);
                }
            });

    private void requestAccessibilityPermission() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mMainActivity);
        builder.setTitle(R.string.dlg_propmt)
                .setMessage(R.string.request_accessibility_prompt)
                .setPositiveButton(R.string.dlg_go_to_open, (dialog, which) -> {
                    mRequestAccessibilityLauncher.launch(new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS));
                    dialog.dismiss();
                })
                .setCancelable(false);
        builder.create().show();
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private final ActivityResultLauncher<Intent> mRequestScreenCaptureLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() != Activity.RESULT_CANCELED) {
                    ScreenCapture.initMediaProjection(mMainActivity, result.getResultCode(), result.getData());
                } else {
                    mRequestScreenCapture.setChecked(false);
                    ToastUtil.show(mMainActivity, R.string.screen_capture_permission_denied);
                }
            });

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private void requestScreenCapturePermission() {
        MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) mMainActivity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        mRequestScreenCaptureLauncher.launch(mediaProjectionManager.createScreenCaptureIntent());
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private void cancelScreenCapturePermission() {
        ScreenCapture.freeMediaProjection();
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    private final ActivityResultLauncher<Intent> mRequestDrawOverlayLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (Settings.canDrawOverlays(mMainActivity)) {
                    mRequestDrawOverlay.setClickable(false);
                    App.getInstance().switchFloatingWindow();
                } else {
                    mRequestDrawOverlay.setChecked(false);
                    AppSetting.setFloatingWndEnabled(false);
                    mFloatingWindowSwitch.setChecked(false);
                    ToastUtil.show(mMainActivity, R.string.draw_overlay_permission_denied);
                }
            });

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void requestDrawOverlayPermission() {
        if (!Settings.canDrawOverlays(mMainActivity)) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + mMainActivity.getPackageName()));
            mRequestDrawOverlayLauncher.launch(intent);
        }
    }
}
