package com.rainman.asf.fragment;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.appcompat.widget.Toolbar;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.rainman.asf.R;
import com.rainman.asf.core.database.ScriptLog;
import com.rainman.asf.core.ScriptLogger;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

public class LogFragment extends BaseFragment {

    private static final String TAG = "LogFragment";
    private RecyclerView rvLogView;
    private LogAdapter mLogAdapter;
    private ScriptLogger mScriptLogger;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_log, container, false);
    }

    @Override
    protected void initView(View view) {
        rvLogView = view.findViewById(R.id.rv_script_log);
        rvLogView.setLayoutManager(new LinearLayoutManager(getContext()));
    }

    @Override
    protected void initData(Bundle savedInstanceState) {
        mScriptLogger = ScriptLogger.getInstance();

        int initPos = -1;
        Bundle bundle = getArguments();
        if (bundle != null) {
            initPos = bundle.getInt("init_pos", initPos);
        }

        mLogAdapter = new LogAdapter();
        rvLogView.setAdapter(mLogAdapter);
        mLogAdapter.initLogData(initPos);

        // 注册日志监听器用于实时打印日志输出
        mScriptLogger.registerLogListener(mLogAdapter);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mScriptLogger.unregisterLogListener(mLogAdapter);
    }

    @Override
    protected void onCreateToolbar(Toolbar toolbar) {
        toolbar.inflateMenu(R.menu.log_list);
        toolbar.setTitle(R.string.script_log);
    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        switch (menuItem.getItemId()) {
            case R.id.action_clear_logs:
                onClearLogs();
                return true;
            case R.id.action_show_scripts:
                mMainActivity.displayFragment(new ScriptFragment());
                return true;
        }
        return false;
    }

    private void onClearLogs() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mMainActivity);
        builder.setTitle(R.string.clear_logs_confirmation)
                .setMessage(R.string.clear_logs_confirmation_prompt)
                .setPositiveButton(R.string.dlg_confirm, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        mLogAdapter.clearAllLogs();
                        mScriptLogger.clearAllLogs();
                        dialog.dismiss();
                    }
                })
                .setNegativeButton(R.string.dlg_cancel, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
        builder.create().show();
    }

    private class LogAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> implements ScriptLogger.LogListener {

        private static final int PAGE_LOG_COUNT = 50;
        private static final int MAX_CACHE_COUNT = PAGE_LOG_COUNT * 2;
        private int mLogTotal;
        private int mCacheStartPos;
        private int mCacheEndPos;
        private List<ScriptLog> mLogCache = new ArrayList<>();

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.view_log_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            // 判断当前要显示的记录是否在缓存中，不存则需要重新加载缓存
            if (position < mCacheStartPos || position >= mCacheEndPos) {
                loadLogCache(position);
            }
            ((ViewHolder) holder).bindData(mLogCache.get(position - mCacheStartPos));
        }

        @Override
        public int getItemCount() {
            return mLogTotal;
        }

        @Override
        public void onScriptLogOutput(ScriptLog log) {
            addLog(log);
        }

        void addLog(ScriptLog log) {
            // 如果当前显示的是最后一页日志，直接往缓存中添加新的记录
            if (mCacheEndPos == mLogTotal) {
                // 缓存记录超过一个页后每添加一条都从最前面删除一条
                if (mLogCache.size() >= MAX_CACHE_COUNT) {
                    mLogCache.remove(0);
                    mCacheStartPos++;   // 最前面一条被删除了，移动缓存起始位置
                }
                mLogCache.add(log);     // 添加一条记录
                mLogTotal++;            // 添加记录总数
                mCacheEndPos++;         // 添加缓存结束位置
            } else {
                // 当前显示的不是最后一页日志，重新加载最后一页到缓存中
                mLogTotal = mScriptLogger.getLogCount();
                loadLogCache(mLogTotal - 1);
            }
            rvLogView.scrollToPosition(mLogTotal - 1);  // 跳转到最后一条日志显示
            notifyDataSetChanged();
        }

        private void loadLogCache(int pos) {
            mCacheStartPos = Math.max(0, pos - PAGE_LOG_COUNT);
            mCacheEndPos = mCacheStartPos + Math.min(mLogTotal - mCacheStartPos, MAX_CACHE_COUNT);
            Log.i(TAG, String.format("load log cache, pos=%d, count=%d", mCacheStartPos, mCacheEndPos - mCacheStartPos));
            mLogCache = mScriptLogger.queryLogInfo(mCacheStartPos, mCacheEndPos - mCacheStartPos);
        }

        void initLogData(int initPos) {
            mLogTotal = mScriptLogger.getLogCount();
            if (initPos < 0 || initPos >= mLogTotal) {
                initPos = mLogTotal - 1;
            }
            loadLogCache(initPos);
            rvLogView.scrollToPosition(initPos);
            notifyDataSetChanged();

            final int finalInitPos = initPos;
            rvLogView.post(new Runnable() {
                @Override
                public void run() {
                    LinearLayoutManager manager = (LinearLayoutManager) rvLogView.getLayoutManager();
                    if (manager != null) {
                        int firstPosition = manager.findFirstVisibleItemPosition();
                        int lastPosition = manager.findLastVisibleItemPosition();
                        // 滚动列表让初始条目处于列表倒数第五的位置
                        rvLogView.scrollToPosition(finalInitPos - (lastPosition - firstPosition) + 5);
                    }
                }
            });
        }

        void clearAllLogs() {
            mLogCache.clear();
            mLogTotal = 0;
            mCacheStartPos = 0;
            mCacheEndPos = 0;
            notifyDataSetChanged();
        }

        private class ViewHolder extends RecyclerView.ViewHolder {

            private final TextView tvLogView;

            ViewHolder(View itemView) {
                super(itemView);
                tvLogView = itemView.findViewById(R.id.tv_log_view);
            }

            void bindData(ScriptLog log) {
                Calendar calendar = Calendar.getInstance();
                calendar.setTime(new Date());
                calendar.set(Calendar.HOUR_OF_DAY, 0);
                calendar.set(Calendar.MINUTE, 0);
                calendar.set(Calendar.SECOND, 0);
                Date date = new Date((long) log.getTime() * 1000);
                SimpleDateFormat sdf = (SimpleDateFormat) DateFormat.getDateTimeInstance();

                // 当天的日志只显示时间，当天以前的显示日期加时间
                if (date.before(calendar.getTime())) {
                    sdf.applyPattern("MM/dd HH:mm:ss");
                } else {
                    sdf.applyPattern("HH:mm:ss");
                }

                tvLogView.setTextColor(log.getColor() | 0xff000000);
                tvLogView.setText(String.format("%s  %s", sdf.format(date), log.getText()));
            }
        }
    }
}
