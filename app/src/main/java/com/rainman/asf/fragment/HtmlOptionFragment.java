package com.rainman.asf.fragment;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.net.http.SslError;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
import com.rainman.asf.R;
import com.rainman.asf.userconfig.UserVar;
import com.rainman.asf.util.ToastUtil;

import java.io.File;
import java.lang.reflect.Type;
import java.util.Map;

public class HtmlOptionFragment extends Fragment {

    private static final String TAG = "HtmlOptionFragment";
    private UserVar mUserVar;
    private ProgressBar pb_loading;
    private WebView mWebView;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_html_option, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView(view);
        initData(savedInstanceState);
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void initView(View view) {
        pb_loading = view.findViewById(R.id.pb_loading);
        mWebView = view.findViewById(R.id.wv_option_view);

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

    private void initData(Bundle savedInstanceState) {
        Bundle bundle = getArguments();
        assert bundle != null;
        String scriptPath = bundle.getString("script_path");
        String configName = bundle.getString("config_name");
        String optionViewFile = bundle.getString("option_view_file");
        mUserVar = new UserVar(new File(scriptPath), configName);
        mWebView.loadUrl("file://" + optionViewFile);
    }

    public void onSaveConfigs() {
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
}
