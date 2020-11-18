package com.rainman.asf;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.http.SslError;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDialog;

import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ProgressBar;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.userconfig.UserVar;
import com.rainman.asf.util.ToastUtil;

import java.io.File;
import java.lang.reflect.Type;
import java.util.Map;

public class OptionViewDialog extends AppCompatDialog {

    private static final String TAG = "OptionViewDialog";
    private Script mScript;
    private WebView mWebView;
    private UserVar mUserVar;
    private ProgressBar pb_loading;

    private OptionViewDialog(@NonNull Context context) {
        super(context, R.style.MyDialog);
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dialog_option_view);

        Window window = getWindow();
        if (window != null) {
            window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        }

        initView();
        initData();
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void initView() {
        pb_loading = findViewById(R.id.pb_loading);
        mWebView = findViewById(R.id.wv_option_view);

        View btn_save = findViewById(R.id.btn_save);
        assert btn_save != null;
        btn_save.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onSaveConfigs();
                dismiss();
            }
        });

        mWebView.addJavascriptInterface(this, "configMgr");

        WebSettings webSettings = mWebView.getSettings();
        webSettings.setJavaScriptEnabled(true);         // 允许执行JS脚本
        webSettings.setDomStorageEnabled(true);         // 允许键值存储
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            webSettings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);    // 允许HTTP+HTTPS混合使用
        }

        mWebView.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                super.onPageStarted(view, url, favicon);
                Log.i(TAG, "start loading page");
                pb_loading.setVisibility(View.VISIBLE);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                super.onPageFinished(view, url);
                Log.i(TAG, "page loading complete");
                pb_loading.setVisibility(View.GONE);
            }

            @Override
            public void onLoadResource(WebView view, String url) {
                super.onLoadResource(view, url);
                Log.i(TAG, "load resources: " + url);
            }

            @Override
            public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                super.onReceivedError(view, request, error);
                Log.i(TAG, error.toString());
            }

            @Override
            public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
                handler.proceed();      // 处理资源加载中的SSL证书错误
                super.onReceivedSslError(view, handler, error);
                Log.i(TAG, error.toString());
            }
        });
    }

    private void initData() {
        mUserVar = new UserVar(new File(mScript.getScriptDir()));
        mWebView.loadUrl("file://" + mScript.getOptionViewFile());
    }

    private void onSaveConfigs() {
        mWebView.loadUrl("javascript:onSaveConfigs()");
    }

    @JavascriptInterface
    public String getConfigs() {
        return new Gson().toJson(mUserVar.getUserVars());
    }

    @JavascriptInterface
    public void saveConfigs(String json) {
        Log.i(TAG, json);
        Type mapType = new TypeToken<Map<String, String>>() {
        }.getType();
        Map<String, String> configs = new Gson().fromJson(json, mapType);
        for (Map.Entry<String, String> entry : configs.entrySet()) {
            mUserVar.put(entry.getKey(), entry.getValue());
        }
        mUserVar.save();
        ToastUtil.show(this.getContext(), R.string.option_saved);
    }

    public static void popup(Context context, Script script) {
        OptionViewDialog dialog = new OptionViewDialog(context);
        dialog.mScript = script;
        Window dialogWindow = dialog.getWindow();
        if (dialogWindow != null) {
            if (!(context instanceof Activity)) {
                dialogWindow.setType(Build.VERSION.SDK_INT >= 26 ?
                        WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                        WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            }
        }
        dialog.show();
    }
}
