#ifndef LUA_COMPAT_H
#define LUA_COMPAT_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __has_include
# if __has_include(<luajit.h>)
#include <luajit.h>
#endif
#endif

// Basic LuaJIT compatibility shims for Lua 5.3/5.4 APIs
#ifdef LUAJIT_VERSION

// lua_absindex if not available
#ifndef lua_absindex
static inline int lua_absindex(lua_State *L, int idx) {
  return (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : lua_gettop(L) + idx + 1;
}
#endif

// lua_isinteger compatibility
#ifndef lua_isinteger
static inline int lua_isinteger(lua_State *L, int idx) {
  if (lua_type(L, idx) == LUA_TNUMBER) {
    lua_Number n = lua_tonumber(L, idx);
    lua_Integer i = (lua_Integer)n;
    return (n == (lua_Number)i);
  }
  return 0;
}
#endif

// lua_geti/lua_seti compatibility since LuaJIT has these in compatibility mode
#ifndef lua_geti
static inline int lua_geti(lua_State *L, int idx, lua_Integer n) {
  idx = lua_absindex(L, idx);
  lua_pushinteger(L, n);
  lua_gettable(L, idx);
  return lua_type(L, -1);
}
#endif

#ifndef lua_seti
static inline void lua_seti(lua_State *L, int idx, lua_Integer n) {
  idx = lua_absindex(L, idx);
  lua_pushinteger(L, n);
  lua_insert(L, -2);
  lua_settable(L, idx);
}
#endif

// lua_getfield returns void in LuaJIT, wrap it to return type
static inline int lua_getfield_compat(lua_State *L, int idx, const char *k) {
  lua_getfield(L, idx, k);
  return lua_type(L, -1);
}
#define lua_getfield lua_getfield_compat

// lua_rawgeti returns void in LuaJIT, wrap it to return type  
static inline int lua_rawgeti_compat(lua_State *L, int idx, int n) {
  lua_rawgeti(L, idx, n);
  return lua_type(L, -1);
}
#define lua_rawgeti lua_rawgeti_compat

// luaL_tolstring compatibility for LuaJIT
#ifndef luaL_tolstring
static inline const char *luaL_tolstring(lua_State *L, int idx, size_t *len) {
  if (luaL_callmeta(L, idx, "__tostring")) {
    if (!lua_isstring(L, -1))
      luaL_error(L, "'__tostring' must return a string");
  } else {
    switch (lua_type(L, idx)) {
      case LUA_TNUMBER:
      case LUA_TSTRING:
        lua_pushvalue(L, idx);
        break;
      case LUA_TBOOLEAN:
        lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
        break;
      case LUA_TNIL:
        lua_pushliteral(L, "nil");
        break;
      default:
        lua_pushfstring(L, "%s: %p", luaL_typename(L, idx), lua_topointer(L, idx));
        break;
    }
  }
  return lua_tolstring(L, -1, len);
}
#endif

// luaL_len compatibility - implement using lua_objlen for LuaJIT
static inline lua_Integer luaL_len(lua_State *L, int idx) {
  return (lua_Integer)lua_objlen(L, idx);
}

// luaL_requiref compatibility for LuaJIT
static inline void luaL_requiref(lua_State *L, const char *modname,
                                  lua_CFunction openf, int glb) {
  lua_pushcfunction(L, openf);
  lua_pushstring(L, modname);
  lua_call(L, 1, 1);
  lua_getglobal(L, "package");
  if (lua_istable(L, -1)) {
    lua_getfield_compat(L, -1, "loaded");
    if (lua_istable(L, -1)) {
      lua_pushvalue(L, -3);
      lua_setfield(L, -2, modname);
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  if (glb) {
    lua_pushvalue(L, -1);
    lua_setglobal(L, modname);
  }
}

// lua_rawlen compatibility
#ifndef lua_rawlen
#define lua_rawlen lua_objlen
#endif

// luaL_typeerror compatibility
#ifndef luaL_typeerror
static inline int luaL_typeerror(lua_State *L, int arg, const char *tname) {
  const char *msg;
  const char *typearg;
  if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING)
    typearg = lua_tostring(L, -1);
  else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata";
  else
    typearg = luaL_typename(L, arg);
  msg = lua_pushfstring(L, "%s expected, got %s", tname, typearg);
  return luaL_argerror(L, arg, msg);
}
#endif

#endif // LUAJIT_VERSION

#endif // LUA_COMPAT_H
