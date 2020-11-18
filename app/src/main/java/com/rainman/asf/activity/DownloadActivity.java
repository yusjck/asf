package com.rainman.asf.activity;

import android.app.ProgressDialog;
import android.content.DialogInterface;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.text.Html;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.rainman.asf.R;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.script.DownloadItem;
import com.rainman.asf.script.DownloadManager;
import com.rainman.asf.script.ScriptImportHelper;
import com.rainman.asf.util.ToastUtil;

import org.xutils.common.Callback;
import org.xutils.http.RequestParams;
import org.xutils.x;

import java.io.File;

public class DownloadActivity extends AppCompatActivity {

    private static final String TAG = "DownloadActivity";
    private static final int IMPORT_SCRIPT = 200;
    private static final int UPDATE_SCRIPT = 201;
    private ProgressBar pb_loading;
    private DownloadManager mDownloadManager;
    private DownloadAdapter mDownloadAdapter;
    private ProgressDialog mProgressDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_download);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        RecyclerView rvDownloadList = findViewById(R.id.rv_download_list);
        rvDownloadList.setLayoutManager(new LinearLayoutManager(this));
        rvDownloadList.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));

        pb_loading = findViewById(R.id.pb_loading);
        pb_loading.setVisibility(View.VISIBLE);

        mDownloadAdapter = new DownloadAdapter();
        rvDownloadList.setAdapter(mDownloadAdapter);

        mDownloadManager = new DownloadManager();
        mDownloadManager.loadDownloadList(new DownloadManager.ListLoadListener() {
            @Override
            public void onLoadSuccess() {
                pb_loading.setVisibility(View.GONE);
                mDownloadAdapter.reloadData();
            }

            @Override
            public void onLoadFailure() {
                pb_loading.setVisibility(View.GONE);
                ToastUtil.show(DownloadActivity.this, R.string.get_download_list_failed);
            }
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void onDownloadClick(final DownloadItem mDownloadItem, final int action, final long scriptId) {
        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(this);
            mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mProgressDialog.setCancelable(false);
        }

        RequestParams requestParams = new RequestParams(mDownloadItem.getDownloadUrl());
        final Callback.Cancelable request = x.http().get(requestParams, new Callback.ProgressCallback<File>() {
            @Override
            public void onSuccess(File result) {
                Log.i(TAG, "download successful");
                mProgressDialog.dismiss();
                ScriptImportHelper importHelper = new ScriptImportHelper(DownloadActivity.this);
                switch (action) {
                    case IMPORT_SCRIPT:
                        if (!importHelper.importNetScript(result, mDownloadItem.getDownloadUrl())) {
                            ToastUtil.show(DownloadActivity.this, R.string.import_script_failed);
                        } else {
                            ToastUtil.show(DownloadActivity.this, R.string.import_script_successful);
                        }
                        break;
                    case UPDATE_SCRIPT:
                        if (!importHelper.updateNetScript(scriptId, result)) {
                            ToastUtil.show(DownloadActivity.this, R.string.update_script_failed);
                        } else {
                            ToastUtil.show(DownloadActivity.this, R.string.update_script_successful);
                        }
                        break;
                }
                mDownloadAdapter.reloadData();
            }

            @Override
            public void onError(Throwable ex, boolean isOnCallback) {
                Log.i(TAG, "download failed");
                mProgressDialog.dismiss();
                ToastUtil.show(DownloadActivity.this, R.string.download_script_failed);
            }

            @Override
            public void onCancelled(CancelledException cex) {
                Log.i(TAG, "cancel download");
                mProgressDialog.dismiss();
            }

            @Override
            public void onFinished() {
                Log.i(TAG, "end download");
                mProgressDialog.dismiss();
            }

            @Override
            public void onWaiting() {
                // 网络请求开始的时候调用
                Log.i(TAG, "waiting for download");
            }

            @Override
            public void onStarted() {
                // 下载的时候不断回调的方法
                Log.i(TAG, "start download");
                mProgressDialog.show();
            }

            @Override
            public void onLoading(long total, long current, boolean isDownloading) {
                // 当前的下载进度和文件总大小
                Log.i(TAG, "downloading...");
                mProgressDialog.setMessage(getString(R.string.downloading));
                mProgressDialog.setMax((int) total);
                mProgressDialog.setProgress((int) current);
            }
        });

        mProgressDialog.setButton(ProgressDialog.BUTTON_NEGATIVE, getString(R.string.dlg_cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                request.cancel();
            }
        });
    }

    private class DownloadAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

        private DownloadItem[] mDownloadItems;

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.view_download_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            ((ViewHolder) holder).bindData(mDownloadItems[position]);
        }

        @Override
        public int getItemCount() {
            return mDownloadItems == null ? 0 : mDownloadItems.length;
        }

        void reloadData() {
            mDownloadItems = mDownloadManager.getDownloadItems();
            notifyDataSetChanged();
        }

        private class ViewHolder extends RecyclerView.ViewHolder {

            private final ImageView iv_icon;
            private final TextView tv_name;
            private final TextView tv_version;
            private final Button btn_download;
            private final Button btn_update;
            private final Button btn_redownload;
            private final TextView tv_desc;
            private DownloadItem mDownloadItem;
            private Script mBindScript;

            ViewHolder(View view) {
                super(view);
                iv_icon = view.findViewById(R.id.iv_icon);
                tv_name = view.findViewById(R.id.tv_name);
                tv_version = view.findViewById(R.id.tv_version);
                btn_download = view.findViewById(R.id.btn_download);
                btn_update = view.findViewById(R.id.btn_update);
                btn_redownload = view.findViewById(R.id.btn_redownload);
                tv_desc = view.findViewById(R.id.tv_desc);

                btn_download.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onDownloadClick(mDownloadItem, IMPORT_SCRIPT, -1);
                    }
                });
                btn_update.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onDownloadClick(mDownloadItem, UPDATE_SCRIPT, mBindScript.getId());
                    }
                });
                btn_redownload.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        onDownloadClick(mDownloadItem, UPDATE_SCRIPT, mBindScript.getId());
                    }
                });
            }

            void bindData(DownloadItem downloadItem) {
                mDownloadItem = downloadItem;
                mBindScript = ScriptManager.getInstance().findScriptByGuid(downloadItem.getGuid());

                x.image().bind(iv_icon, downloadItem.getIconUrl());
                tv_name.setText(downloadItem.getName());
                tv_version.setText(Script.getVersionName(downloadItem.getVersionCode()));
                tv_desc.setText(downloadItem.getDescription());

                if (mBindScript == null) {
                    btn_download.setVisibility(View.VISIBLE);
                    btn_update.setVisibility(View.GONE);
                    btn_redownload.setVisibility(View.GONE);
                } else {
                    btn_download.setVisibility(View.GONE);

                    if (mBindScript.getVersionCode() < mDownloadItem.getVersionCode()) {
                        btn_update.setVisibility(View.VISIBLE);
                        btn_redownload.setVisibility(View.GONE);
                        String versionInfo = "<font color='#CCCCCC'>" + Script.getVersionName(mBindScript.getVersionCode()) + "→</font>"
                                + Script.getVersionName(downloadItem.getVersionCode());
                        tv_version.setText(Html.fromHtml(versionInfo));
                    } else {
                        btn_update.setVisibility(View.GONE);
                        btn_redownload.setVisibility(View.VISIBLE);
                    }
                }
            }
        }
    }
}
