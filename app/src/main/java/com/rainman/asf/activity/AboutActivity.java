package com.rainman.asf.activity;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.MenuItem;
import android.widget.TextView;

import com.rainman.asf.R;
import com.rainman.asf.util.RootUtil;
import com.rainman.asf.util.SystemUtils;
import com.rainman.asf.view.AboutItem;

public class AboutActivity extends AppCompatActivity {

    private TextView tvVersionName;
    private AboutItem mInfoSystemVersion;
    private AboutItem mInfoCpuAbi;
    private AboutItem mInfoRootStatus;
    private AboutItem mInfoWifiAddr;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        initView();
        initData();
    }

    private void initView() {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        tvVersionName = findViewById(R.id.tv_version_name);
        mInfoSystemVersion = findViewById(R.id.info_system_version);
        mInfoCpuAbi = findViewById(R.id.info_cpu_abi);
        mInfoRootStatus = findViewById(R.id.info_root_status);
        mInfoWifiAddr = findViewById(R.id.info_wifi_addr);
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

        mInfoSystemVersion.setContent("Android-" + Build.VERSION.SDK_INT);
        mInfoCpuAbi.setContent(Build.CPU_ABI);
        mInfoRootStatus.setContent(RootUtil.haveRoot() ? R.string.yes : R.string.no);
        mInfoWifiAddr.setContent(SystemUtils.getWifiIp(this));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
