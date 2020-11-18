package com.rainman.asf.core.ipc;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;
import java.util.List;

class IpcRequest {

    private List<DataHead> mDataHeads = new ArrayList<>();

    void parseRequest(byte[] request) {
        ByteArrayInputStream inputStream = new ByteArrayInputStream(request);

        while (inputStream.available() > 0) {
            byte[] tmp = new byte[4];
            inputStream.read(tmp, 0, 4);
            DataHead dataHead = new DataHead();
            dataHead.type = tmp[0];
            dataHead.size = (tmp[1] & 0xff) | (tmp[2] << 8 & 0xff00) | (tmp[3] << 16 & 0xff0000);
            dataHead.data = new byte[dataHead.size];
            inputStream.read(dataHead.data, 0, dataHead.size);
            // 跳过内存对齐数据
            int alignlen = (dataHead.size + (dataHead.type == Constant.BDT_STR ? 1 : 0) + 3) & ~3;
            inputStream.skip(alignlen - dataHead.size);
            mDataHeads.add(dataHead);
        }
    }

    private DataHead getDataHead(int type) throws Exception {
        if (mDataHeads.isEmpty())
            throw new Exception("Invalid index");

        DataHead dataHead = mDataHeads.get(0);
        if (dataHead.type != type)
            throw new Exception("Invalid type");

        mDataHeads.remove(0);
        return dataHead;
    }

    private int bytes2int(byte[] value) {
        int tmp = value[0] & 0xff;
        tmp |= value[1] << 8 & 0xff00;
        tmp |= value[2] << 16 & 0xff0000;
        tmp |= value[3] << 24 & 0xff000000;
        return tmp;
    }

    boolean getBool() throws Exception {
        DataHead dataHead = getDataHead(Constant.BDT_BOOL);
        return dataHead.data[0] != 0;
    }

    int getInt() throws Exception {
        DataHead dataHead = getDataHead(Constant.BDT_INT);
        return bytes2int(dataHead.data);
    }

    float getFloat() throws Exception {
        DataHead dataHead = getDataHead(Constant.BDT_FLOAT);
        return Float.intBitsToFloat(bytes2int(dataHead.data));
    }

    String getString() throws Exception {
        DataHead dataHead = getDataHead(Constant.BDT_STR);
        return new String(dataHead.data);
    }

    byte[] getBin() throws Exception {
        DataHead dataHead = getDataHead(Constant.BDT_BIN);
        return dataHead.data;
    }
}
