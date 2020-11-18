package com.rainman.asf.script;

import com.google.gson.Gson;
import com.rainman.asf.util.Constant;
import org.xutils.common.Callback;
import org.xutils.http.RequestParams;
import org.xutils.x;

public class DownloadManager {

    private DownloadItem[] mDownloadItems;
    private ListLoadListener mListLoadListener;

    public interface ListLoadListener {

        void onLoadSuccess();

        void onLoadFailure();
    }

    public void loadDownloadList(ListLoadListener listLoadListener) {
        mListLoadListener = listLoadListener;
        RequestParams params = new RequestParams(Constant.SCRIPT_LIST_URL);
        x.http().get(params, new Callback.CommonCallback<String>() {
            @Override
            public void onSuccess(String result) {
                Gson gson = new Gson();
                mDownloadItems = gson.fromJson(result, DownloadItem[].class);
                mListLoadListener.onLoadSuccess();
            }

            @Override
            public void onError(Throwable ex, boolean isOnCallback) {
                mListLoadListener.onLoadFailure();
            }

            @Override
            public void onCancelled(CancelledException cex) {

            }

            @Override
            public void onFinished() {

            }
        });
    }

    public DownloadItem[] getDownloadItems() {
        return mDownloadItems;
    }
}
