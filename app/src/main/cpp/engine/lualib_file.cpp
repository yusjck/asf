// Module for interfacing with file system

#include "luamain.h"
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <zconf.h>

#define FILE_READ_CHUNK 1024

typedef struct _file_fd_ud {
  FILE *fd;
} file_fd_ud;

// Lua: close()
static int file_close( lua_State* L )
{
  file_fd_ud *ud;

  if (lua_type( L, 1 ) != LUA_TUSERDATA) {
    // fall back to last opened file
    lua_getfield(L, LUA_ENVIRONINDEX, "file_fd");
    if (!lua_isnil(L, -1)) {
      // top of stack is now default file descriptor
      ud = (file_fd_ud *)luaL_checkudata(L, -1, "file.obj");
      lua_pop( L, 1 );
    } else {
      lua_pop( L, 1 );
      // no default file currently opened
      return 0;
    }
  } else {
    ud = (file_fd_ud *)luaL_checkudata(L, 1, "file.obj");
  }

  // unref default file descriptor
  lua_pushnil(L);
  lua_setfield(L, LUA_ENVIRONINDEX, "file_fd");

  if(ud->fd){
      fclose(ud->fd);
      // mark as closed
      ud->fd = 0;
  }
  return 0;  
}

static int file_obj_free( lua_State *L )
{
  file_fd_ud *ud = (file_fd_ud *)luaL_checkudata(L, 1, "file.obj");
  if (ud->fd) {
    // close file if it's still open
    fclose(ud->fd);
    ud->fd = 0;
  }

  return 0;
}

// Lua: open(filename, mode)
static int file_open( lua_State* L )
{
  lua_pushnil(L);
  lua_setfield(L, LUA_ENVIRONINDEX, "file_fd");

  const char *fname = luaL_checkstring( L, 1 );
  const char *mode = luaL_optstring(L, 2, "r");

  FILE *file_fd = fopen(fname, mode);

  if(!file_fd){
    lua_pushnil(L);
  } else {
    file_fd_ud *ud = (file_fd_ud *) lua_newuserdata( L, sizeof( file_fd_ud ) );
    ud->fd = file_fd;
    luaL_getmetatable( L, "file.obj" );
    lua_setmetatable( L, -2 );

    // store reference to opened file
    lua_pushvalue( L, -1 );
    lua_setfield(L, LUA_ENVIRONINDEX, "file_fd");
  }
  return 1; 
}

// Lua: list()
static int file_list( lua_State* L )
{
  const char *path = luaL_checkstring( L, 1 );  /* Path (arg) at 1 */
  DIR  *dir;
  const char *pattern;
  int pcres;

  lua_settop(L, 1);
  pattern = luaL_optstring(L, 2, NULL);   /* Pattern (arg) or nil (not) at 2 */

  dir = opendir(path);
  if (dir == NULL) {
    return 0;
  }

  lua_newtable( L );                      /* Table at 3 */

  if (pattern) {
    /*
     * We know that pattern is a string, and so the "match" method will always
     * exist.  No need to check return value here
     */
    luaL_getmetafield( L, 1, "match" );  /* Function at 4 */
  }

  struct dirent *dir_entry;
  while ((dir_entry = readdir(dir)) != NULL) {
    if (pattern) {
      lua_settop( L, 4 );                 /* Ensure nothing else on stack */

      /* Construct and pcall(string.match,name,pattern) */
      lua_pushvalue( L, 4 );
      lua_pushstring( L, dir_entry->d_name );
      lua_pushvalue( L, 2 );
      pcres = lua_pcall( L, 2, 1, 0 );
      if (pcres != 0) {
        closedir(dir);
        lua_error( L );
      }
      if (lua_isnil( L, -1 )) {
        continue;
      }
    }

    char fpath[200];
    sprintf(fpath, "%s/%s", path, dir_entry->d_name);
    struct stat st;
    stat(fpath, &st);
    lua_pushinteger( L, st.st_size );
    lua_setfield( L, 3, dir_entry->d_name );
  }

  /* Shed everything back to Table */
  lua_settop( L, 3 );
  closedir(dir);
  return 1;
}

static FILE *get_file_obj( lua_State *L, int *argpos )
{
  if (lua_type( L, 1 ) == LUA_TUSERDATA) {
    file_fd_ud *ud = (file_fd_ud *)luaL_checkudata(L, 1, "file.obj");
    *argpos = 2;
    return ud->fd;
  } else {
    *argpos = 1;
    lua_getfield(L, LUA_ENVIRONINDEX, "file_fd");
    if (lua_isnil(L, -1)) {
      lua_pop( L, 1 );
      return 0;
    }
    // top of stack is now default file descriptor
    file_fd_ud *ud = (file_fd_ud *)luaL_checkudata(L, -1, "file.obj");
    lua_pop( L, 1 );
    return ud->fd;
  }
}

#define GET_FILE_OBJ int argpos; \
  FILE *fd = get_file_obj( L, &argpos );

static int file_seek (lua_State *L)
{
  GET_FILE_OBJ;

  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  if(!fd)
    return luaL_error(L, "open a file first");
  int op = luaL_checkoption(L, argpos, "cur", modenames);
  long offset = luaL_optlong(L, ++argpos, 0);
  op = fseek(fd, offset, mode[op]);
  if (op < 0)
    lua_pushnil(L);  /* error */
  else
    lua_pushinteger(L, ftell(fd));
  return 1;
}

// Lua: exists(filename)
static int file_exists( lua_State* L )
{
  const char *fname = luaL_checkstring( L, 1 );
  lua_pushboolean(L, access(fname, 0) == 0);
  return 1;
}

// Lua: remove(filename)
static int file_remove( lua_State* L )
{
  const char *fname = luaL_checkstring( L, 1 );
  remove(fname);
  return 0;
}

// Lua: flush()
static int file_flush( lua_State* L )
{
  GET_FILE_OBJ;

  if(!fd)
    return luaL_error(L, "open a file first");
  if(fflush(fd) == 0)
    lua_pushboolean(L, 1);
  else
    lua_pushnil(L);
  return 1;
}

// Lua: rename("oldname", "newname")
static int file_rename( lua_State* L )
{
  const char *oldname = luaL_checkstring( L, 1 );
  const char *newname = luaL_checkstring( L, 2 );
  if(0 <= rename( oldname, newname )){
    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

// Lua: stat(filename)
static int file_stat( lua_State* L )
{
  const char *fname = luaL_checkstring( L, 1 );

  struct stat st;
  if (stat( (char *)fname, &st ) < 0) {
    lua_pushnil( L );
    return 1;
  }

  lua_createtable( L, 0, 2 );

  lua_pushinteger( L, (long)st.st_size );
  lua_setfield( L, -2, "size" );

  lua_pushboolean(L, ((st.st_mode & S_IFMT) == S_IFDIR));
  lua_setfield(L, -2, "isdir");

  // time stamp as sub-table
  lua_createtable( L, 0, 6 );

  struct tm *tm = localtime(&st.st_mtim.tv_sec);
  lua_pushinteger( L, tm->tm_year );
  lua_setfield( L, -2, "year" );

  lua_pushinteger( L, tm->tm_mon );
  lua_setfield( L, -2, "mon" );

  lua_pushinteger( L, tm->tm_yday );
  lua_setfield( L, -2, "day" );

  lua_pushinteger( L, tm->tm_hour );
  lua_setfield( L, -2, "hour" );

  lua_pushinteger( L, tm->tm_min );
  lua_setfield( L, -2, "min" );

  lua_pushinteger( L, tm->tm_sec );
  lua_setfield( L, -2, "sec" );

  lua_setfield( L, -2, "time" );

  return 1;
}

// g_read()
static int file_g_read( lua_State* L, int n, int16_t end_char, FILE *fd )
{
  if(n <= 0) {
    long cur = ftell(fd);
    fseek(fd, 0, SEEK_END);
    n = ftell(fd) - cur;
    fseek(fd, cur, SEEK_SET);
  }

  if(end_char < 0 || end_char >255)
    end_char = EOF;

  if(!fd)
    return luaL_error(L, "open a file first");

  char *p;
  int i;

  p = (char *)malloc(n);
  if (!p)
    return luaL_error(L, "not enough memory");
  n = fread(p, 1, n, fd);
  // bypass search if no end character provided
  if (n > 0 && end_char != EOF) {
    for (i = 0; i < n; ++i)
      if (p[i] == end_char)
      {
        ++i;
        break;
      }
  } else {
    i = n;
  }

  if (i == 0) {
    free(p);
    lua_pushnil(L);
    return 1;
  }

  fseek(fd, -(n - i), SEEK_CUR);
  lua_pushlstring(L, p, i);
  free(p);
  return 1;
}

// Lua: read()
// file.read() will read all byte in file
// file.read(10) will read 10 byte from file, or EOF is reached.
// file.read('q') will read until 'q' or EOF is reached. 
static int file_read( lua_State* L )
{
  unsigned need_len = 0;
  int16_t end_char = EOF;
  size_t el;

  GET_FILE_OBJ;

  if( lua_type( L, argpos ) == LUA_TNUMBER )
  {
    need_len = ( unsigned )luaL_checkinteger( L, argpos );
  }
  else if(lua_isstring(L, argpos))
  {
    const char *end = luaL_checklstring( L, argpos, &el );
    if(el!=1){
      return luaL_error( L, "wrong arg range" );
    }
    end_char = (int16_t)end[0];
  }

  return file_g_read(L, need_len, end_char, fd);
}

// Lua: readline()
static int file_readline( lua_State* L )
{
  GET_FILE_OBJ;

  return file_g_read(L, FILE_READ_CHUNK, '\n', fd);
}

// Lua: write("string")
static int file_write( lua_State* L )
{
  GET_FILE_OBJ;

  if(!fd)
    return luaL_error(L, "open a file first");
  size_t l, rl;
  const char *s = luaL_checklstring(L, argpos, &l);
  rl = fwrite(s, 1, l, fd);
  if(rl==l)
    lua_pushboolean(L, 1);
  else
    lua_pushnil(L);
  return 1;
}

// Lua: writeline("string")
static int file_writeline( lua_State* L )
{
  GET_FILE_OBJ;

  if(!fd)
    return luaL_error(L, "open a file first");
  size_t l, rl;
  const char *s = luaL_checklstring(L, argpos, &l);
  rl = fwrite(s, 1, l, fd);
  if(rl==l){
    rl = fwrite("\n", 1, 1, fd);
    if(rl==1)
      lua_pushboolean(L, 1);
    else
      lua_pushnil(L);
  }
  else{
    lua_pushnil(L);
  }
  return 1;
}

static void createmeta(lua_State *L)
{
	luaL_Reg flib[] = {
		{ "close", file_close },
		{ "read", file_read },
		{ "readline", file_readline },
		{ "write", file_write },
		{ "writeline", file_writeline },
		{ "seek", file_seek },
		{ "flush", file_flush },
		{ "__gc", file_obj_free },
		{ NULL, NULL },
	};
	
	luaL_newmetatable(L, "file.obj");  /* create metatable for file handles */
	lua_pushvalue(L, -1);  /* push metatable */
	lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
	luaL_register(L, NULL, flib);  /* file methods */
}

int luaopen_file(lua_State *L)
{
	createmeta(L);
	
	luaL_Reg lib[] = {
		{ "list", file_list },
		{ "open", file_open },
		{ "close", file_close },
		{ "write", file_write },
		{ "writeline", file_writeline },
		{ "read", file_read },
		{ "readline", file_readline },
		{ "remove", file_remove },
		{ "seek", file_seek },
		{ "flush", file_flush },
		{ "rename", file_rename },
		{ "exists", file_exists },
        { "stat", file_stat },
		{ NULL, NULL },
	};

	luaL_register(L, "file", lib);
	return 1;
}
