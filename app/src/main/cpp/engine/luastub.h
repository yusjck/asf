#ifndef _LUASTUB_H
#define _LUASTUB_H

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

int lua_docall (lua_State *L, int narg, int clear);

/* 模块初始化 */
void cobj_init(lua_State *L);

/* 把 C 对象指针 obj 压入堆栈，如果不提供释放函数 release ，则以 lightuserdata 形式存放。
    如果提供 release 函数，则会在 gc 环节调用 */
void cobj_push(lua_State *L, void *obj, void (*release)(void *));

/* 将堆栈上的对象转换为 C 指针 */
void *cobj_get(lua_State *L, int idx);

/* 把 obj 对应在 lua 状态机中的对象引用数量加 1 ，返回新的引用次数，出错返回 -1 */
int cobj_addref(lua_State *L, void *obj);

/* 把 obj 对应在 lua 状态机中的对象引用数量减 1 ，返回新的引用次数，出错返回 -1 */
int cobj_release(lua_State *L, void *obj);

int luaopen_cobject(lua_State *L);
int lua_pushcobject(lua_State *L, void *obj, void (*release)(void *));
void *luaL_checkcobject(lua_State *L, int idx);

#endif