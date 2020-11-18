package com.rainman.asf.core;

public class ScriptException extends RuntimeException {

    public ScriptException(String message) {
        super(message);
        ScriptLogger.addError(message);
    }
}
