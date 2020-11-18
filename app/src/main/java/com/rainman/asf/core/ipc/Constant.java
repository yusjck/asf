package com.rainman.asf.core.ipc;

class Constant {

    public static final int BDT_BOOL = 0;
    public static final int BDT_INT = 1;
    public static final int BDT_UINT = 2;
    public static final int BDT_FLOAT = 3;
    public static final int BDT_STR = 4;
    public static final int BDT_BIN = 5;

    public static final int ERR_NONE = 0;
    public static final int ERR_INVALID_INVOKE = -1;
    public static final int ERR_INVALID_PARAMETER = -2;
    public static final int ERR_INVOKE_FAILED = -3;
    public static final int ERR_ACCESS_DENIED = -31;

    // lua引擎往java层调用命令
    public static final int ACCMD_MESSAGEBOX = 1;
    public static final int ACCMD_SHOWMESSAGE = 2;
    public static final int ACCMD_VIBRATE = 3;
    public static final int ACCMD_CALLANDROID = 4;
    public static final int ACCMD_SCREENSHOT = 5;
    public static final int ACCMD_GETDISPLAYINFO = 6;
    public static final int ACCMD_REQUESTCONTROL = 7;
}
