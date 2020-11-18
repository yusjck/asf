#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include "common.h"
#include "logfile.h"
#include "errcode.h"
#include "luamain.h"

static lua_State *g_L = nullptr;
static bool g_bIsUserAbort = false;
static bool g_bResetScript = false;
static bool g_bShieldExcept = false;
static bool g_bExceptAbort = false;
static int g_iScriptRunState = SCRIPT_STOPPED;
static uint32_t g_iStopTimeoutTime;
static uint32_t g_scriptExitCode;
static LogFile g_logfile;
static std::string g_luaLoadPath;
static std::mutex g_threadSyncLock;
static LUA_EXIT_CB g_threadExitCb = nullptr;
static void *g_threadExitCbUd = nullptr;

std::string g_strScriptDir;     // 脚本和资源文件存放路径
std::string g_strTempDir;       // 可供脚本使用临时路径
std::string g_strPluginDir;     // so插件路径
std::string g_strLogDir;        // 日志输出路径
std::string g_strMainPath;      // 主脚本文件路径

ConfigUtil g_appConfig;
Controller *g_controller;
MsgQueue g_msgQueue;
ThreadEvent g_threadEvent;

static void SetScriptState(int iRunState)
{
	if (iRunState != g_iScriptRunState)
	{
		g_iScriptRunState = iRunState;
		if (g_bExceptAbort)
		{
			g_engineService.ReportScriptState(iRunState, g_scriptExitCode | SCRIPT_EXCEPTION_FLAG);
		}
		else
		{
			g_engineService.ReportScriptState(iRunState, g_scriptExitCode);
		}
	}
}

static int GetScriptState()
{
	return g_iScriptRunState;
}

void DoDelay(uint32_t iMilliseconds, bool bProcessEvent)
{
	uint32_t timeNow = GetTickCount();
	if (GetScriptState() == SCRIPT_STOPPING)
	{
		// 脚本运行停止例程中如果运行超时立即退出Delay
		if (g_iStopTimeoutTime - timeNow < iMilliseconds)
			iMilliseconds = g_iStopTimeoutTime - timeNow;
		g_threadEvent.Wait(iMilliseconds);
		return;
	}

	uint32_t timeEnd = timeNow + iMilliseconds;
	do
	{
		g_threadEvent.Reset();
		if (bProcessEvent && DispatchMessage(g_L) > 0)
		{
			timeNow = GetTickCount();
			if (timeNow >= timeEnd)
				return;
		}

		// 脚本睡眠指定时长，但可被中止执行事件或其它事件打断
		g_threadEvent.Wait(timeEnd - timeNow);
		// 如果用户发出了中止命令则立即退出Delay
		if (IsUserAbort())
			return;
		timeNow = GetTickCount();
		// 判断睡眠时长是否足够，不够可能是被外部事件打断了
	} while (timeNow < timeEnd);
}

void PrintScriptLog(uint32_t logColor, const char *log, bool saveToFile)
{
	time_t logTime = time(nullptr);
	g_engineService.ReportScriptLog((uint32_t)logTime, logColor, log);

	if (saveToFile)
	{
		if (!g_logfile.IsCreated())
		{
			LOGI("create log file: %s\n", g_strLogDir.c_str());
			g_logfile.Create(logTime, g_strLogDir.c_str());
		}

		g_logfile.Write(logTime, log);
	}
}

void AbortScript(const char *quitReason)
{
	// 脚本执行退出例程时调用AbortScript将不产生任何效果
	if (GetScriptState() == SCRIPT_STOPPING)
		return;
	if (quitReason != nullptr)
	{
		if (strcasecmp(quitReason, "completion") == 0)
		{
			g_scriptExitCode = SCRIPT_TASK_COMPLETION_FLAG;
		}
		else if (strcasecmp(quitReason, "failure") == 0)
		{
			g_scriptExitCode = SCRIPT_TASK_FAILURE_FLAG;
		}
		else if (strcasecmp(quitReason, "pending") == 0)
		{
			g_scriptExitCode = SCRIPT_TASK_PENDING_FLAG;
		}
	}
	g_bShieldExcept = true;
	luaL_error(g_L, "interrupted!");
}

void ResetScript()
{
	// 脚本执行退出例程时调用ResetScript将不产生任何效果
	if (GetScriptState() == SCRIPT_STOPPING)
		return;
	LOGI("reset script\n");
	g_bResetScript = true;
	g_bShieldExcept = true;
	luaL_error(g_L, "interrupted!");
}

bool IsUserAbort()
{
	return g_bIsUserAbort;
}

static void OnUserAbort()
{
	switch (GetScriptState())
	{
		case SCRIPT_STOPPING:	    	// 脚本停止中不处理
		case SCRIPT_STOPPED:
			g_bIsUserAbort = true;		// 用户发出停止信号，需防止脚本重启
			return;
		default:
			break;
	}
	g_bIsUserAbort = true;
	lua_sethook(g_L, [](lua_State *L, lua_Debug *ar) {
		lua_sethook(L, nullptr, 0, 0);
		g_bShieldExcept = true;
		luaL_error(L, "interrupted!");
		}, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
	g_threadEvent.Set();
}

static void PrintLuaError(int iErrCode)
{
	if (iErrCode == 0 || lua_isnil(g_L, -1))
		return;

	const char *msg = lua_tostring(g_L, -1);
	if (msg == nullptr)
		msg = "(error object is not a string)";

	char szErrorType[2000];
	switch (iErrCode)
	{
		case LUA_ERRSYNTAX:	// 编译时错误
			snprintf(szErrorType, sizeof(szErrorType), "%s\n%s", "syntax error during pre-compilation", msg);
			break;
		case LUA_ERRMEM:	// 内存错误
			snprintf(szErrorType, sizeof(szErrorType), "%s\n%s", "memory allocation error", msg);
			break;
		case LUA_ERRRUN:	// 运行时错误
			snprintf(szErrorType, sizeof(szErrorType), "%s\n%s", "a runtime error", msg);
			break;
		case LUA_YIELD:		// 线程被挂起错误
			snprintf(szErrorType, sizeof(szErrorType), "%s\n%s", "thread has suspended", msg);
			break;
		case LUA_ERRERR:	// 在进行错误处理时发生错误
			snprintf(szErrorType, sizeof(szErrorType), "%s\n%s", "error while running the error handler function", msg);
			break;
		default:
			snprintf(szErrorType, sizeof(szErrorType), "%s", msg);
			break;
	}
	lua_pop(g_L, 1);	// 打印完移除栈上的错误信息
	PrintScriptLog(0xff0000, szErrorType, true);       // 打印红色错误日志，并记录到日志文件中
}

int LoadScriptFile(const char *pScriptName)
{
	std::string filepath = pScriptName;
	if (filepath[0] != '/')
		filepath.insert(0, g_luaLoadPath + "/");

	const char *extname = strrchr(pScriptName, '.');
	if (extname == nullptr || (strcmp(extname, ".lua") != 0 && strcmp(extname, ".lc") != 0))
	{
		filepath += strrchr(g_strMainPath.c_str(), '.');
	}

	FileBuffer fb;
	if (!fb.LoadFromPrivateFile(filepath.c_str()))
	{
		lua_pushfstring(g_L, "cannot open file: %s", filepath.c_str());
		return -1;
	}

	const auto *scriptBuffer = (const uint8_t *)fb.GetBuffer();
	size_t scriptLength = fb.GetSize();

	if (scriptLength >= 16)
	{
		// 判断脚本是否加密
		if (memcmp(scriptBuffer, LUA_SIGNATURE, 4) == 0 && *(uint32_t *)(scriptBuffer + 4) == 0x59568783)
		{
			// 得到脚本对应的软件ID（软件ID同时也作为脚本加密KEY）
			uint8_t bScriptSoftId[8];
			memcpy(bScriptSoftId, scriptBuffer + 8, 8);
			*(uint32_t *)bScriptSoftId ^= 0xF181E070;
			*(uint32_t *)(bScriptSoftId + 4) ^= 0x7318B679;

			// 移除脚本文件中的加密头
			scriptBuffer += 16;
			scriptLength -= 16;

			// 解密脚本
			int n = min(scriptLength, 80);
			for (int i = 0; i < n; i++)
			{
				((uint8_t *)scriptBuffer)[i] ^= bScriptSoftId[i & 7];
			}
		}
	}

	const char *pName = strrchr(filepath.c_str(), '/');
	if (pName)
		pName++;
	else
		pName = filepath.c_str();

	/* 加载脚本 */
	return luaL_loadbuffer(g_L, (const char *)scriptBuffer, scriptLength, pName);
}

static int OnScriptInit(const char *pUserVar)
{
	g_scriptExitCode = 0;

	if (g_bResetScript)
		SetScriptState(SCRIPT_RESETING);
	else
		SetScriptState(SCRIPT_STARTING);

	lua_State *L = lua_open();  /* create state */
	if (L == nullptr)
		return -1;

	int status = lua_cpcall(L, [](lua_State *L) {
		const char *pUserVar = (const char *)lua_touserdata(L, 1);
		g_L = L;

		g_controller = Controller::NewInstance();
		if (g_controller == nullptr)
			luaL_error(L, "controller init failed");

		lua_gc(L, LUA_GCSTOP, 0);
		int top = lua_gettop(L);

		luaopen_base(L);
		luaopen_table(L);
		luaopen_os(L);
		luaopen_string(L);
		luaopen_math(L);
		luaopen_debug(L);
		luaopen_cobject(L);

		luaopen_color(L);
		luaopen_event(L);
		luaopen_input(L);
		luaopen_memory(L);
		luaopen_misc(L);
		luaopen_plugin(L);
		luaopen_script(L, pUserVar);

		luaopen_cjson(L);
		luaopen_bit(L);
		luaopen_file(L);
		luaopen_int64(L);
		luaopen_struct(L);

		lua_settop(L, top);
		lua_gc(L, LUA_GCRESTART, 0);
		return 0;
		}, (void *)pUserVar);
	if (status != 0)
		PrintLuaError(status);
	return status;
}

static int OnScriptRun()
{
	SetScriptState(SCRIPT_RUNNING);
	g_bShieldExcept = false;
	lua_sethook(g_L, nullptr, 0, 0);

	if (g_bResetScript)
	{
		LOGI("script reset successful\n");
		g_bResetScript = false;
	}

	const char *pScriptName = g_strMainPath.c_str();
	const char *p = strrchr(pScriptName, '/');
	if (p)
	{
		g_luaLoadPath = pScriptName;
		g_luaLoadPath.resize(p - pScriptName);
	}
	else
	{
		g_luaLoadPath = g_strScriptDir;
	}

	int status = LoadScriptFile(pScriptName);
	if (status != 0)
	{
		PrintLuaError(status);
	}
	else
	{
		/* 执行脚本 */
		status = lua_docall(g_L, 0, 1);
		if (status != 0 && !g_bShieldExcept)
		{
			LOGI("script exception\n");
			PrintLuaError(status);
			g_bExceptAbort = true;
		}
	}

	return status;
}

static int OnScriptExit()
{
	SetScriptState(SCRIPT_STOPPING);
	g_bShieldExcept = false;

	lua_getfield(g_L, LUA_REGISTRYINDEX, "user_cb.ScriptExit");
	// 判定用户是否注册了脚本ScriptExit处理函数
	if (!lua_isfunction(g_L, -1))
	{
		lua_pop(g_L, 1);
		return 0;
	}

	// 为退出例程增加一个5秒超时检测，防止退出脚本有问题导致退出卡死
	g_iStopTimeoutTime = GetTickCount() + 5000;
	lua_sethook(g_L, [](lua_State *L, lua_Debug *ar) {
		lua_sethook(L, nullptr, 0, 0);
		if (GetTickCount() > g_iStopTimeoutTime)
		{
			luaL_error(L, "run OnScriptExit() timeout");
		}
	}, LUA_MASKCOUNT, 1);

	LOGI("call OnScriptExit()\n");
	// 存在OnScriptExit就调用它
	int status = lua_docall(g_L, 0, 1);
	if (status == 0 || g_bShieldExcept)
	{
		LOGI("script exited\n");
	}
	else
	{
		LOGI("OnScriptExit() exception\n");
		PrintLuaError(status);
		g_bExceptAbort = true;
	}

	return status;
}

static void OnScriptCleanup()
{
	if (g_L)
	{
		lua_close(g_L);
		g_L = nullptr;
	}

	if (g_controller)
	{
		g_controller->Release();
		g_controller = nullptr;
	}

	// 清空消息队列
	g_msgQueue.ClearAll();

	// 如果用户中止了脚本，就清除重启标志
	if (g_bIsUserAbort)
	{
		g_bResetScript = false;
		SetScriptState(SCRIPT_STOPPED);
	}
	else if (g_bExceptAbort)
	{
		g_bResetScript = false;
		SetScriptState(SCRIPT_STOPPED);
	}
	else
	{
		if (!g_bResetScript)
		{
			SetScriptState(SCRIPT_STOPPED);
		}
	}
}

static void OnLuaThreadExit()
{
	std::lock_guard<std::mutex> lock(g_threadSyncLock);
	if (g_threadExitCb)
	{
		g_threadExitCb(g_threadExitCbUd);
		g_threadExitCb = nullptr;
		g_threadExitCbUd = nullptr;
	}
}

int LuaRunScript(const char *pScriptCfg, const char *pUserVar)
{
	// 脚本还在运行则返回调用失败
	if (GetScriptState() != SCRIPT_STOPPED)
		return -1;

	SetScriptState(SCRIPT_STARTING);

	g_appConfig.InitConfigs(pScriptCfg);
	g_strScriptDir = g_appConfig.GetString("ScriptDir");
	g_strTempDir = g_appConfig.GetString("TempDir");
	g_strPluginDir = g_appConfig.GetString("PluginDir");
	g_strLogDir = g_appConfig.GetString("LogDir");
	g_strMainPath = g_appConfig.GetString("MainPath");

	std::string userVar = pUserVar;
	std::thread luaThread([userVar]() {
		g_bIsUserAbort = false;
		g_bResetScript = false;
		g_bExceptAbort = false;

		do
		{
			if (OnScriptInit(userVar.c_str()) == 0)
			{
				OnScriptRun();
				OnScriptExit();
			}

			OnScriptCleanup();
		} while (!g_bIsUserAbort && g_bResetScript);

		// 线程退出前调用，用于清理环境和处理用户回调
		OnLuaThreadExit();
	});
	luaThread.detach();

	return 0;
}

void LuaAbort()
{
	OnUserAbort();
}

void LuaCleanup(LUA_EXIT_CB cb, void *ud)
{
	std::lock_guard<std::mutex> lock(g_threadSyncLock);
	// 中断运行中的脚本
	if (GetScriptState() != SCRIPT_STOPPED)
	{
		LuaAbort();
		g_threadExitCb = cb;
		g_threadExitCbUd = ud;
	}
	else
	{
		cb(ud);
	}
}

void LuaPushEvent(const char *event)
{
	g_msgQueue.PushMsg(event);
	g_threadEvent.Set();
}
