#include "luastub.h"
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

/* FIXME: Assume size_t is an unsigned lua_Integer */
typedef size_t lua_UInteger;
#define LUA_UINTEGER_MAX SIZE_MAX

/* Define TOBIT to get a bit value */
#define TOBIT(L, n)                    \
  (lua_UInteger)(luaL_checkinteger((L), (n)))

/* Operations

   The macros MONADIC and VARIADIC only deal with bitwise operations.

   LOGICAL_SHIFT truncates its left-hand operand before shifting so
   that any extra bits at the most-significant end are not shifted
   into the result.

   ARITHMETIC_SHIFT does not truncate its left-hand operand, so that
   the sign bits are not removed and right shift work properly.
   */
  
#define MONADIC(name, op)                                       \
  static int bit_ ## name(lua_State *L) {                       \
    lua_pushnumber(L, op TOBIT(L, 1));                          \
    return 1;                                                   \
  }

#define VARIADIC(name, op)                      \
  static int bit_ ## name(lua_State *L) {       \
    int n = lua_gettop(L), i;                   \
    lua_UInteger w = TOBIT(L, 1);               \
    for (i = 2; i <= n; i++)                    \
      w op TOBIT(L, i);                         \
    lua_pushnumber(L, w);                       \
    return 1;                                   \
  }

#define LOGICAL_SHIFT(name, op)                                         \
  static int bit_ ## name(lua_State *L) {                               \
    lua_pushnumber(L, (lua_UInteger)TOBIT(L, 1) op                      \
                          (unsigned)luaL_checknumber(L, 2));            \
    return 1;                                                           \
  }

#define ARITHMETIC_SHIFT(name, op)                                      \
  static int bit_ ## name(lua_State *L) {                               \
    lua_pushinteger(L, (lua_Integer)TOBIT(L, 1) op                      \
                          (unsigned)luaL_checknumber(L, 2));            \
    return 1;                                                           \
  }

MONADIC(bnot,  ~)
VARIADIC(band, &=)
VARIADIC(bor,  |=)
VARIADIC(bxor, ^=)
LOGICAL_SHIFT(lshift,     <<)
LOGICAL_SHIFT(rshift,     >>)
ARITHMETIC_SHIFT(alshift, <<)
ARITHMETIC_SHIFT(arshift, >>)

// Lua: res = bit( position )
static int bit_bit( lua_State* L )
{
  lua_pushnumber( L, ( lua_UInteger )( 1 << luaL_checkinteger( L, 1 ) ) );
  return 1;
}

// Lua: res = isset( value, position )
static int bit_isset( lua_State* L )
{
  lua_UInteger val = ( lua_UInteger )luaL_checkinteger( L, 1 );
  unsigned pos = ( unsigned )luaL_checkinteger( L, 2 );
  
  lua_pushboolean( L, val & ( 1 << pos ) ? 1 : 0 );
  return 1;
}

// Lua: res = isclear( value, position )
static int bit_isclear( lua_State* L )
{
  lua_UInteger val = ( lua_UInteger )luaL_checkinteger( L, 1 );
  unsigned pos = ( unsigned )luaL_checkinteger( L, 2 );
  
  lua_pushboolean( L, val & ( 1 << pos ) ? 0 : 1 );
  return 1;
}

// Lua: res = set( value, pos1, pos2, ... )
static int bit_set( lua_State* L )
{ 
  lua_UInteger val = ( lua_UInteger )luaL_checkinteger( L, 1 );
  unsigned total = lua_gettop( L ), i;
  
  for( i = 2; i <= total; i ++ )
    val |= 1 << ( unsigned )luaL_checkinteger( L, i );
  lua_pushnumber( L, val );
  return 1;
}

// Lua: res = clear( value, pos1, pos2, ... )
static int bit_clear( lua_State* L )
{
  lua_UInteger val = ( lua_UInteger )luaL_checkinteger( L, 1 );
  unsigned total = lua_gettop( L ), i;
  
  for( i = 2; i <= total; i ++ )
    val &= ~( 1 << ( unsigned )luaL_checkinteger( L, i ) );
  lua_pushnumber( L, val );
  return 1; 
}

int luaopen_bit(lua_State *L)
{
	luaL_Reg thislib[] = {
		{"bnot", bit_bnot},
		{"band", bit_band},
		{"bor", bit_bor},
		{"bxor", bit_bxor},
		{"lshift", bit_lshift},
		{"rshift", bit_rshift},
		{"alshift", bit_alshift},
		{"arshift", bit_arshift},
		{"bit", bit_bit},
		{"set", bit_set},
		{"clear", bit_clear},
		{"isset", bit_isset},
		{"isclear", bit_isclear},
		{NULL, NULL}
	};

	luaL_register(L, "bit", thislib);
	return 1;
}
