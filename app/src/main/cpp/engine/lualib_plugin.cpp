#include "common.h"
#include "luamain.h"
#include "pluginproxy.h"

typedef struct _C_OBJECT {
	int ObjectType;
	union {
		struct {
			void *DataPtr;
			uint32_t DataLen;
		} Type1;
		struct {
			char ObjectFunName[100];
			void *ObjectPointer;
			lua_State *L;
			PluginProxy *proxy;
		} Type2;
	};
} C_OBJECT, *PC_OBJECT;

static void CObjectRelease(void *obj)
{
	C_OBJECT *pGCObject = (C_OBJECT *)obj;
	switch (pGCObject->ObjectType)
	{
		case 1:
			delete [] (uint8_t *)pGCObject->Type1.DataPtr;
			break;
		case 2:
			pGCObject->Type2.proxy->DestroyPluginObject(pGCObject->Type2.ObjectFunName, pGCObject->Type2.ObjectPointer);
			cobj_release(pGCObject->Type2.L, pGCObject->Type2.proxy);
			break;
		default:
			break;
	}
	delete pGCObject;
}

static int lua_PluginInvoke(lua_State *L)
{
	const char *pCommandName = lua_tostring(L, lua_upvalueindex(1));
	int nParamCount = lua_gettop(L);
	PluginProxy *proxy = (PluginProxy *)lua_touserdata(L, lua_upvalueindex(2));

	try
	{
		PluginDataWriter dw;
		for (int i = 0; i < nParamCount; i++)
		{
			int type = lua_type(L, i + 1);
			switch (type)
			{
				case LUA_TNIL:
					dw.PushNil();
					break;
				case LUA_TNUMBER:
					dw.PushNumber(lua_tonumber(L, i + 1));
					break;
				case LUA_TBOOLEAN:
					dw.PushBoolean(lua_toboolean(L, i + 1));
					break;
				case LUA_TSTRING:
				{
					size_t l;
					const char *s = lua_tolstring(L, i + 1, &l);
					dw.PushString(s, l);
					break;
				}
				case LUA_TLIGHTUSERDATA:
					dw.PushPointer(lua_touserdata(L, i + 1));
					break;
				case LUA_TUSERDATA:
				{
					C_OBJECT *obj = (C_OBJECT *)luaL_checkcobject(L, i + 1);
					switch (obj->ObjectType)
					{
						case 1:
							dw.PushBinary(obj->Type1.DataPtr, obj->Type1.DataLen);
							break;
						case 2:
							dw.PushObject(obj->Type2.ObjectPointer);
							break;
						default:
							dw.PushNil();
							break;
					}
					break;
				}
				default:
					lua_pushstring(L, "unknown argument type");
					lua_error(L);
					break;
			}
		}

		PluginDataReader dr;
		CBuffer retbuf;
		int iRet = proxy->InvokePluginCommand(pCommandName, dw.GetBuf().GetBuffer(), dw.GetBuf().GetSize(), retbuf);
		if (!IS_SUCCESS(iRet) || !dr.ParseDataBuf(retbuf))
		{
			lua_GotoCmdFailed(L, iRet);
			return 0;
		}

		int nReturnCount = dr.GetValueCount();
		for (int i = 0; i < nReturnCount; i++)
		{
			int type = dr.GetValueType(i);
			switch (type)
			{
				case IVT_NIL:
					lua_pushnil(L);
					break;
				case IVT_BOOLEAN:
					lua_pushboolean(L, dr.GetBoolean(i));
					break;
				case IVT_NUMBER:
					lua_pushnumber(L, dr.GetNumber(i));
					break;
				case IVT_STRING:
				{
					uint32_t l;
					const char *s = dr.GetString(i, l);
					lua_pushlstring(L, s, l);
					break;
				}
				case IVT_POINTER:
					lua_pushlightuserdata(L, (void *)dr.GetPointer(i));
					break;
				case IVT_BINARY:
				{
					uint32_t nDataSize;
					const void *pData = dr.GetBinary(i, nDataSize);
					void *p = NULL;
					try
					{
						p = new uint8_t[nDataSize];
					}
					catch (...)
					{
					}
					if (p == NULL)
					{
						lua_pushstring(L, "not enough memory");
						lua_error(L);
					}
					else
					{
						memcpy(p, pData, nDataSize);
						C_OBJECT *obj = new C_OBJECT;
						obj->ObjectType = 1;
						obj->Type1.DataPtr = p;
						obj->Type1.DataLen = nDataSize;
						lua_pushcobject(L, obj, CObjectRelease);
					}
					break;
				}
				case IVT_OBJECT:
				{
					void *pObj = dr.GetObject(i);
					C_OBJECT *obj = new C_OBJECT;
					obj->ObjectType = 2;
					obj->Type2.L = L;
					obj->Type2.proxy = proxy;
					cobj_addref(L, proxy);
					strcpy(obj->Type2.ObjectFunName, pCommandName);
					obj->Type2.ObjectPointer = pObj;
					lua_pushcobject(L, obj, CObjectRelease);
					break;
				}
				default:
					lua_pushstring(L, "unknown return value type");
					lua_error(L);
					break;
			}
		}
		return nReturnCount;
	}
	catch (std::string &s)
	{
		lua_pushstring(L, s.c_str());
		lua_error(L);
	}
	catch (...)
	{
		lua_pushstring(L, "invoke plugin exception");
		lua_error(L);
	}
	return 0;
}

static void RegisterPluginFunction(lua_State *L)
{
	cobj_init(L);
	PluginProxy *proxy = new PluginProxy;
	proxy->LoadPlugins(g_strPluginDir.c_str(), 0, false);
	cobj_push(L, proxy, [](void *obj) {
		delete (PluginProxy *)obj;
	});
	cobj_addref(L, proxy);

	std::vector<std::string> CmdList;
	proxy->GetCommandList(CmdList);

	for (size_t i = 0; i < CmdList.size(); i++)
	{
		std::vector<std::string> v = split(CmdList[i], ".");
		if (v.size() != 2)
			continue;

		const char *pLibName = v[0].c_str();
		const char *pFunName = v[1].c_str();
		LOGI("add plugin command: %s.%s\n", pLibName, pFunName);

		lua_getglobal(L, "plugin");	// 取出plugin表，如果不存在就创建一份
		if (!lua_istable(L, -1))
		{
			lua_pop(L, 1);			// 移除nil
			lua_createtable(L, 0, 1);
			lua_pushvalue(L, -1);	// 将表在堆栈上的值拷贝一份
			lua_setglobal(L, "plugin");
		}

		lua_getfield(L, -1, pLibName);	// 取出函数表到堆栈上
		if (!lua_istable(L, -1))	// 判定函数表是否存在，不存在创建一张
		{
			lua_pop(L, 1);			// 移除nil
			lua_createtable(L, 0, 1);
			lua_pushvalue(L, -1);	// 将表在堆栈上的值拷贝一份
			lua_setfield(L, -3, pLibName);
		}
		lua_remove(L, -2);			// 移除plugin表

		lua_pushstring(L, CmdList[i].c_str());
		lua_pushlightuserdata(L, proxy);
		lua_pushcclosure(L, lua_PluginInvoke, 2);
		lua_setfield(L, -2, pFunName);	// 将新的函数添加到函数表中
		lua_pop(L, 1);	// 移除函数表
	}
}

int luaopen_plugin(lua_State *L)
{
//	static const luaL_Reg pluginlib[] = {
//			{NULL, NULL}
//	};
//
//	luaI_openlib(L, "plugin", pluginlib, 0);

	RegisterPluginFunction(L);
	return 0;
}
