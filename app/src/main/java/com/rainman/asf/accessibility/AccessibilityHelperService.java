package com.rainman.asf.accessibility;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class AccessibilityHelperService extends AccessibilityService {

    private static final String TAG = "AccessibilityService";
    private static final ReentrantLock LOCK = new ReentrantLock();
    private static final Condition ENABLED = LOCK.newCondition();
    private static final List<GlobalKeyEventListener> keyListeners = new ArrayList<>();
    private static AccessibilityHelperService mInstance;

    public static AccessibilityHelperService getInstance() {
        return mInstance;
    }

    public static boolean waitForEnabled(long timeout) {
        if (mInstance != null)
            return true;

        try {
            LOCK.lock();
            if (mInstance != null)
                return true;

            if (timeout == -1) {
                ENABLED.await();
                return true;
            }

            return ENABLED.await(timeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            e.printStackTrace();
            return false;
        } finally {
            LOCK.unlock();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mInstance = null;
    }

    @Override
    protected void onServiceConnected() {
        super.onServiceConnected();
        Log.i(TAG, "onServiceConnected: " + getServiceInfo().toString());
        mInstance = this;
        LOCK.lock();
        ENABLED.signalAll();
        LOCK.unlock();
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
    }

    @Override
    public void onInterrupt() {
    }

    @Override
    protected boolean onKeyEvent(KeyEvent event) {
        Log.i(TAG, "onKeyEvent");
        for (GlobalKeyEventListener listener : keyListeners) {
            if (listener.onKeyEvent(event))
                return true;
        }
        return super.onKeyEvent(event);
    }

    public static void registerGlobalKeyEventListener(GlobalKeyEventListener listener) {
        keyListeners.add(listener);
    }

    public static void unregisterGlobalKeyEventListener(GlobalKeyEventListener listener) {
        keyListeners.remove(listener);
    }

    public interface GlobalKeyEventListener {

        boolean onKeyEvent(KeyEvent event);
    }
}
