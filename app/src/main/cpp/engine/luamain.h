#ifndef LUAMAIN_H
#define LUAMAIN_H

#include <string>
#include <configutil.h>
#include "luastub.h"
#include "engineservice.h"
#include "../control/controller.h"
#include "msgqueue.h"

#define SCRIPT_STARTING 0
#define SCRIPT_RUNNING 1
#define SCRIPT_STOPPING 2
#define SCRIPT_STOPPED 3
#define SCRIPT_ALERTING 4
#define SCRIPT_BLOCKING 5
#define SCRIPT_SUSPENDING 6
#define SCRIPT_RESETING 7

#define SCRIPT_EXCEPTION_FLAG 1U
#define SCRIPT_TASK_COMPLETION_FLAG 2U
#define SCRIPT_TASK_FAILURE_FLAG 4U
#define SCRIPT_TASK_PENDING_FLAG 8U

extern std::string g_strScriptDir;
extern std::string g_strTempDir;
extern std::string g_strPluginDir;
extern std::string g_strMainPath;

extern EngineService g_engineService;
extern ConfigUtil g_appConfig;
extern Controller *g_controller;
extern MsgQueue g_msgQueue;
extern ThreadEvent g_threadEvent;

typedef void (*LUA_EXIT_CB)(void *ud);

int LuaRunScript(const char *pScriptCfg, const char *pUserVar);
void LuaAbort();
void LuaCleanup(LUA_EXIT_CB cb, void *ud);
void LuaPushEvent(const char *event);

int DispatchMessage(lua_State *L);
int lua_GotoCmdFailed(lua_State *L, int iRet);
void AbortScript(const char *quitReason);
void ResetScript();
bool IsUserAbort();
void PrintScriptLog(uint32_t logColor, const char *log, bool saveToFile);
void DoDelay(uint32_t iMilliseconds, bool bProcessEvent);
int LoadScriptFile(const char *pScriptName);

int luaopen_color(lua_State *L);
int luaopen_event(lua_State *L);
int luaopen_input(lua_State *L);
int luaopen_memory(lua_State *L);
int luaopen_misc(lua_State *L);
int luaopen_plugin(lua_State *L);
int luaopen_script(lua_State *L, const char *pUserVar);

extern "C" int luaopen_cjson(lua_State *l);
int luaopen_bit(lua_State *L);
int luaopen_file(lua_State *L);
int luaopen_int64(lua_State *L);
int luaopen_struct(lua_State *L);

#endif
