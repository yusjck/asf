package com.rainman.asf.fragment;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

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
import com.rainman.asf.core.screenshot.ScreenCaptureService;
import com.rainman.asf.util.ToastUtil;
import com.rainman.asf.view.DrawerMenuItem;

public class DrawerFragment extends Fragment {

    private static final int REQUEST_ACCESSIBILITY = 100;
    private static final int REQUEST_SCREEN_CAPTURE = 101;
    private static final int REQUEST_DRAW_OVERLAY = 102;

    private MainActivity mMainActivity;

    private DrawerMenuItem mRequestAccessibility;
    private DrawerMenuItem mRequestScreenCapture;
    private DrawerMenuItem mRequestDrawOverlay;
    private DrawerMenuItem mFloatingWindowSwitch;
    private DrawerMenuItem mRemoteControlSwitch;
    private final ViewClickListener mViewClickListener = new ViewClickListener();

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

        if (savedInstanceState == null) {
            if (AccessibilityHelperService.getInstance() != null) {
                mRequestAccessibility.setChecked(true);
                mRequestAccessibility.setClickable(false);
            }

            if (ScreenCaptureService.getInstance() != null) {
                mRequestScreenCapture.setChecked(true);
                mRequestScreenCapture.setClickable(false);
            }

            if (Build.VERSION.SDK_INT < 23 || Settings.canDrawOverlays(mMainActivity)) {
                mRequestDrawOverlay.setChecked(true);
                mRequestDrawOverlay.setClickable(false);
            }

            App.getInstance().switchFloatingWindow();
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        mFloatingWindowSwitch.setChecked(AppSetting.isFloatingWndEnabled());
        mRemoteControlSwitch.setChecked(AppSetting.isCmdServerEnabled());
    }

    private void initView(View view) {
        mRequestAccessibility = view.findViewById(R.id.request_accessibility);
        mRequestScreenCapture = view.findViewById(R.id.request_screen_capture);
        mRequestDrawOverlay = view.findViewById(R.id.request_draw_overlay);
        mFloatingWindowSwitch = view.findViewById(R.id.floating_window_switch);
        mRemoteControlSwitch = view.findViewById(R.id.remote_control_switch);

        mRequestAccessibility.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    requestAccessibilityPermission();
                }
            }
        });

        mRequestScreenCapture.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    requestScreenCapturePermission();
                }
            }
        });

        mRequestDrawOverlay.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    requestDrawOverlayPermission();
                }
            }
        });

        mFloatingWindowSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                AppSetting.setFloatingWndEnabled(isChecked);
                App.getInstance().switchFloatingWindow();
            }
        });

        mRemoteControlSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                AppSetting.setCmdServerEnabled(isChecked);
                ScriptActuator.getInstance().switchCmdServer();
            }
        });

        view.findViewById(R.id.switch_to_script_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.switch_to_log_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.switch_to_scheduler_view).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.app_setting).setOnClickListener(mViewClickListener);
        view.findViewById(R.id.app_exit).setOnClickListener(mViewClickListener);
    }

    private void requestAccessibilityPermission() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mMainActivity);
        builder.setTitle(R.string.dlg_propmt)
                .setMessage(R.string.request_accessibility_prompt)
                .setPositiveButton(R.string.dlg_go_to_open, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Intent intent = new Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS);
                        startActivityForResult(intent, REQUEST_ACCESSIBILITY);
                        dialog.dismiss();
                    }
                })
                .setCancelable(false);
        builder.create().show();
    }

    private void requestScreenCapturePermission() {
        if (Build.VERSION.SDK_INT >= 21) {
            MediaProjectionManager mediaProjectionManager = (MediaProjectionManager) mMainActivity.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
            startActivityForResult(mediaProjectionManager.createScreenCaptureIntent(), REQUEST_SCREEN_CAPTURE);
        } else {
            mRequestScreenCapture.setChecked(false);
            ToastUtil.show(mMainActivity, R.string.system_function_not_supported);
        }
    }

    private void requestDrawOverlayPermission() {
        if (Build.VERSION.SDK_INT >= 23) {
            if (!Settings.canDrawOverlays(mMainActivity)) {
                Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + mMainActivity.getPackageName()));
                startActivityForResult(intent, REQUEST_DRAW_OVERLAY);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_ACCESSIBILITY:
                try {
                    AccessibilityHelper accessibilityHelper = new AccessibilityHelper(mMainActivity);
                    accessibilityHelper.ensureAccessibilityServiceEnabled();
                    mRequestAccessibility.setClickable(false);
                } catch (Exception e) {
                    ToastUtil.show(mMainActivity, e.getMessage());
                    mRequestAccessibility.setChecked(false);
                }
                break;
            case REQUEST_SCREEN_CAPTURE:
                if (Build.VERSION.SDK_INT >= 21) {
                    if (resultCode != Activity.RESULT_CANCELED) {
                        mRequestScreenCapture.setClickable(false);
                        Intent service = new Intent(mMainActivity, ScreenCaptureService.class);
                        service.putExtra("code", resultCode);
                        service.putExtra("data", data);
                        mMainActivity.startService(service);
                    } else {
                        mRequestScreenCapture.setChecked(false);
                        ToastUtil.show(mMainActivity, R.string.screen_capture_permission_denied);
                    }
                }
                break;
            case REQUEST_DRAW_OVERLAY:
                if (Build.VERSION.SDK_INT >= 23) {
                    if (Settings.canDrawOverlays(mMainActivity)) {
                        mRequestDrawOverlay.setClickable(false);
                        App.getInstance().switchFloatingWindow();
                    } else {
                        mRequestDrawOverlay.setChecked(false);
                        ToastUtil.show(mMainActivity, R.string.draw_overlay_permission_denied);
                    }
                }
                break;
            default:
                super.onActivityResult(requestCode, resultCode, data);
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
