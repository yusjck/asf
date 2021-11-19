package com.rainman.asf.activity;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;
import android.widget.TimePicker;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;

import com.rainman.asf.R;
import com.rainman.asf.core.SchedulerService;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Scheduler;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.util.ToastUtil;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SchedulerActivity extends AppCompatActivity {

    private TimePicker mTimePicker;
    private TextView tvRepeat;
    private TextView tvScript;
    private SwitchCompat swPrivateConfig;
    private TextView tvShowPrivateConfig;
    private int mRepeat;
    private ServiceConnection mServiceConnection;
    private SchedulerService mSchedulerService;
    private Scheduler mScheduler;
    private List<Script> mScriptList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_scheduler);

        initView();
        initData();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindService(mServiceConnection);
    }

    private void initView() {
        mTimePicker = findViewById(R.id.time_picker);
        mTimePicker.setDescendantFocusability(TimePicker.FOCUS_BLOCK_DESCENDANTS);  // 设置点击事件不弹键盘
        mTimePicker.setIs24HourView(true);   // 设置时间显示为24小时

        tvRepeat = findViewById(R.id.tv_repeat);
        tvScript = findViewById(R.id.tv_script);
        swPrivateConfig = findViewById(R.id.sw_private_config);
        swPrivateConfig.setOnCheckedChangeListener((buttonView, isChecked) -> {
            mScheduler.setConfigEnabled(isChecked);
            tvShowPrivateConfig.setTextColor(isChecked ? 0xFF000000 : 0xFFCCCCCC);
        });
        tvShowPrivateConfig = findViewById(R.id.tv_show_private_config);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    private void initData() {
        final long schedulerId = getIntent().getLongExtra("scheduler_id", 0);
        if (schedulerId == 0) {
            setTitle(R.string.add_scheduler);
        } else {
            setTitle(R.string.edit_scheduler);
        }

        long scriptId = getIntent().getLongExtra("script_id", 0);
        if (scriptId != 0) {
            mScriptList = new ArrayList<>();
            mScriptList.add(ScriptManager.getInstance().findScriptById(scriptId));
        } else {
            mScriptList = ScriptManager.getInstance().getAllScripts();
        }

        mServiceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                SchedulerService.ServiceBinder serviceBinder = (SchedulerService.ServiceBinder) service;
                mSchedulerService = serviceBinder.getService();
                loadScheduler(schedulerId);
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {

            }
        };
        Intent intent = new Intent(this, SchedulerService.class);
        bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
    }

    private void loadScheduler(long schedulerId) {
        mScheduler = mSchedulerService.findSchedulerById(schedulerId);
        // 没有记录则做为新增记录
        if (mScheduler == null) {
            // 当只有一个脚本可选择时默认就选中它
            mScheduler = new Scheduler();
            if (mScriptList.size() == 1) {
                mScheduler.setScriptId(mScriptList.get(0).getId());
            }
        }

        mTimePicker.setCurrentHour(mScheduler.getHour());
        mTimePicker.setCurrentMinute(mScheduler.getMinute());
        swPrivateConfig.setChecked(mScheduler.isConfigEnabled());
        mScheduler.setEnabled(true);

        updateRepeatWidget();
        updateScriptWidget();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.scheduler, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            finish();
            return true;
        } else if (itemId == R.id.action_save) {
            if (saveScheduler()) {
                finish();
            }
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private boolean saveScheduler() {
        if (mScheduler.getScriptId() == 0) {
            ToastUtil.show(this, R.string.no_script_selected);
            return false;
        }

        mScheduler.setHour(mTimePicker.getCurrentHour());
        mScheduler.setMinute(mTimePicker.getCurrentMinute());
        mSchedulerService.addOrUpdateScheduler(mScheduler);

        setResult(Activity.RESULT_OK);
        return true;
    }

    private void updateRepeatWidget() {
        tvRepeat.setText(mScheduler.getRepeatDesc(this));
    }

    private void updateScriptWidget() {
        for (Script script : mScriptList) {
            if (script.getId() == mScheduler.getScriptId()) {
                tvScript.setText(script.getName());
                return;
            }
        }
        tvScript.setText(R.string.select_a_script);
    }

    public void onRepeatClick(View view) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setItems(R.array.repeat_frequency, (dialog, which) -> {
            switch (which) {
                case 0:
                    mScheduler.setRepeat(0);
                    updateRepeatWidget();
                    break;
                case 1:
                    mScheduler.setRepeat(127);
                    updateRepeatWidget();
                    break;
                case 2:
                    onSelectDaysOfWeek();
                    break;
            }
            dialog.dismiss();
        });
        builder.show();
    }

    private void onSelectDaysOfWeek() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        boolean[] checkedItems = new boolean[7];
        mRepeat = mScheduler.getRepeat();
        for (int i = 0; i < checkedItems.length; i++) {
            checkedItems[i] = (mRepeat & (1 << i)) != 0;
        }

        builder.setMultiChoiceItems(R.array.week, checkedItems, (dialog, which, isChecked) -> {
            if (isChecked) {
                mRepeat |= (1 << which);
            } else {
                mRepeat &= ~(1 << which);
            }
        });

        builder.setNegativeButton(R.string.dlg_cancel, (dialog, which) -> dialog.dismiss());
        builder.setPositiveButton(R.string.dlg_confirm, (dialog, which) -> {
            mScheduler.setRepeat(mRepeat);
            updateRepeatWidget();
            dialog.dismiss();
        });
        builder.show();
    }

    public void onScriptClick(View view) {
        String[] items = new String[mScriptList.size()];
        for (int i = 0; i < mScriptList.size(); i++) {
            items[i] = mScriptList.get(i).getName();
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setItems(items, (dialog, which) -> {
            Script script = mScriptList.get(which);
            mScheduler.setScriptId(script.getId());
            updateScriptWidget();
            dialog.dismiss();
        });
        builder.show();
        swPrivateConfig.setChecked(false);
    }

    public void onPrivateConfigClick(View view) {
        if (swPrivateConfig.isChecked()) {
            swPrivateConfig.setChecked(false);
        } else {
            if (mScheduler.getConfigName() == null) {
                onFirstEnablePrivateConfig();
            } else {
                swPrivateConfig.setChecked(true);
            }
        }
    }

    public void onShowPrivateConfigClick(View view) {
        if (swPrivateConfig.isChecked()) {
            showPrivateConfig();
        }
    }

    private void onFirstEnablePrivateConfig() {
        if (mScheduler.getId() == 0) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle(R.string.save_scheduler_confirmation)
                    .setMessage(R.string.save_scheduler_confirmation_prompt)
                    .setPositiveButton(R.string.dlg_confirm, (dialog, which) -> {
                        if (saveScheduler()) {
                            showPrivateConfig();
                        }
                        dialog.dismiss();
                    })
                    .setNegativeButton(R.string.dlg_cancel, (dialog, which) -> dialog.dismiss());
            builder.create().show();
        } else {
            showPrivateConfig();
        }
    }

    private void showPrivateConfig() {
        if (mScheduler.getConfigName() == null) {
            mScheduler.setConfigName(String.format(Locale.getDefault(), "scheduler-%d", mScheduler.getId()));
        }
        Intent intent = new Intent(SchedulerActivity.this, OptionActivity.class);
        intent.putExtra("script_id", mScheduler.getScriptId());
        intent.putExtra("config_name", mScheduler.getConfigName());
        startActivity(intent);
        swPrivateConfig.setChecked(true);
    }
}
