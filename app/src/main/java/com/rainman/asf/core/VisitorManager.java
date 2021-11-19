package com.rainman.asf.core;

import android.app.AlertDialog;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.Window;
import android.view.WindowManager;

import com.rainman.asf.AppSetting;
import com.rainman.asf.R;
import com.rainman.asf.core.database.CoreDatabase;
import com.rainman.asf.core.database.Visitor;
import com.rainman.asf.core.database.VisitorDao;
import com.rainman.asf.util.Compat;

import java.util.Date;
import java.util.List;

public class VisitorManager {

    private static VisitorManager mInstance;
    private VisitorDao mVisitorDao;

    public static VisitorManager getInstance() {
        if (mInstance == null) {
            throw new RuntimeException("init first");
        }
        return mInstance;
    }

    /**
     * 该类在使用前必需先初始化
     *
     * @param context app上下文
     */
    public static void init(Context context) {
        mInstance = new VisitorManager();
        mInstance.mVisitorDao = CoreDatabase.getInstance(context).getVisitorDao();
    }

    public boolean checkPermission(Context context, String name, String signature) {
        if (!AppSetting.isAccessAuthorizationEnabled())
            return true;
        CheckPermissionProxy checkPermissionProxy = new CheckPermissionProxy();
        return checkPermissionProxy.checkPermission(context, name, signature);
    }

    public void addOrUpdateVisitor(Visitor visitor) {
        long id = mVisitorDao.addOrUpdate(visitor);
        if (visitor.getId() == 0) {
            visitor.setId(id);
        }
    }

    public void deleteVisitor(Visitor visitor) {
        mVisitorDao.delete(visitor);
    }

    public List<Visitor> getVisitors() {
        return mVisitorDao.getAll();
    }

    private class CheckPermissionProxy implements Runnable {

        private Context mContext;
        private String mName;
        private String mSignature;
        private Visitor mVisitor;

        @Override
        public void run() {
            mVisitor = mVisitorDao.findVisitor(mName, mSignature);
            if (mVisitor == null) {
                mVisitor = new Visitor(mName, mSignature);
                mVisitor.setAccessPermission(Visitor.PERMISSION_PROMPT);
            }
            if (mVisitor.isPermissionExpired() || mVisitor.getAccessPermission() == Visitor.PERMISSION_PROMPT) {
                requestPermission();
            } else {
                mVisitor.setLastAccessTime(new Date().getTime());
                completeAndNotify();
            }
        }

        boolean checkPermission(Context context, String name, String signature) {
            mContext = context;
            mName = name;
            mSignature = signature;
            new Handler(Looper.getMainLooper()).post(this);

            synchronized (this) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            return mVisitor.getAccessPermission() != Visitor.PERMISSION_DENIED;
        }

        private void completeAndNotify() {
            mVisitorDao.addOrUpdate(mVisitor);
            synchronized (this) {
                this.notify();
            }
        }

        private void requestPermission() {
            if (!Compat.canDrawOverlays(mContext)) {
                mVisitor.setAccessPermission(Visitor.PERMISSION_DENIED);
                completeAndNotify();
                return;
            }

            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setTitle(R.string.device_access_confirmation)
                    .setMessage(String.format(mContext.getString(R.string.device_access_prompt), mVisitor.getName(), mVisitor.getSignature()))
                    .setPositiveButton(R.string.dlg_allow, (dialog, which) -> {
                        mVisitor.setAccessPermission(Visitor.PERMISSION_GRANTED);
                        mVisitor.setLastAccessTime(new Date().getTime());
                        completeAndNotify();
                    })
                    .setNegativeButton(R.string.dlg_deny, (dialog, which) -> {
                        mVisitor.setAccessPermission(Visitor.PERMISSION_DENIED);
                        completeAndNotify();
                    })
                    .setCancelable(false);

            AlertDialog dialog = builder.create();
            Window window = dialog.getWindow();
            if (window != null) {
                window.setType(Build.VERSION.SDK_INT >= 26 ?
                        WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY :
                        WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            }
            dialog.show();
        }
    }
}
