#include <stdlib.h>
#include <string.h>
#include "luastub.h"

static int traceback (lua_State *L) {
	if (!lua_isstring(L, 1))  /* 'message' not a string? */
		return 1;  /* keep it intact */
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  /* pass error message */
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */
	return 1;
}

int lua_docall (lua_State *L, int narg, int clear) {
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, traceback);  /* push traceback function */
	lua_insert(L, base);  /* put it under chunk and args */
	status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
	lua_remove(L, base);  /* remove traceback function */
	/* force a complete garbage collection in case of errors */
	if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
	return status;
}

static void *dummy=0;

struct gc_object {
	void *obj;
	void (*release)(void *);
};

static int release_object(lua_State *L)
{
	struct gc_object *gco=(struct gc_object*)lua_touserdata(L,1);
	gco->release(gco->obj);
	return 0;
}

/* 模块初始化 */
void cobj_init(lua_State *L)
{
	lua_pushlightuserdata(L,&dummy);
	lua_newtable(L);
	lua_createtable(L,0,1);
	lua_pushstring(L,"v");
	lua_setfield(L,-2,"__mode");
	lua_setmetatable(L,-2);

	lua_createtable(L,0,1);
	lua_pushcfunction(L,release_object);
	lua_setfield(L,-2,"__gc");

	lua_pushlightuserdata(L,(void *)release_object);
	lua_pushvalue(L,-2);
	lua_rawset(L,-4);

	lua_pushlightuserdata(L,(void *)release_object);
	lua_rawset(L,-3);

	lua_settable(L,LUA_REGISTRYINDEX);
}

static void get_reftable(lua_State *L)
{
	lua_pushlightuserdata(L,&dummy);
	lua_gettable(L,LUA_REGISTRYINDEX);
}

/* 把 C 对象指针 obj 压入堆栈，如果不提供释放函数 release ，则以 lightuserdata 形式存放。
    如果提供 release 函数，则会在 gc 环节调用 */
void cobj_push(lua_State *L, void *obj, void (*release)(void *))
{
	if (release==0) {
		lua_pushlightuserdata(L,obj);
	}
	else {
		get_reftable(L);
		lua_pushlightuserdata(L,obj);
		lua_rawget(L,-2);

		if (lua_isnil(L,-1)) {
			struct gc_object *gco;
			lua_pop(L,1);
			gco=(struct gc_object *)lua_newuserdata(L,sizeof(*gco));
			gco->obj=obj;
			gco->release=release;

			lua_pushlightuserdata(L,(void *)release_object);
			lua_rawget(L,-3);
			lua_setmetatable(L,-2);

			lua_pushlightuserdata(L,obj);
			lua_pushvalue(L,-2);
			lua_rawset(L,-4);
		}

		lua_replace(L,-2);
	}
}

/* 将堆栈上的对象转换为 C 指针 */
void *cobj_get(lua_State *L, int idx)
{
	if (lua_islightuserdata(L,idx)) {
		return lua_touserdata(L,idx);
	}
	else {
		struct gc_object *gco=(struct gc_object *)lua_touserdata(L,idx);
		return gco->obj;
	}
}

/* 把 obj 对应在 lua 状态机中的对象引用数量加 1 ，返回新的引用次数，出错返回 -1 */
int cobj_addref(lua_State *L, void *obj)
{
	int r=-1;
	get_reftable(L);
	lua_pushlightuserdata(L,obj);
	lua_rawget(L,-2);
	if (!lua_isnil(L,-1)) {
		lua_pushvalue(L,-1);
		lua_rawget(L,-3);
		r=lua_tointeger(L,-1) + 1;
		lua_pop(L,1);
		lua_pushinteger(L,r);
		lua_rawset(L,-3);
	}
	lua_pop(L,1);
	return r;
}

/* 把 obj 对应在 lua 状态机中的对象引用数量减 1 ，返回新的引用次数，出错返回 -1 */
int cobj_release(lua_State *L, void *obj)
{
	int r=-1;
	get_reftable(L);
	lua_pushlightuserdata(L,obj);
	lua_rawget(L,-2);
	if (!lua_isnil(L,-1)) {
		lua_pushvalue(L,-1);
		lua_rawget(L,-3);
		r=lua_tointeger(L,-1) - 1;
		lua_pop(L,1);
		if (r<=0) {
			lua_pushnil(L);
		}
		else {
			lua_pushinteger(L,r);
		}
		lua_rawset(L,-3);
	}
	lua_pop(L,1);
	return r;
}

int luaopen_cobject(lua_State *L)
{
	luaL_newmetatable(L, "cobject");
	lua_pushcfunction(L, release_object);
	lua_setfield(L, -2, "__gc");
	return 1;
}

int lua_pushcobject(lua_State *L, void *obj, void (*release)(void *))
{
	struct gc_object *gco;
	gco = (struct gc_object *)lua_newuserdata(L, sizeof(*gco));
	gco->obj = obj;
	gco->release = release;
	luaL_getmetatable(L, "cobject");
	lua_setmetatable(L, -2);
	return 1;
}

void *luaL_checkcobject(lua_State *L, int idx)
{
	struct gc_object *gco = (struct gc_object *)luaL_checkudata(L, idx, "cobject");
	return gco->obj;
}
