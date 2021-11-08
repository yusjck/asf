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
import com.rainman.asf.util.SystemUtils;
import com.rainman.asf.util.ToastUtil;
import com.rainman.asf.view.DrawerMenuItem;

public class DrawerFragment extends Fragment {

    private MainActivity mMainActivity;

    private DrawerMenuItem mRequestAccessibility;
    private DrawerMenuItem mRequestScreenCapture;
    private DrawerMenuItem mRequestDrawOverlay;
    private DrawerMenuItem mFloatingWindowSwitch;
    private DrawerMenuItem mRemoteControlSwitch;
    private final ViewClickListener mViewClickListener = new ViewClickListener();

    private ActivityResultLauncher<Intent> mRequestAccessibilityLauncher;
    private ActivityResultLauncher<Intent> mRequestScreenCaptureLauncher;
    private ActivityResultLauncher<Intent> mRequestDrawOverlayLauncher;

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

        mRequestScreenCapture.setChecked(ScreenCapture.isMediaProjectionAvailable());

        if (SystemUtils.canDrawOverlays(mMainActivity)) {
            mRequestDrawOverlay.setChecked(true);
            mRequestDrawOverlay.setClickable(false);
        } else {
            mRequestDrawOverlay.setChecked(false);
            mRequestDrawOverlay.setClickable(true);
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

        mRequestScreenCapture.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                requestScreenCapturePermission();
            } else {
                cancelScreenCapturePermission();
            }
        });

        mRequestDrawOverlay.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                requestDrawOverlayPermission();
            }
        });

        mFloatingWindowSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            AppSetting.setFloatingWndEnabled(isChecked);
            if (!isChecked || SystemUtils.canDrawOverlays(mMainActivity)) {
                App.getInstance().switchFloatingWindow();
            } else {
                // 用户打开悬浮窗但没有权限时先申请权限
                requestDrawOverlayPermission();
            }
        });

        mRemoteControlSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            AppSetting.setCmdServerEnabled(isChecked);
            ScriptActuator.getInstance().switchCmdServer();
        });

        view.findViewById(R.id.switch_to_script_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.switch_to_log_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.switch_to_scheduler_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.app_setting).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.app_exit).setOnClickListener(mViewClickListener);

        mRequestAccessibilityLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
            try {
                AccessibilityHelper accessibilityHelper = new AccessibilityHelper(mMainActivity);
                accessibilityHelper.ensureAccessibilityServiceEnabled();
                mRequestAccessibility.setClickable(false);
            } catch (Exception e) {
                ToastUtil.show(mMainActivity, e.getMessage());
                mRequestAccessibility.setChecked(false);
            }
        });

        mRequestScreenCaptureLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
            if (result.getResultCode() != Activity.RESULT_CANCELED) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    ScreenCapture.initMediaProjection(mMainActivity, result.getResultCode(), result.getData());
                }
            } else {
                mRequestScreenCapture.setChecked(false);
                ToastUtil.show(mMainActivity, R.string.screen_capture_permission_denied);
            }
        });

        mRequestDrawOverlayLauncher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
            if (SystemUtils.canDrawOverlays(mMainActivity)) {
                mRequestDrawOverlay.setClickable(false);
                App.getInstance().switchFloatingWindow();
            } else {
                mRequestDrawOverlay.setChecked(false);
                AppSetting.setFloatingWndEnabled(false);
                mFloatingWindowSwitch.setChecked(false);
                ToastUtil.show(mMainActivity, R.string.draw_overlay_permission_denied);
            }
        });
    }

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

    private void requestScreenCapturePermission() {
        if (Build.VERSION.SDK_INT >= 21) {
            MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) mMainActivity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
            mRequestScreenCaptureLauncher.launch(mediaProjectionManager.createScreenCaptureIntent());
        } else {
            mRequestScreenCapture.setChecked(false);
            ToastUtil.show(mMainActivity, R.string.system_function_not_supported);
        }
    }

    private void cancelScreenCapturePermission() {
        if (Build.VERSION.SDK_INT >= 21) {
            ScreenCapture.freeMediaProjection();
        }
    }

    private void requestDrawOverlayPermission() {
        if (!SystemUtils.canDrawOverlays(mMainActivity)) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + mMainActivity.getPackageName()));
            mRequestDrawOverlayLauncher.launch(intent);
        }
    }

    private class ViewClickListener implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            int viewId = v.getId();
            if (viewId == R.id.switch_to_script_view) {
                mMainActivity.displayFragment(new ScriptFragment());
            } else if (viewId == R.id.switch_to_log_view) {
                mMainActivity.displayFragment(new LogFragment());
            } else if (viewId == R.id.switch_to_scheduler_view) {
                mMainActivity.displayFragment(new SchedulerFragment());
            } else if (viewId == R.id.app_setting) {
                mMainActivity.setting();
            } else if (viewId == R.id.app_exit) {
                mMainActivity.exit();
            }
        }
    }
}
