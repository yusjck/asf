#include "common.h"
#include "luamain.h"

static int lua_KeyDown(lua_State *L)
{
	int iVirtKey = luaL_checkinteger(L, 1);

	int iRet = g_controller->m_input.KeyDown(iVirtKey);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_KeyUp(lua_State *L)
{
	int iVirtKey = luaL_checkinteger(L, 1);

	int iRet = g_controller->m_input.KeyUp(iVirtKey);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_KeyPress(lua_State *L)
{
	int iVirtKey = luaL_checkinteger(L, 1);
	uint32_t iDuration = (uint32_t)luaL_optinteger(L, 2, 0);

	int iRet = g_controller->m_input.KeyPress(iVirtKey, iDuration);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_SendString(lua_State *L)
{
	uint32_t iSendMode = (uint32_t)luaL_checkinteger(L, 1);
	const char *pString = luaL_checkstring(L, 2);

	int iRet = g_controller->m_input.SendString(iSendMode, pString);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_TouchMove(lua_State *L)
{
	int iX = luaL_checkinteger(L, 1);
	int iY = luaL_checkinteger(L, 2);
	uint32_t iFlags = (uint32_t)luaL_optinteger(L, 3, 0);
	int iPointId = luaL_optinteger(L, 4, 0);

	int iRet = g_controller->m_input.TouchMove(iPointId, iX, iY, iFlags);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_TouchDown(lua_State *L)
{
	int iX = luaL_checkinteger(L, 1);
	int iY = luaL_checkinteger(L, 2);
	int iPointId = luaL_optinteger(L, 3, 0);

	int iRet = g_controller->m_input.TouchDown(iPointId, iX, iY);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_TouchUp(lua_State *L)
{
	int iPointId = luaL_optinteger(L, 1, 0);

	int iRet = g_controller->m_input.TouchUp(iPointId);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_TouchTap(lua_State *L)
{
	int iX = luaL_checkinteger(L, 1);
	int iY = luaL_checkinteger(L, 2);
	int iPointId = luaL_optinteger(L, 3, 0);

	int iRet = g_controller->m_input.TouchTap(iPointId, iX, iY);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_TouchSwipe(lua_State *L)
{
	int iX1 = luaL_checkinteger(L, 1);
	int iY1 = luaL_checkinteger(L, 2);
	int iX2 = luaL_checkinteger(L, 3);
	int iY2 = luaL_checkinteger(L, 4);
	uint32_t iDuration = (uint32_t)luaL_optinteger(L, 5, 0);
	int iPointId = luaL_optinteger(L, 6, 0);

	int iRet = g_controller->m_input.TouchSwipe(iPointId, iX1, iY1, iX2, iY2, iDuration);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

int luaopen_input(lua_State *L)
{
	static const luaL_Reg inputlib[] = {
			{"KeyDown", lua_KeyDown},
			{"KeyUp", lua_KeyUp},
			{"KeyPress", lua_KeyPress},
			{"KeyClick", lua_KeyPress},
			{"SendString", lua_SendString},
			{"TouchMove", lua_TouchMove},
			{"TouchDown", lua_TouchDown},
			{"TouchUp", lua_TouchUp},
			{"TouchTap", lua_TouchTap},
			{"TouchSwipe", lua_TouchSwipe},
			{"Tap", lua_TouchTap},
			{"Swipe", lua_TouchSwipe},
			{NULL, NULL}
	};

	luaI_openlib(L, "input", inputlib, 0);
	return 1;
}
