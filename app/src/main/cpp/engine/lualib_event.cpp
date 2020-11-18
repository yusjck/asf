//
// Created by Jack on 2019/8/11.
//
#include "common.h"
#include "luamain.h"

static void RegisterUserCallback(lua_State *L)
{
	const char *funName = luaL_checkstring(L, 1);
	luaL_argcheck(L, lua_type(L, 2) == LUA_TFUNCTION, 2, "must be function");

	// 注册事件回调函数
	std::string cbName = (std::string)"user_cb." + funName;
	lua_pushvalue(L, 2);
	lua_setfield(L, LUA_REGISTRYINDEX, cbName.c_str());
}

static void UnregisterUserCallback(lua_State *L)
{
	const char *funName = luaL_checkstring(L, 1);

	// 移除已注册的回调函数
	std::string cbName = (std::string)"user_cb." + funName;
	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, cbName.c_str());
}

static bool CallUserCallback(lua_State *L, const char *funName, const char *param)
{
	std::string cbName = (std::string)"user_cb." + funName;
	// 取出用户注册的回调函数并调用
	lua_getfield(L, LUA_REGISTRYINDEX, cbName.c_str());
	if (lua_isfunction(L, -1))
	{
		lua_pushstring(L, param);
		lua_call(L, 1, 0);
		return true;
	}
	else
	{
		lua_pop(L, 1);
		return false;
	}
}

int DispatchMessage(lua_State *L)
{
	int dispatchCount = 0;
	for (;;)
	{
		std::string msg;
		if (!g_msgQueue.PopMsg(msg))
			break;

		size_t idx = msg.find('#');
		std::string eventName = msg.substr(0, idx);
		std::string eventParam = msg.substr(idx + 1);

		if (CallUserCallback(L, eventName.c_str(), eventParam.c_str()))
			dispatchCount++;
	}
	return dispatchCount;
}

static const char *GetErrorInfo(int iRet)
{
	switch (iRet)
	{
		case ERR_INVALID_INVOKE:
			return "invalid invoke";
		case ERR_INVALID_PARAMETER:
			return "invalid parameter";
		case ERR_INVOKE_FAILED:
			return "invoke failed";
		case ERR_OPEN_FILE_FAILED:
			return "open file failed";
		case ERR_SAVE_FILE_FAILED:
			return "save file failed";
		case ERR_WINAPI_INVOKE_FAILED:
			return "windows api invoke failed";
		case ERR_NOT_FOUND:
			return "not found";
		case ERR_NOT_EXIST:
			return "not exist";
		case ERR_RUN_APP_FAILED:
			return "run app failed";
		case ERR_INVALID_ACCOUNT:
			return "invalid account";
		case ERR_ALLOC_MEMORY_FAILED:
			return "memory alloc failed";
		case ERR_BKAPI_INVOKE_FALIED:
			return "bkapi invoke failed";
		case ERR_SLAVE_DISCONNECTED:
			return "slave disconnected";
		case ERR_BKAPI_RETURN_ERROR:
			return "bkapi return error";
		case ERR_NAME_NOT_EXIST:
			return "name dose not exist";
		case ERR_AREA_INVALID:
			return "area invalid";
		case ERR_WINDOW_BLOCKED:
			return "window blocked";
		case ERR_WINDOW_BOUND:
			return "window bound by other script";
		case ERR_LOAD_LOADER_FAILED:
			return "load bkapi loader failed";
		case ERR_INVALID_WINDOW:
			return "invalid Window";
		case ERR_LOAD_BKAPI_FAILED:
			return "load bkapi failed";
		case ERR_INIT_CAPTURE_FAILED:
			return "init capture failed";
		case ERR_INIT_BKGND_FAILED:
			return "init bkgnd failed";
		case ERR_INVALID_EXPRESSION:
			return "invalid expression";
		case ERR_READ_MEMORY_FAILED:
			return "read process memory failed";
		case ERR_WRITE_MEMORY_FAILED:
			return "write process memory failed";
		case ERR_GET_MODULE_ADDR_FAILED:
			return "get module address failed";
		case ERR_LOAD_DLL_EXTEND_FAILED:
			return "load dll extend failed";
		case ERR_TIME_OUT:
			return "time out";
		case ERR_USER_ABORT:
			return "user abort";
		default:
			return "not error description";
	}
}

static void OnCommandFailed(lua_State *L, int iRet)
{
	// 尝试调用用户注册的错误处理函数
	std::string param = format("%d|%s", iRet, GetErrorInfo(iRet));
	if (CallUserCallback(L, "InternalError", param.c_str()))
		return;

	// 调用用户函数失败就执行默认处理
	if (iRet == ERR_SLAVE_DISCONNECTED)
	{
		LOGI("cmd server disconnect");
		ResetScript();
	}
}

int lua_GotoCmdFailed(lua_State *L, int iRet)
{
	OnCommandFailed(L, iRet);
	lua_pushboolean(L, 0);
	lua_pushstring(L, GetErrorInfo(iRet));
	return 2;
}

static int lua_RegisterCallback(lua_State *L)
{
	RegisterUserCallback(L);
	return 0;
}

static int lua_UnregisterCallback(lua_State *L)
{
	UnregisterUserCallback(L);
	return 0;
}

static int lua_WaitEvent(lua_State *L)
{
	uint32_t iMilliseconds = (uint32_t)luaL_checkinteger(L, 1);
	g_threadEvent.Wait(iMilliseconds);
	std::string msg;
	if (g_msgQueue.PopMsg(msg))
	{
		size_t idx = msg.find('#');
		std::string eventName = msg.substr(0, idx);
		std::string eventParam = msg.substr(idx + 1);
		lua_pushstring(L, eventName.c_str());
		lua_pushstring(L, eventParam.c_str());
		return 2;
	}

	lua_pushboolean(L, 0);
	return 1;
}

int luaopen_event(lua_State *L)
{
	static const luaL_Reg eventlib[] = {
			{"RegisterCallback", lua_RegisterCallback},
			{"UnregisterCallback", lua_UnregisterCallback},
			{"WaitEvent", lua_WaitEvent},
			{nullptr, nullptr}
	};

	luaI_openlib(L, "event", eventlib, 0);
	return 1;
}
