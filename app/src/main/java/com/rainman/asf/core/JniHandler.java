package com.rainman.asf.core;

import androidx.annotation.Keep;

import com.rainman.asf.core.ipc.AndroidCaller;

public class JniHandler {

    @Keep
    private Object createAndroidCaller(ScriptActuator actuator) {
        return new AndroidCaller(actuator.getContext());
    }

    @Keep
    private void destroyAndroidCaller(Object androidCaller) {
        ((AndroidCaller) androidCaller).onDestroy();
    }

    @Keep
    private byte[] forwardAndroidCall(Object androidCaller, byte[] request) {
        return ((AndroidCaller) androidCaller).androidCallDispatch(request);
    }
}
