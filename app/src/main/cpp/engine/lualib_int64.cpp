#include "luamain.h"
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

static int64_t
_toint64(lua_State *L, int index) {
	int type = lua_type(L,index);
	int64_t n = 0;
	switch(type) {
	case LUA_TNUMBER: {
		lua_Number d = lua_tonumber(L,index);
		n = (int64_t)d;
		break;
	}
	case LUA_TSTRING: {
		size_t len = 0;
		const char * str = (const char *)lua_tolstring(L, index, &len);
		if (len > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
			if (len>18) {
				return luaL_error(L, "The string (length = %d) is not an int64 string", len);
			}
			n = strtoll(str + 2, NULL, 16);
		}
		else {
			n = strtoll(str, NULL, 10);
		}
		break;
	}
	case LUA_TUSERDATA: {
		void * p = lua_touserdata(L,index);
		n = *(int64_t *)p;
		break;
	}
	default:
		return luaL_error(L, "argument %d error type %s", index, lua_typename(L,type));
	}
	return n;
}

static inline void
_pushint64(lua_State *L, int64_t n) {
	int64_t *p = (int64_t *)lua_newuserdata(L,sizeof(int64_t));
	*p = n;
	luaL_getmetatable(L, "int64");
	lua_setmetatable(L,-2);
}

static int
int64_add(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	_pushint64(L, a+b);
	
	return 1;
}

static int
int64_sub(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	_pushint64(L, a-b);
	
	return 1;
}

static int
int64_mul(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	_pushint64(L, a * b);
	
	return 1;
}

static int
int64_div(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	if (b == 0) {
		return luaL_error(L, "div by zero");
	}
	_pushint64(L, a / b);
	
	return 1;
}

static int
int64_mod(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	if (b == 0) {
		return luaL_error(L, "mod by zero");
	}
	_pushint64(L, a % b);
	
	return 1;
}

static int64_t
_pow64(int64_t a, int64_t b) {
	if (b == 1) {
		return a;
	}
	int64_t a2 = a * a;
	if (b % 2 == 1) {
		return _pow64(a2, b/2) * a;
	} else {
		return _pow64(a2, b/2);
	}
}

static int
int64_pow(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	int64_t p;
	if (b > 0) {
		p = _pow64(a,b);
	} else if (b == 0) {
		p = 1;
	} else {
		return luaL_error(L, "pow by nagtive number %d",(int)b);
	} 
	_pushint64(L, p);

	return 1;
}

static int
int64_unm(lua_State *L) {
	int64_t a = _toint64(L,1);
	_pushint64(L, -a);
	return 1;
}

static int
int64_new(lua_State *L) {
	int top = lua_gettop(L);
	int64_t n;
	switch(top) {
		case 0 : 
			_pushint64(L,0);
			break;
		case 1 :
			n = _toint64(L,1);
			_pushint64(L,n);
			break;
		default: {
			int base = luaL_checkinteger(L,2);
			if (base < 2) {
				luaL_error(L, "base must be >= 2");
			}
			const char * str = luaL_checkstring(L, 1);
			n = strtoll(str, NULL, base);
			_pushint64(L,n);
			break;
		}
	}
	return 1;
}

static int
int64_eq(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	lua_pushboolean(L,a == b);
	return 1;
}

static int
int64_lt(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	lua_pushboolean(L,a < b);
	return 1;
}

static int
int64_le(lua_State *L) {
	int64_t a = _toint64(L,1);
	int64_t b = _toint64(L,2);
	lua_pushboolean(L,a <= b);
	return 1;
}

static int
int64_len(lua_State *L) {
	int64_t a = _toint64(L,1);
	lua_pushnumber(L,(lua_Number)a);
	return 1;
}

static int
tostring(lua_State *L) {
	static char hex[] = "0123456789ABCDEF";
	int64_t n = *(int64_t *)lua_touserdata(L,1);
	if (lua_gettop(L) == 1) {
		luaL_Buffer b;
		luaL_buffinit(L, &b);
		luaL_addstring(&b, "0x");
		int i;
		bool strip = true;
		for (i=15;i>=0;i--) {
			int c = (n >> (i*4)) & 0xf;
			if (strip && c ==0) {
				continue;
			}
			strip = false;
			luaL_addchar(&b, hex[c]);
		}
		if (strip) {
			luaL_addchar(&b , '0');
		}
		luaL_pushresult(&b);
	} else {
		int base = luaL_checkinteger(L,2);
		int shift , mask;
		switch(base) {
		case 0: {
			unsigned char buffer[8];
			int i;
			for (i=0;i<8;i++) {
				buffer[i] = (n >> (i*8)) & 0xff;
			}
			lua_pushlstring(L,(const char *)buffer, 8);
			return 1;
			}
		case 10: {
			int64_t dec = (int64_t)n;
			luaL_Buffer b;
			luaL_buffinit(L, &b);
			if (dec<0) {
				luaL_addchar(&b, '-');
				dec = -dec;
			}
			int buffer[32];
			int i;
			for (i=0;i<32;i++) {
				buffer[i] = dec%10;
				dec /= 10;
				if (dec == 0)
					break;
			}
			while (i>=0) {
				luaL_addchar(&b, hex[buffer[i]]);
				--i;
			}
			luaL_pushresult(&b);
			return 1;
		}
		case 2:
			shift = 1;
			mask = 1;
			break;
		case 8:
			shift = 3;
			mask = 7;
			break;
		case 16:
			shift = 4;
			mask = 0xf;
			break;
		default:
			luaL_error(L, "Unsupport base %d",base);
			break;
		}
		int i;
		char buffer[64];
		for (i=0;i<64;i+=shift) {
			buffer[i/shift] = hex[(n>>(64-shift-i)) & mask];
		}
		lua_pushlstring(L, buffer, 64 / shift);
	}
	return 1;
}

static int
make_mt(lua_State *L) {
	luaL_Reg lib[] = {
		{ "__add", int64_add },
		{ "__sub", int64_sub },
		{ "__mul", int64_mul },
		{ "__div", int64_div },
		{ "__mod", int64_mod },
		{ "__unm", int64_unm },
		{ "__pow", int64_pow },
		{ "__eq", int64_eq },
		{ "__lt", int64_lt },
		{ "__le", int64_le },
		{ "__len", int64_len },
		{ "__tostring", tostring },
		{ NULL, NULL },
	};
	
	luaL_newmetatable(L, "int64");
	for (int i = 0; lib[i].name; i++)
	{
		lua_pushcfunction(L, lib[i].func);
		lua_setfield(L, -2, lib[i].name);
	}
	return 1;
}

int
luaopen_int64(lua_State *L) {
	make_mt(L);
	lua_pop(L,1);
	
	luaL_Reg lib[] = {
		{ "new", int64_new },
		{ "tostring", tostring },
		{ NULL, NULL },
	};

	luaL_register(L, "int64", lib);
	return 1;
}
