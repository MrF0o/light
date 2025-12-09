#include "../buf/RopeBuffer.hpp"
#include <string.h>

extern "C" {
#include "api.h"
}

#define API_TYPE_BUFFER "Buffer"

using namespace buffer;

static RopeBuffer* checkbuffer(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, API_TYPE_BUFFER);
    luaL_argcheck(L, ud != nullptr, idx, "`Buffer` expected");
    return *(RopeBuffer**)ud;
}

// buffer.new() -> buffer
static int l_buffer_new(lua_State* L) {
    RopeBuffer** ud = (RopeBuffer**)lua_newuserdata(L, sizeof(RopeBuffer*));
    *ud = new RopeBuffer();
    luaL_setmetatable(L, API_TYPE_BUFFER);
    return 1;
}

// buffer:set_text(text: string)
static int l_buffer_set_text(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    size_t len;
    const char* text = luaL_checklstring(L, 2, &len);
    buf->setText(text, len);
    return 0;
}

// buffer:insert(line: number, col: number, text: string)
static int l_buffer_insert(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_Integer line = luaL_checkinteger(L, 2);
    lua_Integer col = luaL_checkinteger(L, 3);
    size_t len;
    const char* text = luaL_checklstring(L, 4, &len);
    
    buf->insert((size_t)line, (size_t)col, text, len);
    return 0;
}

// buffer:remove(line1: number, col1: number, line2: number, col2: number)
static int l_buffer_remove(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_Integer line1 = luaL_checkinteger(L, 2);
    lua_Integer col1 = luaL_checkinteger(L, 3);
    lua_Integer line2 = luaL_checkinteger(L, 4);
    lua_Integer col2 = luaL_checkinteger(L, 5);
    
    buf->remove((size_t)line1, (size_t)col1, (size_t)line2, (size_t)col2);
    return 0;
}

// buffer:get_line(line: number) -> string
static int l_buffer_get_line(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_Integer line = luaL_checkinteger(L, 2);
    
    size_t length;
    const char* text = buf->getLine((size_t)line, &length);
    lua_pushlstring(L, text, length);
    return 1;
}

// buffer:get_text(line1: number, col1: number, line2: number, col2: number) -> string
static int l_buffer_get_text(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_Integer line1 = luaL_checkinteger(L, 2);
    lua_Integer col1 = luaL_checkinteger(L, 3);
    lua_Integer line2 = luaL_checkinteger(L, 4);
    lua_Integer col2 = luaL_checkinteger(L, 5);
    
    size_t length;
    char* text = buf->getText((size_t)line1, (size_t)col1, (size_t)line2, (size_t)col2, &length);
    lua_pushlstring(L, text, length);
    free(text);
    return 1;
}

// buffer:line_count() -> number
static int l_buffer_line_count(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_pushinteger(L, (lua_Integer)buf->getLineCount());
    return 1;
}

// buffer:byte_size() -> number
static int l_buffer_byte_size(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_pushinteger(L, (lua_Integer)buf->getByteSize());
    return 1;
}

// buffer:clear()
static int l_buffer_clear(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    buf->clear();
    return 0;
}

static int l_buffer_gc(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    delete buf;
    return 0;
}

// tostring metamethod
static int l_buffer_tostring(lua_State* L) {
    RopeBuffer* buf = checkbuffer(L, 1);
    lua_pushfstring(L, "Buffer (%d lines, %d bytes)", 
                    (int)buf->getLineCount(), 
                    (int)buf->getByteSize());
    return 1;
}

static const luaL_Reg buffer_methods[] = {
    {"set_text",     l_buffer_set_text},
    {"insert",      l_buffer_insert},
    {"remove",      l_buffer_remove},
    {"get_line",     l_buffer_get_line},
    {"get_text",     l_buffer_get_text},
    {"line_count",l_buffer_line_count},
    {"byte_size", l_buffer_byte_size},
    {"clear",       l_buffer_clear},
    {nullptr,       nullptr}
};

static const luaL_Reg buffer_meta[] = {
    {"__gc",        l_buffer_gc},
    {"__tostring",  l_buffer_tostring},
    {nullptr,       nullptr}
};

static const luaL_Reg buffer_lib[] = {
    {"new",         l_buffer_new},
    {nullptr,       nullptr}
};

extern "C" {
int luaopen_buffer(lua_State* L) {
    luaL_newmetatable(L, API_TYPE_BUFFER);
    luaL_setfuncs(L, buffer_meta, 0);
    luaL_newlib(L, buffer_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    
    luaL_newlib(L, buffer_lib);
    return 1;
}
}
