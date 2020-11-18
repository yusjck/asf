#include "common.h"
#include "luamain.h"

static int lua_BindProcess(lua_State *L)
{
	uint32_t iProcessId = luaL_checkinteger(L, 1);
	uint32_t iInjectMethod = luaL_optinteger(L, 2, 0);

	int iRet = g_controller->m_process.BindProcess(iProcessId, iInjectMethod);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	// 创建用于保存扩展模块索引号的表
	lua_createtable(L, 0, 0);
	lua_setfield(L, LUA_ENVIRONINDEX, "ext_mod_list");

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_UnbindProcess(lua_State *L)
{
	int iRet = g_controller->m_process.UnbindProcess();
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	// 销毁表清空所有扩展模块索引号
	lua_pushnil(L);
	lua_setfield(L, LUA_ENVIRONINDEX, "ext_mod_list");

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_SearchCode(lua_State *L)
{
	const char *pStartAddress = luaL_checkstring(L, 1);
	const char *pEndAddress = luaL_checkstring(L, 2);
	const char *pCode = luaL_checkstring(L, 3);

	std::string strResult;
	int iRet = g_controller->m_process.SearchCode(pStartAddress, pEndAddress, pCode, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

static int lua_ReadInteger(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	int iType = luaL_optinteger(L, 2, 3);

	int iValue = 0;
	uint32_t iReadBytes;
	int iRet;
	switch (iType)
	{
		case 1:
			iReadBytes = 1;
			iRet = g_controller->m_process.Read(pAddress, &iValue, iReadBytes);
			break;
		case 2:
			iReadBytes = 2;
			iRet = g_controller->m_process.Read(pAddress, &iValue, iReadBytes);
			break;
		case 3:
			iReadBytes = 4;
			iRet = g_controller->m_process.Read(pAddress, &iValue, iReadBytes);
			break;
		default:
			iRet = ERR_INVALID_PARAMETER;
			break;
	}

	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushinteger(L, iValue);
	return 1;
}

static int lua_WriteInteger(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	int iValue = luaL_checkinteger(L, 2);
	int iType = luaL_optinteger(L, 3, 3);

	uint32_t iWriteBytes;
	int iRet;
	switch (iType)
	{
		case 1:
			iWriteBytes = 1;
			iRet = g_controller->m_process.Write(pAddress, &iValue, iWriteBytes);
			break;
		case 2:
			iWriteBytes = 2;
			iRet = g_controller->m_process.Write(pAddress, &iValue, iWriteBytes);
			break;
		case 3:
			iWriteBytes = 4;
			iRet = g_controller->m_process.Write(pAddress, &iValue, iWriteBytes);
			break;
		default:
			iRet = ERR_INVALID_PARAMETER;
			break;
	}

	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_ReadFloat(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	int iType = luaL_optinteger(L, 2, 2);

	double dfValue = 0;
	uint32_t dwReadBytes;
	int iRet;
	switch (iType)
	{
		case 2:
			dwReadBytes = sizeof(dfValue);
			iRet = g_controller->m_process.Read(pAddress, &dfValue, dwReadBytes);
			break;
		case 1:
		{
			float fTemp = 0;
			dwReadBytes = sizeof(fTemp);
			iRet = g_controller->m_process.Read(pAddress, &fTemp, dwReadBytes);
			dfValue = fTemp;
		}
			break;
		default:
			iRet = ERR_INVALID_PARAMETER;
			break;
	}

	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushnumber(L, dfValue);
	return 1;
}

static int lua_WriteFloat(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	double dfValue = luaL_checknumber(L, 2);
	int iType = luaL_optinteger(L, 3, 2);

	uint32_t dwWriteBytes;
	int iRet;
	switch (iType)
	{
		case 2:
			dwWriteBytes = sizeof(dfValue);
			iRet = g_controller->m_process.Write(pAddress, &dfValue, dwWriteBytes);
			break;
		case 1:
		{
			float fTemp = (float)dfValue;
			dwWriteBytes = sizeof(fTemp);
			iRet = g_controller->m_process.Write(pAddress, &fTemp, dwWriteBytes);
		}
			break;
		default:
			iRet = ERR_INVALID_PARAMETER;
			break;
	}

	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_ReadString(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	uint32_t iMaxLength = luaL_checkinteger(L, 2);

	CBuffer readbuf;
	char *pBuf = (char *)readbuf.GetBuffer(iMaxLength + 1);
	if (pBuf == NULL)
		return lua_GotoCmdFailed(L, ERR_ALLOC_MEMORY_FAILED);

	memset(pBuf, 0, iMaxLength + 1);
	int iRet = g_controller->m_process.Read(pAddress, pBuf, iMaxLength);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, pBuf);
	return 1;
}

static int lua_WriteString(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	const char *pString = luaL_checkstring(L, 2);
	uint32_t iMaxLength = luaL_checkinteger(L, 3);

	int iRet = g_controller->m_process.Write(pAddress, pString, iMaxLength);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_ReadBinary(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	uint32_t iLength = luaL_checkinteger(L, 2);

	CBuffer readbuf;
	uint8_t *pBuf = (uint8_t *)readbuf.GetBuffer(iLength);
	if (pBuf == NULL)
		return lua_GotoCmdFailed(L, ERR_ALLOC_MEMORY_FAILED);

	int iRet = g_controller->m_process.Read(pAddress, pBuf, iLength);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	CBuffer strhex;
	char *pHex = (char *)strhex.GetBuffer(iLength * 2 + 1);
	if (pHex == NULL)
		return lua_GotoCmdFailed(L, ERR_ALLOC_MEMORY_FAILED);

	BinaryToHexString(pHex, iLength * 2 + 1, pBuf, iLength);
	lua_pushstring(L, pHex);
	return 1;
}

static int lua_WriteBinary(lua_State *L)
{
	const char *pAddress = luaL_checkstring(L, 1);
	const char *pHexString = luaL_checkstring(L, 2);

	std::string strHexString = pHexString;
	string_replace(strHexString, " ", "");

	if (strHexString.length() % 2 == 1)
		strHexString += "0";

	CBuffer writebuf;
	uint8_t *pBuf = (uint8_t *)writebuf.GetBuffer(strHexString.length() / 2);
	if (pBuf == NULL)
		return lua_GotoCmdFailed(L, ERR_ALLOC_MEMORY_FAILED);

	HexStringToBinary(pBuf, strHexString.length() / 2, strHexString.c_str());
	uint32_t iWriteBytes = strHexString.length() / 2;
	int iRet = g_controller->m_process.Write(pAddress, pBuf, iWriteBytes);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_InvokeExtModule(lua_State *L)
{
	const char *pModuleName = luaL_checkstring(L, 1);
	const char *pFunName = luaL_checkstring(L, 2);
	const char *pCommand = luaL_checkstring(L, 3);
	uint32_t iTimeOut = luaL_optinteger(L, 4, 15 * 1000);

	uint32_t iExtModuleIndex = 0;
	lua_getfield(L, LUA_ENVIRONINDEX, "ext_mod_list");  // 压入扩展模块索引表
	if (!lua_istable(L, -1))
		return lua_GotoCmdFailed(L, ERR_INVALID_INVOKE);

	lua_getfield(L, -1, pModuleName);      // 判断模块是否存在
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		FileBuffer fb;
		if (!fb.LoadFromPrivateFile(pModuleName))
			return lua_GotoCmdFailed(L, ERR_LOAD_DLL_EXTEND_FAILED);

		int iRet = g_controller->m_process.LoadExtModule(fb.GetBuffer(), fb.GetSize(), iExtModuleIndex);
		if (!IS_SUCCESS(iRet))
			return lua_GotoCmdFailed(L, iRet);

		lua_pushinteger(L, iExtModuleIndex);
		lua_setfield(L, -2, pModuleName);  // 保存索引号
	}
	else
	{
		iExtModuleIndex = luaL_checkinteger(L, -1); // 从表中获取模块索引号
		lua_pop(L, 1);
	}
	lua_pop(L, 1);  // 移除扩展模块索引表

	std::string strResult;
	int iRet = g_controller->m_process.InvokeExtModule(iExtModuleIndex, pFunName, pCommand, iTimeOut, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

int luaopen_memory(lua_State *L)
{
	static const luaL_Reg memorylib[] = {
			{"BindProcess", lua_BindProcess},
			{"UnbindProcess", lua_UnbindProcess},
			{"SearchCode", lua_SearchCode},
			{"ReadInteger", lua_ReadInteger},
			{"WriteInteger", lua_WriteInteger},
			{"ReadFloat", lua_ReadFloat},
			{"WriteFloat", lua_WriteFloat},
			{"ReadString", lua_ReadString},
			{"WriteString", lua_WriteString},
			{"ReadBinary", lua_ReadBinary},
			{"WriteBinary", lua_WriteBinary},
			{"InvokeExtModule", lua_InvokeExtModule},
			{"InvokeDllExtend", lua_InvokeExtModule},
			{NULL, NULL}
	};

	luaI_openlib(L, "memory", memorylib, 0);
	return 1;
}
