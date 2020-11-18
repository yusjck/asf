#include "common.h"
#include "luamain.h"

static int lua_CaptureBitmap(lua_State *L)
{
	int iX = luaL_checkinteger(L, 1);
	int iY = luaL_checkinteger(L, 2);
	int iWidth = luaL_checkinteger(L, 3) - iX;
	int iHeight = luaL_checkinteger(L, 4) - iY;

	handle_t pMemoryBitmap = NULL;
	int iRet = g_controller->m_color.CaptureBitmap(iX, iY, iWidth, iHeight, pMemoryBitmap);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushlightuserdata(L, (void *)pMemoryBitmap);
	return 1;
}

static int lua_ReleaseBitmap(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_touserdata(L, 1);

	int iRet = g_controller->m_color.ReleaseBitmap(pMemoryBitmap);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

static int lua_GetPreviousCapture(lua_State *L)
{
	handle_t pMemoryBitmap = NULL;
	int iRet = g_controller->m_color.GetPreviousCapture(pMemoryBitmap);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushlightuserdata(L, (void *)pMemoryBitmap);
	return 1;
}

static int lua_LoadBitmapFromFile(lua_State *L)
{
	const char *pImageName = luaL_checkstring(L, 1);
	FileBuffer imgbuf;
	if (!imgbuf.LoadFromFile(pImageName))
		return lua_GotoCmdFailed(L, ERR_OPEN_FILE_FAILED);

	handle_t pMemoryBitmap = NULL;
	int iRet = g_controller->m_color.CreateBitmap(imgbuf.GetBuffer(), imgbuf.GetSize(), pMemoryBitmap);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushlightuserdata(L, (void *)pMemoryBitmap);
	return 1;
}

static int lua_SaveBitmapToFile(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pFileName = luaL_checkstring(L, 6);
	const char *pFileFormat = luaL_checkstring(L, 7);

	FileBuffer imgbuf;
	int iRet = g_controller->m_color.CaptureBitmapToPngFile(pMemoryBitmap, iX, iY, iWidth, iHeight, imgbuf);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	imgbuf.SaveToFile(pFileName);
	lua_pushboolean(L, 1);
	return 1;
}

static int lua_FindPicture(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	handle_t pSourceBitmap = (handle_t)lua_topointer(L, 6);
	const char *pColorMask = lua_tostring(L, 7);
	float fSimilar = (float)luaL_checknumber(L, 8);
	int nMaxResult = luaL_optinteger(L, 9, 1);

	std::string strResultSet;
	int iRet = g_controller->m_color.FindPicture(pMemoryBitmap, iX, iY, iWidth, iHeight, pSourceBitmap, pColorMask, fSimilar, nMaxResult, strResultSet);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResultSet.c_str());
	return 1;
}

static int lua_FindColor(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pColor = luaL_checkstring(L, 6);
	float fSimilar = (float)luaL_checknumber(L, 7);
	int nMaxResult = luaL_optinteger(L, 8, 1);

	std::string strResultSet;
	int iRet = g_controller->m_color.FindColor(pMemoryBitmap, iX, iY, iWidth, iHeight, pColor, fSimilar, nMaxResult, strResultSet);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResultSet.c_str());
	return 1;
}

static int lua_FindColorEx(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pColorGroup = luaL_checkstring(L, 6);
	float fSimilar = (float)luaL_checknumber(L, 7);
	int iType = luaL_checkinteger(L, 8);
	int nMaxResult = luaL_optinteger(L, 9, 1);

	std::string strResultSet;
	int iRet = g_controller->m_color.FindColorEx(pMemoryBitmap, iX, iY, iWidth, iHeight, pColorGroup, fSimilar, iType, nMaxResult, strResultSet);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResultSet.c_str());
	return 1;
}

static int lua_GetPixelColor(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);

	std::string strResult;
	int iRet = g_controller->m_color.GetPixelColor(pMemoryBitmap, iX, iY, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

static int lua_GetColorCount(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pColor = luaL_checkstring(L, 6);
	float fSimilar = (float)luaL_checknumber(L, 7);

	int nColorCount = 0;
	int iRet = g_controller->m_color.GetColorCount(pMemoryBitmap, iX, iY, iWidth, iHeight, pColor, fSimilar, nColorCount);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushinteger(L, nColorCount);
	return 1;
}

static int lua_IsSimilarColor(lua_State *L)
{
	handle_t pMemoryBitmap = (handle_t)lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	const char *pColor = luaL_checkstring(L, 4);
	float fSimilar = (float)luaL_checknumber(L, 5);

	bool bIsSimilar = false;
	int iRet = g_controller->m_color.IsSimilarColor(pMemoryBitmap, iX, iY, pColor, fSimilar, bIsSimilar);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushinteger(L, bIsSimilar);
	return 1;
}

int lua_FindString(lua_State *L)
{
	handle_t pMemoryBitmap = lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pString = luaL_checkstring(L, 6);
	const char *pLogFont = luaL_checkstring(L, 7);
	int nMaxResult = luaL_optinteger(L, 8, 1);

	std::string strResultSet;
	int iRet = g_controller->m_color.m_simpleOcr.FindString(pMemoryBitmap, iX, iY, iWidth, iHeight, pString, pLogFont, nMaxResult, strResultSet);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResultSet.c_str());
	return 1;
}

int lua_FindStringEx(lua_State *L)
{
	handle_t pMemoryBitmap = lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pString = luaL_checkstring(L, 6);
	const char *pLogFont = luaL_checkstring(L, 7);
	const char *pFontColor = luaL_checkstring(L, 8);
	float fSimilar = (float)luaL_checknumber(L, 9);
	int nMaxResult = luaL_optinteger(L, 10, 1);

	std::string strResultSet;
	int iRet = g_controller->m_color.m_simpleOcr.FindStringEx(pMemoryBitmap, iX, iY, iWidth, iHeight, pString, pLogFont, pFontColor, fSimilar, nMaxResult, strResultSet);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResultSet.c_str());
	return 1;
}

int lua_SetOcrDict(lua_State *L)
{
	const char *pDictName = luaL_checkstring(L, 1);
	int iDictType = luaL_checkinteger(L, 2);
	const char *pDict = luaL_checkstring(L, 3);
	const char *pLogFont = luaL_checkstring(L, 4);

	int iRet = g_controller->m_color.m_simpleOcr.SetOcrDict(pDictName, iDictType, pDict);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushboolean(L, 1);
	return 1;
}

int lua_OcrExtract(lua_State *L)
{
	handle_t pMemoryBitmap = lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pFontColor = luaL_checkstring(L, 6);

	std::string strResult;
	int iRet = g_controller->m_color.m_simpleOcr.OcrExtract(pMemoryBitmap, iX, iY, iWidth, iHeight, pFontColor, strResult);
	if (IS_SUCCESS(iRet) || iRet == ERR_AREA_INVALID)
	{
		lua_pushinteger(L, iRet != ERR_AREA_INVALID);
		lua_pushstring(L, strResult.c_str());
		return 2;
	}
	else
	{
		return lua_GotoCmdFailed(L, iRet);
	}
}

int lua_SimpleOcr(lua_State *L)
{
	handle_t pMemoryBitmap = lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pDictName = luaL_checkstring(L, 6);
	const char *pFontColor = luaL_checkstring(L, 7);
	float fSimilar = (float)luaL_checknumber(L, 8);

	std::string strResult;
	int iRet = g_controller->m_color.m_simpleOcr.Ocr(pMemoryBitmap, iX, iY, iWidth, iHeight, pDictName, pFontColor, fSimilar, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

int lua_SimpleOcrEx(lua_State *L)
{
	handle_t pMemoryBitmap = lua_topointer(L, 1);
	int iX = luaL_checkinteger(L, 2);
	int iY = luaL_checkinteger(L, 3);
	int iWidth = luaL_checkinteger(L, 4) - iX;
	int iHeight = luaL_checkinteger(L, 5) - iY;
	const char *pDictName = luaL_checkstring(L, 6);
	const char *pFontColor = luaL_checkstring(L, 7);
	float fSimilar = (float)luaL_checknumber(L, 8);
	bool bCheckSpace = lua_toboolean(L, 9) != 0;

	std::string strResult;
	int iRet = g_controller->m_color.m_simpleOcr.OcrEx(pMemoryBitmap, iX, iY, iWidth, iHeight, pDictName, pFontColor, fSimilar, bCheckSpace, strResult);
	if (!IS_SUCCESS(iRet))
		return lua_GotoCmdFailed(L, iRet);

	lua_pushstring(L, strResult.c_str());
	return 1;
}

int luaopen_color(lua_State *L)
{
	static const luaL_Reg colorlib[] = {
			{"CaptureBitmap", lua_CaptureBitmap},
			{"ReleaseBitmap", lua_ReleaseBitmap},
			{"GetPreviousCapture", lua_GetPreviousCapture},
			{"LoadBitmapFromFile", lua_LoadBitmapFromFile},
			{"SaveBitmapToFile", lua_SaveBitmapToFile},
			{"FindPicture", lua_FindPicture},
			{"FindColor", lua_FindColor},
			{"FindColorEx", lua_FindColorEx},
			{"GetPixelColor", lua_GetPixelColor},
			{"GetColorCount", lua_GetColorCount},
			{"IsSimilarColor", lua_IsSimilarColor},
			{"FindString", lua_FindString},
			{"FindStringEx", lua_FindStringEx},
			{"SetOcrDict", lua_SetOcrDict},
			{"OcrExtract", lua_OcrExtract},
			{"SimpleOcr", lua_SimpleOcr},
			{"SimpleOcrEx", lua_SimpleOcrEx},
			{NULL, NULL}
	};

	luaI_openlib(L, "color", colorlib, 0);
	return 1;
}
