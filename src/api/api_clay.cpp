#include <cstring>
#include <iostream>

extern "C" {
#include "lua_compat.h"
#include "clay.h"
#include "api.h"
#include "renwindow.h"
#include "rencache.h"
}

#include "clay_renderer.hpp"
#include "clay_rencache.hpp"

static Clay_Arena clay_arena;
static Clay_Context* clay_context = nullptr;

namespace clay {
    ClayRenderer* g_clay_renderer = nullptr;
}

static Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    float charWidth = config->fontSize * 0.6f;
    return {
        .width = text.length * charWidth,
        .height = static_cast<float>(config->fontSize)
    };
}

static void ClayErrorHandler(Clay_ErrorData errorData) {
    fprintf(stderr, "Clay Error: %.*s\n", (int)errorData.errorText.length, errorData.errorText.chars);
}

static int l_clay_initialize(lua_State* L) {
    int width = luaL_checkinteger(L, 1);
    int height = luaL_checkinteger(L, 2);
    
    if (!clay_arena.memory) {
        uint32_t memorySize = Clay_MinMemorySize();
        clay_arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, malloc(memorySize));
        
        Clay_Dimensions dimensions = {static_cast<float>(width), static_cast<float>(height)};
        Clay_ErrorHandler errorHandler = {ClayErrorHandler, nullptr};
        
        clay_context = Clay_Initialize(clay_arena, dimensions, errorHandler);
        Clay_SetMeasureTextFunction(MeasureText, nullptr);
    }
    
    return 0;
}

static int l_clay_set_dimensions(lua_State* L) {
    float width = static_cast<float>(luaL_checknumber(L, 1));
    float height = static_cast<float>(luaL_checknumber(L, 2));
    
    Clay_SetLayoutDimensions({width, height});
    return 0;
}

static int l_clay_set_pointer(lua_State* L) {
    float x = static_cast<float>(luaL_checknumber(L, 1));
    float y = static_cast<float>(luaL_checknumber(L, 2));
    bool down = lua_toboolean(L, 3);
    
    Clay_SetPointerState({x, y}, down);
    return 0;
}

static int l_clay_begin_layout(lua_State* L) {
    Clay_BeginLayout();
    return 0;
}

static int l_clay_end_layout(lua_State* L) {
    Clay_RenderCommandArray commands = Clay_EndLayout();
    lua_pushinteger(L, commands.length);
    return 1;
}

static int l_clay_open_element(lua_State* L) {
    Clay__OpenElement();
    return 0;
}

static int l_clay_close_element(lua_State* L) {
    Clay__CloseElement();
    return 0;
}

static int l_clay_configure_element(lua_State* L) {
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "Expected table for element configuration");
    }
    
    Clay_ElementDeclaration config = {};
    
    lua_getfield(L, 1, "layout");
    if (lua_istable(L, -1)) {
        Clay_LayoutConfig layoutConfig = {};
        
        lua_getfield(L, -1, "width");
        if (lua_isnumber(L, -1)) {
            float width = static_cast<float>(lua_tonumber(L, -1));
            layoutConfig.sizing.width = CLAY_SIZING_FIXED(width);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "height");
        if (lua_isnumber(L, -1)) {
            float height = static_cast<float>(lua_tonumber(L, -1));
            layoutConfig.sizing.height = CLAY_SIZING_FIXED(height);
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "padding");
        if (lua_isnumber(L, -1)) {
            uint16_t padding = static_cast<uint16_t>(lua_tointeger(L, -1));
            layoutConfig.padding = {padding, padding, padding, padding};
        }
        lua_pop(L, 1);
        
        config.layout = layoutConfig;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "backgroundColor");
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); float r = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); float g = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); float b = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        lua_rawgeti(L, -1, 4); float a = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        config.backgroundColor = {r, g, b, a};
    }
    lua_pop(L, 1);
    
    Clay__ConfigureOpenElement(config);
    return 0;
}

static int l_clay_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    
    Clay_TextElementConfig textConfig = {};
    textConfig.fontSize = 14;
    textConfig.textColor = {0, 0, 0, 255};
    
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "fontSize");
        if (lua_isnumber(L, -1)) {
            textConfig.fontSize = static_cast<uint16_t>(lua_tointeger(L, -1));
        }
        lua_pop(L, 1);
        
        lua_getfield(L, 2, "color");
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); textConfig.textColor.r = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_rawgeti(L, -1, 2); textConfig.textColor.g = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_rawgeti(L, -1, 3); textConfig.textColor.b = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
            lua_rawgeti(L, -1, 4); textConfig.textColor.a = static_cast<float>(lua_tonumber(L, -1)); lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    
    Clay_String clayString = {
        .isStaticallyAllocated = false,
        .length = static_cast<int32_t>(strlen(text)),
        .chars = text
    };
    
    Clay__OpenTextElement(clayString, Clay__StoreTextElementConfig(textConfig));
    
    return 0;
}

static int l_clay_id(lua_State* L) {
    const char* idString = luaL_checkstring(L, 1);
    Clay_String clayString = {
        .isStaticallyAllocated = false,
        .length = static_cast<int32_t>(strlen(idString)),
        .chars = idString
    };
    
    Clay_ElementId id = Clay__HashString(clayString, 0, 0);
    lua_pushinteger(L, id.id);
    return 1;
}

static int l_clay_pointer_over(lua_State* L) {
    uint32_t elementId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    Clay_ElementId id = {elementId, 0, 0, {}};
    bool isOver = Clay_PointerOver(id);
    lua_pushboolean(L, isOver);
    return 1;
}

static int l_clay_get_element_data(lua_State* L) {
    uint32_t elementId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    Clay_ElementId id = {elementId, 0, 0, {}};
    Clay_ElementData data = Clay_GetElementData(id);
    
    if (data.found) {
        lua_newtable(L);
        lua_pushnumber(L, data.boundingBox.x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, data.boundingBox.y); lua_setfield(L, -2, "y");
        lua_pushnumber(L, data.boundingBox.width); lua_setfield(L, -2, "width");
        lua_pushnumber(L, data.boundingBox.height); lua_setfield(L, -2, "height");
        return 1;
    }
    
    return 0;
}

static int l_clay_update_scroll(lua_State* L) {
    bool enableDrag = lua_toboolean(L, 1);
    float scrollX = static_cast<float>(luaL_checknumber(L, 2));
    float scrollY = static_cast<float>(luaL_checknumber(L, 3));
    float deltaTime = static_cast<float>(luaL_checknumber(L, 4));
    
    Clay_UpdateScrollContainers(enableDrag, {scrollX, scrollY}, deltaTime);
    return 0;
}

static int l_clay_set_debug_mode(lua_State* L) {
    bool enabled = lua_toboolean(L, 1);
    Clay_SetDebugModeEnabled(enabled);
    return 0;
}

static int l_clay_render(lua_State* L) {
    RenWindow* window = nullptr;
    
    if (lua_isuserdata(L, 1)) {
        RenWindow** window_ptr = (RenWindow**)luaL_checkudata(L, 1, API_TYPE_RENWINDOW);
        window = *window_ptr;
    } else {
        window = ren_get_target_window();
    }
    
    if (!window) {
        return luaL_error(L, "No render window available");
    }
    
    SDL_Renderer* renderer = renwin_get_renderer(window);
    if (!renderer) {
        return luaL_error(L, "No SDL renderer available - rebuild with -Drenderer=true");
    }
    
    if (!clay::g_clay_renderer) {
        clay::g_clay_renderer = new clay::ClayRenderer();
        clay::g_clay_renderer->Initialize(window);
    }
    
    Clay_RenderCommandArray commands = Clay_EndLayout();
    
    rencache_begin_frame(window);
    clay::g_clay_renderer->RenderCommands(commands);
    rencache_end_frame(window);
    
    Clay_BeginLayout();
    
    return 0;
}

static const luaL_Reg clay_lib[] = {
    {"initialize", l_clay_initialize},
    {"set_dimensions", l_clay_set_dimensions},
    {"set_pointer", l_clay_set_pointer},
    {"begin_layout", l_clay_begin_layout},
    {"end_layout", l_clay_end_layout},
    {"open_element", l_clay_open_element},
    {"close_element", l_clay_close_element},
    {"configure_element", l_clay_configure_element},
    {"text", l_clay_text},
    {"id", l_clay_id},
    {"pointer_over", l_clay_pointer_over},
    {"get_element_data", l_clay_get_element_data},
    {"update_scroll", l_clay_update_scroll},
    {"set_debug_mode", l_clay_set_debug_mode},
    {"render", l_clay_render},
    {nullptr, nullptr}
};

extern "C" {

int luaopen_clay(lua_State* L) {
    luaL_newlib(L, clay_lib);
    
    lua_pushinteger(L, CLAY_LEFT_TO_RIGHT);
    lua_setfield(L, -2, "LEFT_TO_RIGHT");
    
    lua_pushinteger(L, CLAY_TOP_TO_BOTTOM);
    lua_setfield(L, -2, "TOP_TO_BOTTOM");
    
    return 1;
}

}
