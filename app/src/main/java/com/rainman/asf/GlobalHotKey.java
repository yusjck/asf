package com.rainman.asf;

import android.util.Log;
import android.view.KeyEvent;

import com.rainman.asf.accessibility.AccessibilityHelperService;
import com.rainman.asf.core.ScriptEngine;

public class GlobalHotKey implements AccessibilityHelperService.GlobalKeyEventListener {

    private static final String TAG = "GlobalHotKey";
    private static GlobalHotKey mGlobalHotKey = new GlobalHotKey();

    @Override
    public boolean onKeyEvent(KeyEvent event) {
        int key = event.getKeyCode();

        if (AppSetting.isVolumnKeyControl()) {
            switch (key) {
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                    Log.i(TAG, "KEYCODE_VOLUME_DOWN");
                    ScriptEngine.getInstance().startScript();
                    break;
                case KeyEvent.KEYCODE_VOLUME_UP:
                    Log.i(TAG, "KEYCODE_VOLUME_UP");
                    ScriptEngine.getInstance().stopScript();
                    break;
            }
        }
        return false;
    }

    static void registerGlobalHotKey() {
        // 注册热键
        AccessibilityHelperService.registerGlobalKeyEventListener(mGlobalHotKey);
    }

    static void unregisterGlobalHotKey() {
        AccessibilityHelperService.unregisterGlobalKeyEventListener(mGlobalHotKey);
    }
}
