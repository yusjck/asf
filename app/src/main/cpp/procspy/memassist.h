#ifndef _SPYCMD_H
#define _SPYCMD_H

#include <msgchannel.h>

class MemAssist : public MsgWatcher
{
public:
	bool StartListen();
	MemAssist();
	~MemAssist();

private:
	MsgChannel m_msgChannel;

	int SearchCode(const char *startAddr, const char *endAddr, const char *featureCodes, uintptr_t &foundAddr);
	int Read(const char *addr, void *buf, uint32_t readCount);
	int Write(const char *addr, const void *data, uint32_t dataSize);
	void OnSearchCode(BufferReader &msg, BufferWriter &reply);
	void OnRead(BufferReader &msg, BufferWriter &reply);
	void OnWrite(BufferReader &msg, BufferWriter &reply);
	void OnLoadExtModule(BufferReader &msg, BufferWriter &reply);
	void OnFreeExtModule(BufferReader &msg, BufferWriter &reply);
	void OnInvokeExtModule(BufferReader &msg, BufferWriter &reply);
	virtual void OnReceive(CBuffer &pack);
	virtual void OnDisconnect();
};

#endif