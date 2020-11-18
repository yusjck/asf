#include <unistd.h>
#include <configutil.h>
#include "common.h"
#include "luamain.h"

static ConfigUtil g_uservar;
static ConfigUtil g_sharevar;

static int lua_LogPrint(lua_State *L)
{
	const char *strLog = luaL_checkstring(L, 1);
	uint32_t iLogColor = (uint32_t)luaL_optint(L, 2, -1);
	bool bSaveToFile = lua_toboolean(L, 3) != 0;

	uint8_t *p = (uint8_t *)&iLogColor;
	uint8_t t = p[0];
	p[0] = p[2];
	p[2] = t;
	LOGI("script output: %s\n", strLog);
	PrintScriptLog(iLogColor, strLog, bSaveToFile);
	return 0;
}

static int lua_Delay(lua_State *L)
{
	uint32_t iMilliseconds = (uint32_t)luaL_checkinteger(L, 1);
	bool bProcessEvent = luaL_optint(L, 2, 0) == 1;
	DoDelay(iMilliseconds, bProcessEvent);
	return 0;
}

static int lua_AbortScript(lua_State *L)
{
	const char *quitReason = lua_tostring(L, 1);
	AbortScript(quitReason);
	return 0;
}

static int lua_ResetScript(lua_State *L)
{
	ResetScript();
	return 0;
}

static int lua_IsUserAbort(lua_State *L)
{
	lua_pushboolean(L, IsUserAbort());
	return 1;
}

static int lua_LoadScript(lua_State *L)
{
	const char *strScriptFile = luaL_checkstring(L, 1);
	int status = LoadScriptFile(strScriptFile);
	if (status != 0)
		lua_error(L);
	// 执行加载的脚本文件，由于是从脚本里回调过来的，如果发生异常将从上一个pcall处直接退出
	lua_call(L, 0, 0);
	return 0;
}

static int lua_GetShareVar(lua_State *L)
{
	const char *strVarName = lua_tostring(L, 1);
	const char *strDefaultValue = luaL_optstring(L, 2, "");
	std::string strValue = g_sharevar.GetString(strVarName, strDefaultValue);
	lua_pushstring(L, strValue.c_str());
	return 1;
}

static int lua_SetShareVar(lua_State *L)
{
	const char *strVarName = lua_tostring(L, 1);
	const char *strVarValue = lua_tostring(L, 2);
	g_sharevar.SetString(strVarName, strVarValue);
	return 0;
}

static int lua_GetUserVar(lua_State *L)
{
	const char *strVarName = lua_tostring(L, 1);
	const char *strDefaultValue = luaL_optstring(L, 2, "");
	std::string strValue = g_uservar.GetString(strVarName, strDefaultValue);
	lua_pushstring(L, strValue.c_str());
	return 1;
}

static int lua_GetScriptDir(lua_State *L)
{
	lua_pushstring(L, g_strScriptDir.c_str());
	return 1;
}

static int lua_GetTempDir(lua_State *L)
{
	lua_pushstring(L, g_strTempDir.c_str());
	return 1;
}

int luaopen_script(lua_State *L, const char *pUserVar)
{
	g_uservar.InitConfigs(pUserVar);

	static const luaL_Reg scriptlib[] = {
			{"LogPrint", lua_LogPrint},
			{"Delay", lua_Delay},
			{"IsUserAbort", lua_IsUserAbort},
			{"AbortScript", lua_AbortScript},
			{"ResetScript", lua_ResetScript},
			{"LoadScript", lua_LoadScript},
			{"GetShareVar", lua_GetShareVar},
			{"SetShareVar", lua_SetShareVar},
			{"GetGlobalVar", lua_GetShareVar},
			{"SetGlobalVar", lua_SetShareVar},
			{"GetUserVar", lua_GetUserVar},
			{"GetScriptDir", lua_GetScriptDir},
			{"GetTempDir", lua_GetTempDir},
			{NULL, NULL}
	};

	luaI_openlib(L, "script", scriptlib, 0);
	return 1;
}
