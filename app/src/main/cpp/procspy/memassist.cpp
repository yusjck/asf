#include <unistd.h>
#include <map>
#include <dlfcn.h>
#include "common.h"
#include "memassist.h"
#include "../control/processbindercmd.h"

static int GetModuleAddress(const char *moduleName, uintptr_t *moduleAddr)
{
	*moduleAddr = (uintptr_t)GetModuleAddress(-1, moduleName);
	return *moduleAddr == 0 ? ERR_GET_MODULE_ADDR_FAILED : ERR_NONE;
}

static int ReadMemory(uintptr_t addr, uint32_t size, void *buffer)
{
	try
	{
		memcpy(buffer, (void *)addr, size);
		return ERR_NONE;
	}
	catch (...)
	{
		return ERR_READ_MEMORY_FAILED;
	}
}

static int WriteMemory(uintptr_t addr, const void *data, uint32_t size)
{
	try
	{
		memcpy((void *)addr, data, size);
		return ERR_NONE;
	}
	catch (...)
	{
		return ERR_WRITE_MEMORY_FAILED;
	}
}

static int calculate_num(uintptr_t num1, uintptr_t num2, short op, uintptr_t *result)
{
	switch (op)
	{
		case '+':
			*result = num1 + num2;
			return ERR_NONE;
		case '-':
			*result = num1 - num2;
			return ERR_NONE;
		default:
			return ERR_INVALID_EXPRESSION;
	}
}

static int find_last_op(const char *str)
{
	int len = (int)strlen(str);
	int	n = 0;

	while (len > 0)
	{
		if ((str[len] == '+' || str[len] == '-') && n == 0)
			return len;
		if (str[len] == '[')
			n--;
		else if (str[len] == ']')
			n++;
		len--;
	}

	return -1;
}

static int parse_str_value(const char *str, uintptr_t *result)
{
	std::string exp = str;
	exp = trim(exp);

	if (exp[0] == '<')
	{
		if (exp[exp.length() - 1] != '>')
			return ERR_INVALID_EXPRESSION;
		std::string name = exp.substr(1, exp.length() - 2);
		return GetModuleAddress(name.c_str(), result);
	}

	if (exp.length() > 8)
		return ERR_INVALID_EXPRESSION;

	for (size_t i = 0; i < exp.length(); i++)
	{
		if ((exp[i] | 0x20) >= 'a' && (exp[i] | 0x20) <= 'f')
			continue;
		if (exp[i] >= '0' && exp[i] <= '9')
			continue;
		return ERR_INVALID_EXPRESSION;
	}

	*result = strtoul(exp.c_str(), NULL, 16);
	return ERR_NONE;
}

static int calculate_exp(const char *str, uintptr_t *result)
{
	std::string exp = str;
	exp = trim(exp);
	int last_op = find_last_op(exp.c_str());		// 找到最后运算的运算符

	if (last_op == -1 && exp[0] != '[')
	{
		return parse_str_value(exp.c_str(), result);
	}
	else
	{
		int stack = 0;
		if (last_op != -1)
		{
			for (int i = 0; i < last_op; i++)
			{
				if (exp[i] == '[')
					stack++;
				else if (exp[i] == ']')
					stack--;
			}
		}

		if (stack != 0 || last_op == -1)	// last_op 在括号内 或者 括号内根本没有运算符
		{
			/*
			* 脱括号，递归调用
			*/
			std::string tmp = exp.substr(1, exp.length() - 2);
			uintptr_t num;
			int ret = calculate_exp(tmp.c_str(), &num);
			if (ret != ERR_NONE)
				return ret;
			return ReadMemory(num, sizeof(uint32_t), result);
		}
		else			// last_op 在括号外
		{
			/*
			* 把 last_op 两边的表达式分别计算，再合并结果
			*/
			std::string left_str = exp.substr(0, last_op);
			std::string right_str = exp.substr(last_op + 1);
			uintptr_t num1, num2;
			int ret = calculate_exp(left_str.c_str(), &num1);
			if (ret != ERR_NONE)
				return ret;
			ret = calculate_exp(right_str.c_str(), &num2);
			if (ret != ERR_NONE)
				return ret;
			return calculate_num(num1, num2, exp[last_op], result);
		}
	}
}

typedef std::map<uint32_t, void *> EXTEND_DLL_LIST;
static EXTEND_DLL_LIST g_ExtendDllLoaded;

static void LoadDllExtend(const char *moduleName, uint32_t &dllExtendIndex)
{
	do
	{
		static uint32_t sExtendDllCount = 0;
		void *hExtendLib = NULL;

		try
		{
			hExtendLib = dlopen(moduleName, RTLD_NOW);
		}
		catch (...)
		{
			LOGI("MemAssist: load %s except", moduleName);
			break;
		}

		if (hExtendLib == NULL)
		{
			LOGI("MemAssist: load %s failed", moduleName);
			break;
		}

		g_ExtendDllLoaded[++sExtendDllCount] = hExtendLib;
		dllExtendIndex = sExtendDllCount;
		LOGI("MemAssist: load %s success, idx=%u", moduleName, sExtendDllCount);
	} while (0);
}

static void FreeDllExtend(uint32_t dllExtendIndex)
{
	LOGI("MemAssist: free so cmd, idx=%u", dllExtendIndex);
	auto iter = g_ExtendDllLoaded.find(dllExtendIndex);
	if (iter == g_ExtendDllLoaded.end())
	{
		LOGI("MemAssist: invalid idx %u", dllExtendIndex);
		return;
	}
	void *hExtendLib = iter->second;
	try
	{
		dlclose(hExtendLib);
	}
	catch (...)
	{
		LOGI("MemAssist: unload so except");
	}
	g_ExtendDllLoaded.erase(iter);

}

static void FreeAllDllExtend()
{
	LOGI("MemAssist: unload all so");
	EXTEND_DLL_LIST::iterator iter = g_ExtendDllLoaded.begin();
	while (iter != g_ExtendDllLoaded.end())
	{
		void *hExtendLib = iter->second;
		try
		{
			dlclose(hExtendLib);
		}
		catch (...)
		{
			LOGI("MemAssist: unload so except");
		}
		iter++;
	}
	g_ExtendDllLoaded.clear();
}

static void InvokeDllExtend(uint32_t dllExtendIndex, const char *funName, const char *params, std::string &strResult)
{
	LOGI("MemAssist: invoke so cmd, idx=%u, cmd=%s,%s", dllExtendIndex, funName, params);

	do
	{
		if (g_ExtendDllLoaded.find(dllExtendIndex) == g_ExtendDllLoaded.end())
		{
			LOGI("MemAssist: invalid idx %u", dllExtendIndex);
			break;
		}

		void *hExtendLib = g_ExtendDllLoaded[dllExtendIndex];
		typedef const char *(*EXTENDCOMMAND)(const char *commandLine);
		typedef void (*FREESTRING)(const char *buffer);

		EXTENDCOMMAND ExtendCommand = (EXTENDCOMMAND)dlsym(hExtendLib, funName);
		FREESTRING FreeCommand = (FREESTRING)dlsym(hExtendLib, "FreeString");
		if (ExtendCommand == NULL)
		{
			LOGI("MemAssist: cmd %s not found", funName);
			break;
		}

		try
		{
			const char *lpstr = ExtendCommand(params);
			if (lpstr)
			{
				strResult = lpstr;
				if (FreeCommand)
					FreeCommand(lpstr);
			}
			LOGI("MemAssist: invoke so success");
		}
		catch (...)
		{
			LOGI("MemAssist: invoke so except");
			break;
		}
	} while (0);
}

MemAssist::MemAssist()
{

}

MemAssist::~MemAssist()
{

}

int MemAssist::SearchCode(const char *startAddr, const char *endAddr, const char *featureCodes, uintptr_t &foundAddr)
{
	uintptr_t startAddress, endAddress;
	int ret = calculate_exp(startAddr, &startAddress);
	if (ret != ERR_NONE)
		return ret;

	ret = calculate_exp(endAddr, &endAddress);
	if (ret != ERR_NONE)
		return ret;

	if (endAddress <= startAddress)
		return ERR_INVALID_PARAMETER;

	try
	{
		foundAddr = (uintptr_t)MemorySearch((uint8_t *)startAddress, endAddress - startAddress, featureCodes);
		return ERR_NONE;
	}
	catch (...)
	{
		return ERR_READ_MEMORY_FAILED;
	}
}

int MemAssist::Read(const char *addr, void *buf, uint32_t readCount)
{
	uintptr_t readAddr;
	int ret = calculate_exp(addr, &readAddr);
	if (ret != ERR_NONE)
		return ret;
	return ReadMemory(readAddr, readCount, buf);
}

int MemAssist::Write(const char *addr, const void *data, uint32_t dataSize)
{
	uintptr_t writeAddr;
	int ret = calculate_exp(addr, &writeAddr);
	if (ret != ERR_NONE)
		return ret;
	return WriteMemory(writeAddr, data, dataSize);
}

void MemAssist::OnSearchCode(BufferReader &msg, BufferWriter &reply)
{
	const char *startAddr = msg.ReadStr();
	const char *endAddr = msg.ReadStr();
	const char *featureCodes = msg.ReadStr();
	uintptr_t foundAddr = 0;
	int ret = SearchCode(startAddr, endAddr, featureCodes, foundAddr);
	reply.WriteInt(ret);
	reply.WriteStr(format("%p", foundAddr).c_str());
}

void MemAssist::OnRead(BufferReader &msg, BufferWriter &reply)
{
	const char *addr = msg.ReadStr();
	uint32_t readCount = msg.ReadUint();
	CBuffer buf;
	uint8_t *p = (uint8_t *)buf.GetBuffer(readCount);
	if (p == nullptr)
	{
		reply.WriteInt(ERR_ALLOC_MEMORY_FAILED);
		return;
	}
	int ret = Read(addr, p, readCount);
	if (!IS_SUCCESS(ret))
	{
		reply.WriteInt(ret);
		return;
	}
	reply.WriteInt(ERR_NONE);
	reply.WriteBin(p, readCount);
}

void MemAssist::OnWrite(BufferReader &msg, BufferWriter &reply)
{
	const char *addr = msg.ReadStr();
	uint32_t dataSize;
	const void *data = msg.ReadBin(dataSize);
	reply.WriteInt(Write(addr, data, dataSize));
}

void MemAssist::OnLoadExtModule(BufferReader &msg, BufferWriter &reply)
{
	const char *moduleName = msg.ReadStr();
	uint32_t dllExtendIndex = ~0U;
	LoadDllExtend(moduleName, dllExtendIndex);
	reply.WriteInt(ERR_NONE);
	reply.WriteUint(dllExtendIndex);
}

void MemAssist::OnFreeExtModule(BufferReader &msg, BufferWriter &reply)
{
	uint32_t dllExtendIndex = msg.ReadUint();
	FreeDllExtend(dllExtendIndex);
	reply.WriteInt(ERR_NONE);
}

void MemAssist::OnInvokeExtModule(BufferReader &msg, BufferWriter &reply)
{
	uint32_t dllExtendIndex = msg.ReadUint();
	const char *funName = msg.ReadStr();
	const char *params = msg.ReadStr();
	std::string result;
	InvokeDllExtend(dllExtendIndex, funName, params, result);
	reply.WriteInt(ERR_NONE);
	reply.WriteStr(result.c_str());
}

bool MemAssist::StartListen()
{
	return m_msgChannel.Listen(format("@proc-conn-%d", getpid()).c_str(), this);
}

void MemAssist::OnReceive(CBuffer &pack)
{
	BufferReader reader(pack);
	BufferWriter writer;
	int cmd = reader.ReadInt();
	switch (cmd)
	{
		case PBC_SEARCH_CODE:
			OnSearchCode(reader, writer);
			break;
		case PBC_READ:
			OnRead(reader, writer);
			break;
		case PBC_WRITE:
			OnWrite(reader, writer);
			break;
		case PBC_LOAD_EXT:
			OnLoadExtModule(reader, writer);
			break;
		case PBC_FREE_EXT:
			OnFreeExtModule(reader, writer);
			break;
		case PBC_INVOKE_EXT:
			OnInvokeExtModule(reader, writer);
			break;
		default:
			LOGE("MemAssist: invalid memory command");
			writer.WriteInt(ERR_INVALID_INVOKE);
			break;
	}
	m_msgChannel.Send(writer);
}

void MemAssist::OnDisconnect()
{
	FreeAllDllExtend();
}
