#include "common.h"
#include "luamain.h"
#include "base64.h"

static int lua_MsgBox(lua_State *L)
{
	const char *pMsg = luaL_checkstring(L, 1);
	uint32_t iTimeOut = luaL_optinteger(L, 2, -1);
	g_controller->m_androidCaller.MessageBox(pMsg, iTimeOut);
	return 0;
}

static int lua_CaptureAndSaveWithGIF(lua_State *L)
{
	int iX = luaL_checkinteger(L, 1);
	int iY = luaL_checkinteger(L, 2);
	int iWidth = luaL_checkinteger(L, 3) - iX;
	int iHeight = luaL_checkinteger(L, 4) - iY;
	int iFrameDelay = max(luaL_checkinteger(L, 5), 5);
	int iTotalFrames = max(luaL_checkinteger(L, 6), 1);
	const char *pSaveFileName = luaL_checkstring(L, 7);

	return 0;
//	CScriptMiscCmd *pThis = (CScriptMiscCmd *)lua_touserdata(L, lua_upvalueindex(1));
//	CLuaScriptContainer *pScriptContainer = pThis->m_pScriptContainer;
//
//	CxImage **pimgarray = NULL;
//	CxImage *pimage = NULL;
//	std::string strErrorInfo;
//
//	try
//	{
//		pimgarray = new CxImage *[iTotalFrames];
//		pimage = new CxImage[iTotalFrames];
//	}
//	catch (...)
//	{
//		strErrorInfo = "new object failed";
//		goto failed;
//	}
//
//	for (int i = 0; i < iTotalFrames; i++)
//	{
//		DWORD dwTickTime = GetTickCount();
//		void *pFileBuffer = NULL;
//		uint32_t nFileSize = 0;
//		int iRet = pScriptContainer->m_pScriptCmd->GetBitmapFile(NULL, iX, iY, iWidth, iHeight, pFileBuffer, nFileSize);
//		if (!IS_SUCCESS(iRet))
//		{
//			pScriptContainer->OnCommandFailed(iRet);
//			strErrorInfo = "capture screen failed";
//			goto failed;
//		}
//
//		pimage[i].Decode((PBYTE)pFileBuffer, nFileSize, CXIMAGE_FORMAT_UNKNOWN);
//		pScriptContainer->m_pScriptCmd->ReleaseBitmapFile(pFileBuffer);
//		pimage[i].DecreaseBpp(8, false);
//		pimage[i].SetFrameDelay(iFrameDelay / 10);
//		pimage[i].SetDisposalMethod(0);
//		pimgarray[i] = &pimage[i];
//
//		if (i < iTotalFrames - 1)
//		{
//			DWORD dwElapse = GetTickCount() - dwTickTime;
//			if (dwElapse < (DWORD)iFrameDelay)
//				pScriptContainer->DoDelay(iFrameDelay - dwElapse);
//			if (pScriptContainer->IsUserAbort())
//			{
//				strErrorInfo = "user abort";
//				goto failed;
//			}
//		}
//	}
//
//	{
//		CxIOFile hFile;
//		if (!hFile.Open((CUtf8Convert)pSaveFileName, _T("wb")))
//		{
//			strErrorInfo = "save image failed";
//			goto failed;
//		}
//		else
//		{
//			CxImageGIF multiimage;
//			multiimage.SetLoops(0);
//			multiimage.SetDisposalMethod(2);
//			multiimage.Encode(&hFile, pimgarray, iTotalFrames, false, false);
//			hFile.Close();
//		}
//	}
//
//	delete [] pimgarray;
//	delete [] pimage;
//	lua_pushboolean(L, TRUE);
//	return 1;
//
//	failed:
//	if (pimgarray)
//		delete [] pimgarray;
//	if (pimage)
//		delete [] pimage;
//	lua_pushboolean(L, FALSE);
//	lua_pushstring(L, strErrorInfo.c_str());
//	return 2;
}

static int lua_RunApp(lua_State *L)
{
	const char *pApp = luaL_checkstring(L, 1);
	const char *pParameters = luaL_checkstring(L, 2);
	uint32_t nShowCmd = lua_tointeger(L, 3);
	bool bWaitAppExit = lua_toboolean(L, 4) != 0;

	uint32_t iProcessIdOrExitCode;
	int iRet = g_controller->RunApp(pApp, pParameters, nShowCmd, bWaitAppExit, iProcessIdOrExitCode);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushinteger(L, iProcessIdOrExitCode);
	return 1;
}

static int lua_KillApp(lua_State *L)
{
	uint32_t iProcessId = lua_tointeger(L, 1);
	const char *pProcessName = lua_tostring(L, 2);

	int iRet = g_controller->KillApp(iProcessId, pProcessName);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_GetProcessId(lua_State *L)
{
	const char *pImageName = luaL_checkstring(L, 1);

	std::string strResult;
	int iRet = g_controller->GetProcessId(pImageName, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

static int lua_SendDeviceCmd(lua_State *L)
{
	const char *pCmd = luaL_checkstring(L, 1);
	const char *pArgs = luaL_checkstring(L, 2);

	std::string strResult;
	int iRet = g_controller->SendDeviceCmd(pCmd, pArgs, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

static int lua_GetDeviceInfo(lua_State *L)
{
	const char *pInfoClass = luaL_checkstring(L, 1);

	std::string strResult;
	int iRet = g_controller->GetDeviceInfo(pInfoClass, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

static int lua_GetTickCount(lua_State *L)
{
	lua_pushnumber(L, GetTickCount());
	return 1;
}

static int lua_FromBase64(lua_State *L)
{
	const char *encMsg = luaL_checkstring(L, 1);
	CBase64 base64 = CBase64::Decode(encMsg);
	lua_pushlstring(L, (const char *)base64.GetBuffer(), base64.GetSize());
	return 1;
}

static int lua_ToBase64(lua_State *L)
{
	size_t len;
	const char *msg = luaL_checklstring(L, 1, &len);
	CBase64 base64 = CBase64::Encode(msg, len);
	lua_pushstring(L, (const char *)base64.GetBuffer());
	return 1;
}

int luaopen_misc(lua_State *L)
{
	static const luaL_Reg misclib[] = {
			{"MsgBox", lua_MsgBox},
			{"CaptureAndSaveWithGIF", lua_CaptureAndSaveWithGIF},
			{"RunApp", lua_RunApp},
			{"KillApp", lua_KillApp},
			{"GetProcessId", lua_GetProcessId},
			{"SendDeviceCmd", lua_SendDeviceCmd},
			{"GetDeviceInfo", lua_GetDeviceInfo},
			{"GetTickCount", lua_GetTickCount},
			{"FromBase64", lua_FromBase64},
			{"ToBase64", lua_ToBase64},
			{NULL, NULL}
	};

	luaI_openlib(L, "misc", misclib, 0);
	return 1;
}
