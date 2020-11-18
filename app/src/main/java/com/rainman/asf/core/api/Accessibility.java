package com.rainman.asf.core.api;

import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.view.accessibility.AccessibilityNodeInfo;

import androidx.annotation.Keep;

import com.rainman.asf.accessibility.AccessibilityHelper;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;

@Keep
public class Accessibility {

    private final AccessibilityHelper mAccessibilityHelper;
    private AccessibilityNodeInfo mFoundNode = null;
    private Map<Integer, AccessibilityNodeInfo> mViews = new HashMap<>();

    public Accessibility(Context context) {
        mAccessibilityHelper = new AccessibilityHelper(context);
    }

    private JSONObject enumNodeInfo(AccessibilityNodeInfo nodeInfo) throws JSONException {
        int handle = nodeInfo.hashCode();
        mViews.put(handle, nodeInfo);
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("handle", handle);
        jsonObject.put("id", nodeInfo.getViewIdResourceName());
        jsonObject.put("text", nodeInfo.getText());
        jsonObject.put("desc", nodeInfo.getContentDescription());
        jsonObject.put("class", nodeInfo.getClassName());
        jsonObject.put("clickable", nodeInfo.isClickable());

        Rect rect = new Rect();
        nodeInfo.getBoundsInScreen(rect);
        JSONObject bounds = new JSONObject();
        bounds.put("left", rect.left);
        bounds.put("top", rect.top);
        bounds.put("right", rect.right);
        bounds.put("bottom", rect.bottom);
        jsonObject.put("bounds", bounds);
        jsonObject.put("x", rect.centerX());
        jsonObject.put("y", rect.centerY());

        if (nodeInfo.getChildCount() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (int i = 0; i < nodeInfo.getChildCount(); i++) {
                AccessibilityNodeInfo subNodeInfo = nodeInfo.getChild(i);
                if (subNodeInfo != null) {
                    JSONObject subJsonObject = enumNodeInfo(subNodeInfo);
                    jsonArray.put(subJsonObject);
                }
            }
            jsonObject.put("children", jsonArray);
        }

        return jsonObject;
    }

    public String getViewsInfo() {
        mViews.clear();
        AccessibilityNodeInfo accessibilityNodeInfo = mAccessibilityHelper.getRootNode();
        if (accessibilityNodeInfo == null) {
            return null;
        }

        try {
            JSONObject jsonObject = enumNodeInfo(accessibilityNodeInfo);
            return jsonObject.toString();
        } catch (JSONException e) {
            e.printStackTrace();
            return null;
        }
    }

    public String getViewInfo(int handle) {
        AccessibilityNodeInfo nodeInfo = mViews.get(handle);
        if (nodeInfo == null) {
            return null;
        }
        return nodeInfo.toString();
    }

    public boolean clickView(int handle) {
        AccessibilityNodeInfo nodeInfo = mViews.get(handle);
        if (nodeInfo == null) {
            return false;
        }
        mAccessibilityHelper.performClick(nodeInfo);
        return true;
    }

    public boolean longClickView(int handle) {
        AccessibilityNodeInfo nodeInfo = mViews.get(handle);
        if (nodeInfo == null) {
            return false;
        }
        mAccessibilityHelper.performLongClick(nodeInfo);
        return true;
    }

    public boolean findViewByText(String text, boolean clickable) {
        mFoundNode = mAccessibilityHelper.findNodeByText(text, clickable);
        return mFoundNode != null;
    }

    public boolean findViewByDesc(String desc, boolean clickable) {
        mFoundNode = mAccessibilityHelper.findNodeByDesc(desc, clickable);
        return mFoundNode != null;
    }

    public boolean findViewById(String id) {
        mFoundNode = mAccessibilityHelper.findNodeByViewId(id);
        return mFoundNode != null;
    }

    public void clickView() {
        if (mFoundNode != null) {
            mAccessibilityHelper.performClick(mFoundNode);
        }
    }

    public void longClickView() {
        if (mFoundNode != null) {
            mAccessibilityHelper.performLongClick(mFoundNode);
        }
    }

    public void back() {
        mAccessibilityHelper.performBackClick();
    }

    public void home() {
        mAccessibilityHelper.performHomeClick();
    }

    public void scrollBackward() {
        if (mFoundNode != null) {
            mAccessibilityHelper.performScrollBackward(mFoundNode);
        }
    }

    public void scrollForward() {
        if (mFoundNode != null) {
            mAccessibilityHelper.performScrollForward(mFoundNode);
        }
    }

    public void inputText(String text) {
        if (mFoundNode != null) {
            mAccessibilityHelper.inputText(mFoundNode, text);
        }
    }

    public void gesture(int startTime, int duration, String pointsJson) throws JSONException {
        JSONArray jsonArray = new JSONArray(pointsJson);
        Point[] points = new Point[jsonArray.length()];
        for (int i = 0; i < jsonArray.length(); i++) {
            JSONArray coord = jsonArray.getJSONArray(i);
            points[i] = new Point();
            points[i].x = coord.getInt(0);
            points[i].y = coord.getInt(1);
        }
        mAccessibilityHelper.gesture(startTime, duration, points);
    }

    public boolean isServiceAvailable() {
        return mAccessibilityHelper.isAccessibilityServiceAvailable();
    }
}
