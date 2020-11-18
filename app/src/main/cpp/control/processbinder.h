#ifndef _PROCESSBINDER_H
#define _PROCESSBINDER_H

#include <stdint.h>
#include <string>
#include <msgchannel.h>
#include <threadevent.h>

class CmdContext;

class ProcessBinder : private MsgWatcher
{
public:
	int BindProcess(uint32_t pid, uint32_t injectMethod);
	int UnbindProcess();
	int SearchCode(const char *startAddr, const char *endAddr, const char *featureCodes, std::string &result);
	int Read(const char *addr, void *buf, uint32_t readCount);
	int Write(const char *addr, const void *data, uint32_t dataSize);
	int LoadExtModule(const void *moduleCode, size_t moduleSize, uint32_t &extIndex);
	int FreeExtModule(uint32_t extIndex);
	int InvokeExtModule(uint32_t extIndex, const char *funName, const char *params, uint32_t timeOut, std::string &result);
	ProcessBinder();
	~ProcessBinder();

private:
	MsgChannel m_msgChannel;
	pid_t m_pid;
	ThreadEvent m_cmdCompleteNotify;
	CBuffer m_cmdReply;

	int SendCommand(CmdContext &ctx);
	virtual void OnReceive(CBuffer &msg);
	virtual void OnDisconnect();
};

#endif