package com.rainman.asf.fragment;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.res.ResourcesCompat;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.rainman.asf.R;
import com.rainman.asf.activity.DownloadActivity;
import com.rainman.asf.activity.OptionActivity;
import com.rainman.asf.core.SchedulerService;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.script.ScriptImportHelper;
import com.rainman.asf.util.DateUtil;
import com.rainman.asf.util.ToastUtil;

import java.util.List;

public class ScriptFragment extends BaseFragment {

    private RecyclerView rvScriptList;
    private ScriptAdapter mScriptAdapter;
    private ScriptManager mScriptManager;
    private Script mMenuSelectedScript;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_script, container, false);
    }

    @Override
    protected void initView(View view) {
        rvScriptList = view.findViewById(R.id.rv_script_list);
        rvScriptList.setLayoutManager(new LinearLayoutManager(mMainActivity));
        rvScriptList.addItemDecoration(new DividerItemDecoration(mMainActivity, DividerItemDecoration.VERTICAL));
    }

    @Override
    protected void initData(Bundle savedInstanceState) {
        mScriptManager = ScriptManager.getInstance();

        mScriptAdapter = new ScriptAdapter();
        rvScriptList.setAdapter(mScriptAdapter);
        mScriptAdapter.reloadData();

        mScriptManager.registerScriptListener(mScriptAdapter);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mScriptManager.unregisterScriptListener(mScriptAdapter);
    }

    @Override
    protected void onCreateToolbar(Toolbar toolbar) {
        toolbar.inflateMenu(R.menu.script_list);
        toolbar.setTitle(R.string.script_list);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == R.id.action_update) {
            onUpdateScriptClick(mMenuSelectedScript);
            return true;
        } else if (itemId == R.id.action_delete) {
            onDeleteScriptClick(mMenuSelectedScript);
            return true;
        } else if (itemId == R.id.action_help) {
            return true;
        }
        return super.onContextItemSelected(item);
    }

    private final ActivityResultLauncher<Intent> mUpdateScriptLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    Intent data = result.getData();
                    assert data != null;
                    Uri uri = data.getData();
                    if (uri != null) {
                        ScriptImportHelper scriptImportHelper = new ScriptImportHelper(getContext());
                        if (!scriptImportHelper.updateLocalScript(mMenuSelectedScript.getId(), uri)) {
                            ToastUtil.show(getContext(), R.string.file_import_failed);
                        }
                    }
                }

            });

    private void onUpdateScriptClick(Script script) {
        // 如果是下载的脚本则跳转到脚本下载页面更新
        if (script.getUpdateUrl() != null) {
            startActivity(new Intent(mMainActivity, DownloadActivity.class));
            return;
        }
        // 本地导入的脚本通过导入新文件更新
        Intent intent = new Intent();
        intent.setType("*/*");
        intent.setAction(Intent.ACTION_GET_CONTENT);
        mUpdateScriptLauncher.launch(intent);
        ToastUtil.showLong(getContext(), R.string.import_update_prompt);
    }

    private void onDeleteScriptClick(final Script script) {
        if (mScriptManager.getRunningScriptId() != 0) {
            ToastUtil.show(mMainActivity, R.string.forbid_deletion);
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(mMainActivity);
        builder.setTitle(R.string.delete_confirmation)
                .setMessage(String.format(mMainActivity.getString(R.string.delete_confirmation_prompt), script.getName()))
                .setPositiveButton(R.string.dlg_confirm, (dialog, which) -> {
                    // 删除脚本
                    if (mScriptManager.deleteScript(script.getId())) {
                        // 删除相关联的运行计划
                        Intent service = new Intent(mMainActivity, SchedulerService.class);
                        service.setAction("deleteScriptScheduler");
                        service.putExtra("script_id", script.getId());
                        mMainActivity.startService(service);
                    }
                    dialog.dismiss();
                })
                .setNegativeButton(R.string.dlg_cancel, (dialog, which) -> dialog.dismiss());
        builder.create().show();
    }

    private void onScriptOptionClick(Script script) {
        Intent intent = new Intent(mMainActivity, OptionActivity.class);
        intent.putExtra("script_id", script.getId());
        startActivity(intent);
    }

    private void onScriptSchedulerClick(Script script) {
        SchedulerFragment schedulerFragment = new SchedulerFragment();
        Bundle bundle = new Bundle();
        bundle.putLong("script_id", script.getId());
        schedulerFragment.setArguments(bundle);
        mMainActivity.displayFragment(schedulerFragment);
    }

    private final ActivityResultLauncher<Intent> mImportScriptLauncher =
            registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    Intent data = result.getData();
                    assert data != null;
                    Uri uri = data.getData();
                    if (uri != null) {
                        ScriptImportHelper scriptImportHelper = new ScriptImportHelper(getContext());
                        if (!scriptImportHelper.importLocalScript(uri)) {
                            ToastUtil.show(getContext(), R.string.file_import_failed);
                        }
                    }
                }
            });

    private void onImportScriptClick() {
        Intent intent = new Intent();
        intent.setType("*/*");
        intent.setAction(Intent.ACTION_GET_CONTENT);
        mImportScriptLauncher.launch(intent);
        ToastUtil.showLong(getContext(), R.string.import_file_prompt);
    }

    private void onDownloadScriptClick() {
        startActivity(new Intent(mMainActivity, DownloadActivity.class));
    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        int itemId = menuItem.getItemId();
        if (itemId == R.id.action_import_script) {
            onImportScriptClick();
            return true;
        } else if (itemId == R.id.action_download_script) {
            onDownloadScriptClick();
            return true;
        } else if (itemId == R.id.action_show_logs) {
            mMainActivity.displayFragment(new LogFragment());
            return true;
        }
        return false;
    }

    private class ScriptAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> implements ScriptManager.ScriptListener {

        private List<Script> mScriptList;

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.view_script_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            ((ViewHolder) holder).bindData(mScriptList.get(position));
        }

        @Override
        public int getItemCount() {
            return mScriptList == null ? 0 : mScriptList.size();
        }

        @Override
        public void onScriptListUpdated() {
            reloadData();
        }

        @Override
        public void onScriptSwitched() {
            long selectedId = mScriptManager.getCurrentScriptId();
            // 重新设置选中项
            for (int i = 0; i < mScriptList.size(); i++) {
                ViewHolder holder = (ViewHolder) rvScriptList.findViewHolderForLayoutPosition(i);
                if (holder != null) {
                    holder.itemView.setSelected(holder.mScript.getId() == selectedId);
                }
            }
        }

        @Override
        public void onScriptStateChanged(int state, boolean exceptFlag) {

        }

        @SuppressLint("NotifyDataSetChanged")
        void reloadData() {
            mScriptList = mScriptManager.getAllScripts();
            notifyDataSetChanged();
        }

        private class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener, View.OnCreateContextMenuListener {

            private final ImageView iv_icon;
            private final TextView tv_name;
            private final TextView tv_version;
            private final TextView tv_status;
            private final TextView tv_runtime;
            private final ImageView iv_setting;
            private Script mScript;

            ViewHolder(View view) {
                super(view);
                iv_icon = view.findViewById(R.id.iv_icon);
                tv_name = view.findViewById(R.id.tv_name);
                tv_version = view.findViewById(R.id.tv_version);
                tv_status = view.findViewById(R.id.tv_status);
                tv_runtime = view.findViewById(R.id.tv_runtime);
                ImageView iv_scheduler = view.findViewById(R.id.iv_scheduler);
                iv_setting = view.findViewById(R.id.iv_setting);

                itemView.setOnClickListener(this);
                itemView.setOnCreateContextMenuListener(this);
                iv_scheduler.setOnClickListener(v -> onScriptSchedulerClick(mScript));
                iv_setting.setOnClickListener(v -> onScriptOptionClick(mScript));
            }

            void bindData(Script script) {
                mScript = script;
                if (!script.getIconPath().isEmpty()) {
                    Bitmap bitmap = BitmapFactory.decodeFile(script.getIconPath());
                    iv_icon.setImageBitmap(bitmap);
                }
                tv_name.setText(script.getName());
                tv_version.setText(script.getVersionName());

                if (script.getLastRunTime() == 0) {
                    tv_runtime.setText(R.string.never_run);
                } else {
                    tv_runtime.setText(String.format(mMainActivity.getString(R.string.run_time), DateUtil.getLastTimeDescription(mMainActivity, script.getLastRunTime())));
                }

                String taskState = script.getLastQuitReason();
                if (taskState != null) {
                    switch (taskState) {
                        case "exception":
                        case "failure":
                            tv_status.setTextColor(0xFFFF0000);
                            break;
                        case "completion":
                            tv_status.setTextColor(0xFF00FF00);
                            break;
                        case "pending":
                            tv_status.setTextColor(0xFF0000FF);
                            break;
                        default:
                            tv_status.setTextColor(0xFFBBBBBB);
                            break;
                    }
                } else {
                    tv_status.setTextColor(0xFFBBBBBB);
                }

                itemView.setSelected(mScriptManager.getCurrentScriptId() == script.getId());
                iv_setting.setImageDrawable(ResourcesCompat.getDrawable(getResources(), R.drawable.ic_script_setting, null));
            }

            @Override
            public void onClick(View v) {
                if (mScriptManager.getRunningScriptId() != 0) {
                    ToastUtil.show(mMainActivity, R.string.forbid_switch);
                    return;
                }
                if (!mScriptManager.switchScript(mScript.getId())) {
                    ToastUtil.show(mMainActivity, R.string.switch_failed);
                }
            }

            @Override
            public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                mMenuSelectedScript = mScript;
                new MenuInflater(mMainActivity).inflate(R.menu.script_item, menu);
                menu.setHeaderTitle(mScript.getName());
            }
        }
    }
}
