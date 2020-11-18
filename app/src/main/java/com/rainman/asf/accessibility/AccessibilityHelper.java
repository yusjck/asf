package com.rainman.asf.accessibility;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.GestureDescription;
import android.annotation.TargetApi;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.graphics.Path;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityNodeInfo;

import com.rainman.asf.R;
import com.rainman.asf.core.ScriptException;
import com.rainman.asf.util.RootUtil;

import java.util.Locale;

public class AccessibilityHelper {

    private static final String TAG = "AccessibilityHelper";
    private Context mContext;

    public AccessibilityHelper(Context context) {
        mContext = context.getApplicationContext();
    }

    private AccessibilityHelperService getAccessibilityService() {
        AccessibilityHelperService service = AccessibilityHelperService.getInstance();
        if (service == null)
            throw new ScriptException(mContext.getString(R.string.acessibility_service_unavailable));
        return service;
    }

    public boolean isAccessibilityServiceAvailable() {
        return AccessibilityHelperService.getInstance() != null;
    }

    /**
     * Check当前辅助服务是否启用
     */
    public boolean isAccessibilityServiceEnabled() {
        ComponentName expectedComponentName = new ComponentName(mContext, AccessibilityHelperService.class);

        String enabledServicesSetting = Settings.Secure.getString(mContext.getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES);
        if (enabledServicesSetting == null)
            return false;

        TextUtils.SimpleStringSplitter colonSplitter = new TextUtils.SimpleStringSplitter(':');
        colonSplitter.setString(enabledServicesSetting);

        while (colonSplitter.hasNext()) {
            String componentNameString = colonSplitter.next();
            ComponentName enabledService = ComponentName.unflattenFromString(componentNameString);

            if (enabledService != null && enabledService.equals(expectedComponentName))
                return true;
        }

        return false;
    }

    public boolean enableAccessibilityServiceByRoot() {
        if (!RootUtil.haveRoot())
            return false;

        String cmd = "enabled=$(settings get secure enabled_accessibility_services)\n" +
                "pkg=%s\n" +
                "if [[ $enabled == *$pkg* ]]\n" +
                "then\n" +
                "echo already_enabled\n" +
                "else\n" +
                "enabled=$pkg:$enabled\n" +
                "settings put secure enabled_accessibility_services $enabled\n" +
                "fi";
        ComponentName serviceName = new ComponentName(mContext, AccessibilityHelperService.class);
        RootUtil.executeRootCmdSilent(String.format(Locale.getDefault(), cmd, serviceName));

        return AccessibilityHelperService.waitForEnabled(2000);
    }

    public void ensureAccessibilityServiceEnabled() {
        if (AccessibilityHelperService.getInstance() != null)
            return;

        String errorMessage;
        if (isAccessibilityServiceEnabled()) {
            errorMessage = mContext.getString(R.string.accessibility_service_not_running);
        } else {
            errorMessage = mContext.getString(R.string.accessibility_service_not_enabled);
        }

        throw new RuntimeException(errorMessage);
    }

    public AccessibilityNodeInfo getRootNode() {
        return getAccessibilityService().getRootInActiveWindow();
    }

    private interface AccessibilityNodeComparer {
        boolean isFound(AccessibilityNodeInfo nodeInfo);
    }

    private AccessibilityNodeInfo findNode(AccessibilityNodeInfo nodeInfo, AccessibilityNodeComparer comparer) {
        if (comparer.isFound(nodeInfo)) {
            return nodeInfo;
        }
        for (int i = 0; i < nodeInfo.getChildCount(); i++) {
            AccessibilityNodeInfo subNode = nodeInfo.getChild(i);
            if (subNode == null) {
                continue;
            }
            AccessibilityNodeInfo foundNode = findNode(subNode, comparer);
            if (foundNode != null) {
                return foundNode;
            }
        }
        return null;
    }

    /**
     * 查找对应viewId的节点
     */
    public AccessibilityNodeInfo findNodeByViewId(final String viewId) {
        AccessibilityNodeInfo accessibilityNodeInfo = getRootNode();
        if (accessibilityNodeInfo == null) {
            return null;
        }
        return findNode(accessibilityNodeInfo, new AccessibilityNodeComparer() {
            @Override
            public boolean isFound(AccessibilityNodeInfo nodeInfo) {
                if (nodeInfo.getViewIdResourceName() == null)
                    return false;
                return viewId.equals(nodeInfo.getViewIdResourceName());
            }
        });
    }

    /**
     * 查找对应文本的节点
     */
    public AccessibilityNodeInfo findNodeByText(final String text, final boolean clickable) {
        AccessibilityNodeInfo accessibilityNodeInfo = getRootNode();
        if (accessibilityNodeInfo == null) {
            return null;
        }
        return findNode(accessibilityNodeInfo, new AccessibilityNodeComparer() {
            @Override
            public boolean isFound(AccessibilityNodeInfo nodeInfo) {
                if (nodeInfo.getText() == null)
                    return false;
                return text.contentEquals(nodeInfo.getText()) && nodeInfo.isClickable() == clickable;
            }
        });
    }

    /**
     * 查找对应描述的节点
     */
    public AccessibilityNodeInfo findNodeByDesc(final String desc, final boolean clickable) {
        AccessibilityNodeInfo accessibilityNodeInfo = getRootNode();
        if (accessibilityNodeInfo == null) {
            return null;
        }
        return findNode(accessibilityNodeInfo, new AccessibilityNodeComparer() {
            @Override
            public boolean isFound(AccessibilityNodeInfo nodeInfo) {
                if (nodeInfo.getContentDescription() == null)
                    return false;
                return desc.contentEquals(nodeInfo.getContentDescription()) && nodeInfo.isClickable() == clickable;
            }
        });
    }

    /**
     * 模拟点击事件
     */
    public void performClick(AccessibilityNodeInfo nodeInfo) {
        nodeInfo.performAction(AccessibilityNodeInfo.ACTION_CLICK);
    }

    /**
     * 模拟长点击事件
     */
    public void performLongClick(AccessibilityNodeInfo nodeInfo) {
        nodeInfo.performAction(AccessibilityNodeInfo.ACTION_LONG_CLICK);
    }

    /**
     * 模拟返回操作
     */
    public void performBackClick() {
        getAccessibilityService().performGlobalAction(AccessibilityService.GLOBAL_ACTION_BACK);
    }

    /**
     * 模拟HOME键
     */
    public void performHomeClick() {
        getAccessibilityService().performGlobalAction(AccessibilityService.GLOBAL_ACTION_HOME);
    }

    /**
     * 模拟下滑操作
     */
    public void performScrollBackward(AccessibilityNodeInfo nodeInfo) {
        nodeInfo.performAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
    }

    /**
     * 模拟上滑操作
     */
    public void performScrollForward(AccessibilityNodeInfo nodeInfo) {
        nodeInfo.performAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
    }

    /**
     * 模拟输入
     */
    public void inputText(AccessibilityNodeInfo nodeInfo, String text) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Bundle arguments = new Bundle();
            arguments.putCharSequence(AccessibilityNodeInfo.ACTION_ARGUMENT_SET_TEXT_CHARSEQUENCE, text);
            nodeInfo.performAction(AccessibilityNodeInfo.ACTION_SET_TEXT, arguments);
        } else {
            ClipboardManager clipboard = (ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clip = ClipData.newPlainText("label", text);
            clipboard.setPrimaryClip(clip);
            nodeInfo.performAction(AccessibilityNodeInfo.ACTION_FOCUS);
            nodeInfo.performAction(AccessibilityNodeInfo.ACTION_PASTE);
        }
    }

    @TargetApi(Build.VERSION_CODES.N)
    public void gesture(long startTime, long duration, Point[] points) {
        if (Build.VERSION.SDK_INT < 24)
            throw new ScriptException(mContext.getString(R.string.system_function_not_supported));

        Path path = new Path();
        path.moveTo(points[0].x, points[0].y);
        for (int i = 1; i < points.length; i++) {
            path.lineTo(points[i].x, points[i].y);
        }

        GestureDescription.Builder builder = new GestureDescription.Builder();
        GestureDescription gestureDescription = builder
                .addStroke(new GestureDescription.StrokeDescription(path, startTime, duration))
                .build();

        getAccessibilityService().dispatchGesture(gestureDescription, null, null);
    }
}
