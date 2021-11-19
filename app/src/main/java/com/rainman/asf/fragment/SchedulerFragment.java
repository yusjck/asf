package com.rainman.asf.fragment;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SwitchCompat;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.rainman.asf.R;
import com.rainman.asf.activity.SchedulerActivity;
import com.rainman.asf.core.SchedulerService;
import com.rainman.asf.core.database.Scheduler;

import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class SchedulerFragment extends BaseFragment {

    private RecyclerView rvSchedulerList;
    private SchedulerAdapter mSchedulerAdapter;
    private long mScriptId;
    private SchedulerService mSchedulerService;
    private Scheduler.SchedulerInfo mMenuSelectedScheduler;
    private ServiceConnection mServiceConnection;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_scheduler, container, false);
    }

    @Override
    protected void initView(View view) {
        rvSchedulerList = view.findViewById(R.id.rv_scheduler_list);
        rvSchedulerList.setLayoutManager(new LinearLayoutManager(mMainActivity));
        rvSchedulerList.addItemDecoration(new DividerItemDecoration(mMainActivity, DividerItemDecoration.VERTICAL));
    }

    @Override
    protected void initData(Bundle savedInstanceState) {
        Bundle bundle = getArguments();
        if (bundle == null) {
            mScriptId = 0;
        } else {
            mScriptId = bundle.getLong("script_id", 0);
        }

        mSchedulerAdapter = new SchedulerAdapter();
        rvSchedulerList.setAdapter(mSchedulerAdapter);

        mServiceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                SchedulerService.ServiceBinder serviceBinder = (SchedulerService.ServiceBinder) service;
                mSchedulerService = serviceBinder.getService();
                setListAutoUpdate();
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {

            }
        };
        Intent intent = new Intent(mMainActivity, SchedulerService.class);
        mMainActivity.bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mMainActivity.unbindService(mServiceConnection);
    }

    @Override
    protected void onCreateToolbar(Toolbar toolbar) {
        toolbar.inflateMenu(R.menu.scheduler_list);
        toolbar.setTitle(R.string.script_scheduler);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.action_delete) {
            mSchedulerService.deleteScheduler(mMenuSelectedScheduler);
            mSchedulerAdapter.reloadData();
            return true;
        }
        return super.onContextItemSelected(item);
    }

    private final ActivityResultLauncher<Intent> mSchedulerActivityLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    mSchedulerAdapter.reloadData();
                }
            });

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        if (menuItem.getItemId() == R.id.action_add) {
            Intent intent = new Intent(mMainActivity, SchedulerActivity.class);
            intent.putExtra("script_id", mScriptId);
            mSchedulerActivityLauncher.launch(intent);
            return true;
        }
        return false;
    }

    private void setListAutoUpdate() {
        // 设置每分钟更新一次列表
        Calendar calendar = Calendar.getInstance();
        int delayTime = 60 - calendar.get(Calendar.SECOND);
        mSchedulerAdapter.reloadData();
        rvSchedulerList.postDelayed(this::setListAutoUpdate, delayTime * 1000);
    }

    private class SchedulerAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

        private List<Scheduler.SchedulerInfo> mSchedulerList;

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.view_scheduler_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            ((ViewHolder) holder).bindData(mSchedulerList.get(position));
        }

        @Override
        public int getItemCount() {
            return mSchedulerList == null ? 0 : mSchedulerList.size();
        }

        @SuppressLint("NotifyDataSetChanged")
        void reloadData() {
            if (mScriptId != 0) {
                mSchedulerList = mSchedulerService.getSchedulersByScriptId(mScriptId);
            } else {
                mSchedulerList = mSchedulerService.getAllSchedulers();
            }
            notifyDataSetChanged();
        }

        private class ViewHolder extends RecyclerView.ViewHolder implements View.OnCreateContextMenuListener {

            private final TextView tv_time;
            private final TextView tv_repeat;
            private final TextView tv_script;
            private final SwitchCompat sw_enabled;
            private Scheduler.SchedulerInfo mScheduler;

            ViewHolder(View view) {
                super(view);
                tv_time = view.findViewById(R.id.tv_time);
                tv_repeat = view.findViewById(R.id.tv_repeat);
                tv_script = view.findViewById(R.id.tv_script);
                sw_enabled = view.findViewById(R.id.sw_enabled);

                itemView.setOnCreateContextMenuListener(this);
                sw_enabled.setOnCheckedChangeListener((buttonView, isChecked) -> {
                    if (mScheduler.isEnabled() != isChecked) {
                        mScheduler.setEnabled(isChecked);
                        mSchedulerService.addOrUpdateScheduler(mScheduler);
                        updateRepeatDesc();
                    }
                });

                itemView.setOnClickListener(v -> {
                    Intent intent = new Intent(mMainActivity, SchedulerActivity.class);
                    intent.putExtra("script_id", mScriptId);
                    intent.putExtra("scheduler_id", mScheduler.getId());
                    mSchedulerActivityLauncher.launch(intent);
                });
            }

            void updateRepeatDesc() {
                String repeatDesc = mScheduler.getRepeatDesc(mMainActivity);
                if (mScheduler.isEnabled()) {
                    Date nextRunTime = mScheduler.getNextRunTime();
                    long diffTime = (nextRunTime.getTime() - new Date().getTime()) / 1000;
                    diffTime = diffTime / 60 + ((diffTime % 60) == 0 ? 0 : 1);
                    if (diffTime / 60 > 0) {
                        repeatDesc += String.format(getString(R.string.scheduler_repeat_desc1), diffTime / 60, diffTime % 60);
                    } else {
                        repeatDesc += String.format(getString(R.string.scheduler_repeat_desc2), diffTime % 60);
                    }
                } else {
                    repeatDesc += getString(R.string.scheduler_repeat_desc3);
                }
                tv_repeat.setText(repeatDesc);
            }

            void bindData(Scheduler.SchedulerInfo scheduler) {
                mScheduler = scheduler;
                tv_time.setText(String.format(Locale.getDefault(), "%02d:%02d", scheduler.getHour(), scheduler.getMinute()));
                tv_script.setText(scheduler.getScriptName());
                sw_enabled.setChecked(scheduler.isEnabled());
                updateRepeatDesc();
            }

            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                mMenuSelectedScheduler = mScheduler;
                new MenuInflater(mMainActivity).inflate(R.menu.scheduler_item, menu);
            }
        }
    }
}
