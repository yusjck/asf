package com.rainman.asf.core.ipc;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

class IpcResponse {

    private List<DataHead> mDataHeads = new ArrayList<>();

    IpcResponse() {
        writeInt(Constant.ERR_NONE);    // 写入默认错误码
    }

    private byte[] int2bytes(int value) {
        byte[] tmp = new byte[4];
        tmp[0] = (byte) (value & 0xff);
        tmp[1] = (byte) (value >> 8 & 0xff);
        tmp[2] = (byte) (value >> 16 & 0xff);
        tmp[3] = (byte) (value >> 24 & 0xff);
        return tmp;
    }

    void setErrorCode(int errorCode) {
        DataHead dataHead = mDataHeads.get(0);
        mDataHeads.remove(0);
        dataHead.data = int2bytes(errorCode);
        mDataHeads.add(0, dataHead);    // 修改已写入的错误代码
    }

    void writeBool(boolean value) {
        DataHead dataHead = new DataHead();
        dataHead.type = Constant.BDT_BOOL;
        dataHead.data = new byte[1];
        dataHead.data[0] = value ? (byte) 1 : 0;
        dataHead.size = dataHead.data.length;
        mDataHeads.add(dataHead);
    }

    void writeInt(int value) {
        DataHead dataHead = new DataHead();
        dataHead.type = Constant.BDT_INT;
        dataHead.data = int2bytes(value);
        dataHead.size = dataHead.data.length;
        mDataHeads.add(dataHead);
    }

    void writeFloat(float value) {
        DataHead dataHead = new DataHead();
        dataHead.type = Constant.BDT_FLOAT;
        dataHead.data = int2bytes(Float.floatToIntBits(value));
        dataHead.size = dataHead.data.length;
        mDataHeads.add(dataHead);
    }

    void writeStr(String value) {
        DataHead dataHead = new DataHead();
        dataHead.type = Constant.BDT_STR;
        dataHead.data = value.getBytes();
        dataHead.size = dataHead.data.length;
        mDataHeads.add(dataHead);
    }

    void writeBin(byte[] value) {
        DataHead dataHead = new DataHead();
        dataHead.type = Constant.BDT_BIN;
        dataHead.data = value;
        dataHead.size = dataHead.data.length;
        mDataHeads.add(dataHead);
    }

    byte[] makeResponse() {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        for (DataHead dataHead : mDataHeads) {
            byte[] tmp = new byte[4];
            tmp[0] = (byte) (dataHead.type);
            tmp[1] = (byte) (dataHead.size & 0xff);
            tmp[2] = (byte) (dataHead.size >> 8 & 0xff);
            tmp[3] = (byte) (dataHead.size >> 16 & 0xff);
            outputStream.write(tmp, 0, 4);
            outputStream.write(dataHead.data, 0, dataHead.size);
            // 写入对齐数据，最多4字节
            int alignlen = (dataHead.size + (dataHead.type == Constant.BDT_STR ? 1 : 0) + 3) & ~3;
            outputStream.write(new byte[4], 0, alignlen - dataHead.size);
        }
        return outputStream.toByteArray();
    }
}
