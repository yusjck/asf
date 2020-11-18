#include "common.h"
#include "inject.h"
#include "processbinder.h"
#include "processbindercmd.h"

class CmdContext : public BufferReader, public BufferWriter
{
public:
	const CBuffer &GetRequestBuffer()
	{
		return BufferWriter::GetBuf();
	}

	void SetResponseBuf(CBuffer &buf)
	{
		return BufferReader::SetBuf(buf);
	}

	CmdContext(int cmd)
	{
		WriteInt(cmd);
	}

	virtual ~CmdContext()
	{

	}
};

ProcessBinder::ProcessBinder()
{
	m_pid = -1;
}

ProcessBinder::~ProcessBinder()
{

}

int ProcessBinder::BindProcess(uint32_t pid, uint32_t injectMethod)
{
	if (m_msgChannel.IsConnected())
		return m_pid == pid ? ERR_NONE : ERR_INVOKE_FAILED;

	if (!m_msgChannel.Connect(format("@proc-conn-%d", pid).c_str(), this))
	{
		std::string soPath = GetModulePath(-1, (void *) &GetModulePath);
		soPath = soPath.substr(0, soPath.find_last_of('/')) + "/libprocspy.so";
		system(format("cp -f %s /data/local/tmp/libmemassist.so&&chmod 777 /data/local/tmp/libmemassist.so",
		              soPath.c_str()).c_str());
		if (inject_remote_process(pid, "/data/local/tmp/libmemassist.so", "main_entry", "Frist load") < 0)
			return ERR_INVOKE_FAILED;

		LOGI("Inject finished, pid=%d\n", pid);
		if (!m_msgChannel.Connect(format("@proc-conn-%d", pid).c_str(), this))
			return ERR_INVOKE_FAILED;
	}

	m_pid = pid;
	return ERR_NONE;
}

int ProcessBinder::UnbindProcess()
{
	m_msgChannel.Close();
	m_pid = -1;
	return ERR_NONE;
}

int ProcessBinder::SearchCode(const char *startAddr, const char *endAddr, const char *featureCodes, std::string &result)
{
	CmdContext ctx(PBC_SEARCH_CODE);
	ctx.WriteStr(startAddr);
	ctx.WriteStr(endAddr);
	ctx.WriteStr(featureCodes);

	int err = SendCommand(ctx);
	if (!IS_SUCCESS(err))
		return err;

	result = ctx.ReadStr();
	return ERR_NONE;
}

int ProcessBinder::Read(const char *addr, void *buf, uint32_t readCount)
{
	CmdContext ctx(PBC_READ);
	ctx.WriteStr(addr);
	ctx.WriteUint(readCount);

	int err = SendCommand(ctx);
	if (!IS_SUCCESS(err))
		return err;

	uint32_t len;
	const void *data = ctx.ReadBin(len);
	memcpy(buf, data, readCount);
	return ERR_NONE;
}

int ProcessBinder::Write(const char *addr, const void *data, uint32_t dataSize)
{
	CmdContext ctx(PBC_WRITE);
	ctx.WriteStr(addr);
	ctx.WriteBin(data, dataSize);
	return SendCommand(ctx);
}

int ProcessBinder::LoadExtModule(const void *moduleCode, size_t moduleSize, uint32_t &extIndex)
{
	const char *moduleName = "/data/local/tmp/libhello.so";
	FileBuffer fb;
	fb.Write(moduleCode, moduleSize);
	if (!fb.SaveToFile(moduleName))
	{
		LOGI("create %s failed", moduleName);
		return ERR_INVOKE_FAILED;
	}

	system(format("chmod 777 %s", moduleName).c_str());

	CmdContext ctx(PBC_LOAD_EXT);
	ctx.WriteStr(moduleName);

	int err = SendCommand(ctx);
	if (!IS_SUCCESS(err))
		return err;

	extIndex = ctx.ReadUint();
	return ERR_NONE;
}

int ProcessBinder::FreeExtModule(uint32_t extIndex)
{
	CmdContext ctx(PBC_FREE_EXT);
	ctx.WriteUint(extIndex);
	return SendCommand(ctx);
}

int ProcessBinder::InvokeExtModule(uint32_t extIndex, const char *funName, const char *params, uint32_t timeOut, std::string &result)
{
	CmdContext ctx(PBC_INVOKE_EXT);
	ctx.WriteUint(extIndex);
	ctx.WriteStr(funName);
	ctx.WriteStr(params);
	ctx.WriteUint(timeOut);

	int err = SendCommand(ctx);
	if (!IS_SUCCESS(err))
		return err;

	result = ctx.ReadStr();
	return ERR_NONE;
}

int ProcessBinder::SendCommand(CmdContext &ctx)
{
	m_cmdCompleteNotify.Reset();
	if (!m_msgChannel.Send(ctx.GetRequestBuffer()))
		return ERR_INVOKE_FAILED;

	m_cmdCompleteNotify.Wait();

	if (!m_msgChannel.IsConnected())
		return ERR_INVOKE_FAILED;

	ctx.SetResponseBuf(m_cmdReply);
	m_cmdReply.FreeBuffer();

	return ctx.ReadInt();
}

void ProcessBinder::OnReceive(CBuffer &msg)
{
	m_cmdReply = msg;
	m_cmdCompleteNotify.Set();
}

void ProcessBinder::OnDisconnect()
{
	m_cmdCompleteNotify.Set();
}
