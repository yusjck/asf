package com.rainman.asf.core.ipc;

import android.content.Context;
import android.graphics.Point;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;

import com.rainman.asf.core.VisitorManager;
import com.rainman.asf.core.api.Accessibility;
import com.rainman.asf.core.api.Device;
import com.rainman.asf.core.api.Net;
import com.rainman.asf.core.api.System;
import com.rainman.asf.core.screenshot.ScreenCapture;
import com.rainman.asf.util.SystemUtils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.lang.reflect.Method;

public class AndroidCaller {

    private final Context mContext;
    private IpcRequest mRequest;
    private IpcResponse mResponse;
    private final Accessibility mAccessibility;
    private final Device mDevice;
    private final Net mNet;
    private final System mSystem;
    private ScreenCapture.CaptureHandle mCaptureHandle;

    public AndroidCaller(Context context) {
        mContext = context;
        mAccessibility = new Accessibility(context);
        mDevice = new Device(context);
        mNet = new Net(context);
        mSystem = new System(context);
        mCaptureHandle = ScreenCapture.getCaptureHandle(context);
    }

    private void onMessageBox() throws Exception {
        String msg = mRequest.getString();
        int timeout = mRequest.getInt();
        SystemUtils.messageBox(mContext, msg, timeout);
    }

    private void onShowMessage() throws Exception {
        String msg = mRequest.getString();
        SystemUtils.showMessage(mContext, msg);
    }

    private void onVibrate() throws Exception {
        int duration = mRequest.getInt();
        SystemUtils.vibrate(mContext, duration);
    }

    private void onCallAndroid() throws Exception {
        String cmd = mRequest.getString();
        if (cmd.indexOf('.') == -1)
            throw new NoSuchMethodException();

        int dotIndex = cmd.indexOf('.');
        String module = cmd.substring(0, dotIndex);
        cmd = cmd.substring(dotIndex + 1);

        Object object;
        switch (module) {
            case "accessibility":
                object = mAccessibility;
                break;
            case "device":
                object = mDevice;
                break;
            case "net":
                object = mNet;
                break;
            case "system":
                object = mSystem;
                break;
            default:
                throw new NoSuchMethodException();
        }

        String args = mRequest.getString();
        JSONArray jsonParams = new JSONArray(args);

        Class<?> clazz = object.getClass();
        Method method = null;
        for (Method m : clazz.getMethods()) {
            if (m.getName().equals(cmd) && m.getParameterTypes().length == jsonParams.length()) {
                method = m;
                break;
            }
        }
        if (method == null)
            throw new NoSuchMethodException();

        Class<?>[] parameterTypes = method.getParameterTypes();
        int paramNum = parameterTypes.length;

        JSONObject jsonResult = new JSONObject();
        Object retval = null;

        if (paramNum == 0) {
            retval = method.invoke(object);
        } else {
            Object[] params = new Object[paramNum];

            for (int i = 0; i < paramNum; i++) {
                switch (parameterTypes[i].getName()) {
                    case "java.lang.String":
                        params[i] = jsonParams.getString(i);
                        break;
                    case "int":
                    case "java.lang.Integer":
                        params[i] = jsonParams.getInt(i);
                        break;
                    case "boolean":
                    case "java.lang.Boolean":
                        params[i] = jsonParams.getBoolean(i);
                        break;
                    default:
                        throw new NoSuchFieldException();
                }
            }

            switch (paramNum) {
                case 1:
                    retval = method.invoke(object, params[0]);
                    break;
                case 2:
                    retval = method.invoke(object, params[0], params[1]);
                    break;
                case 3:
                    retval = method.invoke(object, params[0], params[1], params[2]);
                    break;
                case 4:
                    retval = method.invoke(object, params[0], params[1], params[2], params[3]);
                    break;
                case 5:
                    retval = method.invoke(object, params[0], params[1], params[2], params[3], params[4]);
                    break;
            }
        }

        switch (method.getReturnType().getName()) {
            case "java.lang.String":
                jsonResult.put("result", retval);
                break;
            case "int":
            case "java.lang.Integer":
                assert retval != null;
                jsonResult.put("result", (int) retval);
                break;
            case "boolean":
            case "java.lang.Boolean":
                assert retval != null;
                jsonResult.put("result", (boolean) retval);
                break;
        }

        mResponse.writeStr(jsonResult.toString());
    }

    private void onScreenShot() throws Exception {
        int x = mRequest.getInt();
        int y = mRequest.getInt();
        int width = mRequest.getInt();
        int height = mRequest.getInt();

        if (mCaptureHandle == null) {
            mCaptureHandle = ScreenCapture.getCaptureHandle(mContext);
            if (mCaptureHandle == null) {
                mResponse.setErrorCode(Constant.ERR_INVOKE_FAILED);
                return;
            }
        }

        byte[] buf = mCaptureHandle.getScreenPixels(x, y, width, height);
        if (buf == null) {
            mResponse.setErrorCode(Constant.ERR_INVOKE_FAILED);
        } else {
            mResponse.writeBin(buf);
        }
    }

    private void onGetDisplayInfo() {
        Display defaultDisplay = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        int rotation = 0;
        switch (defaultDisplay.getRotation()) {
            case Surface.ROTATION_0:
                rotation = 0;
                break;
            case Surface.ROTATION_90:
                rotation = 90;
                break;
            case Surface.ROTATION_180:
                rotation = 180;
                break;
            case Surface.ROTATION_270:
                rotation = 270;
                break;
        }
        Point displayPoint = new Point();
        defaultDisplay.getRealSize(displayPoint);

        mResponse.writeInt(displayPoint.x);
        mResponse.writeInt(displayPoint.y);
        mResponse.writeInt(rotation);
    }

    private void onRequestControl() throws Exception {
        String sourceDeviceName = mRequest.getString();
        String deviceSignature = mRequest.getString();

        VisitorManager visitorManager = VisitorManager.getInstance();
        if (visitorManager.checkPermission(mContext, sourceDeviceName, deviceSignature)) {
            mResponse.setErrorCode(Constant.ERR_NONE);
        } else {
            mResponse.setErrorCode(Constant.ERR_ACCESS_DENIED);
        }
    }

    public byte[] androidCallDispatch(byte[] request) {
        mRequest = new IpcRequest();
        mResponse = new IpcResponse();

        try {
            mRequest.parseRequest(request);

            int cmd = mRequest.getInt();
            switch (cmd) {
                case Constant.ACCMD_MESSAGEBOX:
                    onMessageBox();
                    break;
                case Constant.ACCMD_SHOWMESSAGE:
                    onShowMessage();
                    break;
                case Constant.ACCMD_VIBRATE:
                    onVibrate();
                    break;
                case Constant.ACCMD_CALLANDROID:
                    onCallAndroid();
                    break;
                case Constant.ACCMD_SCREENSHOT:
                    onScreenShot();
                    break;
                case Constant.ACCMD_GETDISPLAYINFO:
                    onGetDisplayInfo();
                    break;
                case Constant.ACCMD_REQUESTCONTROL:
                    onRequestControl();
                    break;
            }
        } catch (Exception e) {
            e.printStackTrace();
            mResponse.setErrorCode(Constant.ERR_INVOKE_FAILED);
        }

        return mResponse.makeResponse();
    }

    public void onDestroy() {
        if (mCaptureHandle != null) {
            mCaptureHandle.release();
        }
    }
}
