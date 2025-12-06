#include <cstring>
#include <vector>
#include <string>

extern "C" {
#include "api.h"
#include "clay.h"
#include "lua_compat.h"
#include "rencache.h"
#include "renwindow.h"
}

#include "clay_renderer.hpp"
#include "clay_rencache.hpp"

static Clay_Arena clay_arena;
static Clay_Context *clay_context = nullptr;
static int clay_open_depth = 0; // tracks open/close balance
static std::vector<std::string> clay_string_storage;

namespace clay {
ClayRenderer *g_clay_renderer = nullptr;
}

static Clay_Dimensions MeasureText(Clay_StringSlice text,
                                   Clay_TextElementConfig *config,
                                   void *userData) {
  clay::FontGroup* group = clay::ClayRencache::GetFont(config->fontId);
  if (group) {
      int x_offset;
      double width = ren_font_group_get_width(group->fonts.data(), text.chars, text.length, {0}, &x_offset);
      float height = ren_font_group_get_height(group->fonts.data());
      return {static_cast<float>(width), height};
  }
  float charWidth = config->fontSize * 0.6f;
  return {.width = text.length * charWidth,
          .height = static_cast<float>(config->fontSize)};
}

static void ClayErrorHandler(Clay_ErrorData errorData) {
  fprintf(stderr, "Clay Error: %.*s\n", (int)errorData.errorText.length,
          errorData.errorText.chars);
}

static bool retrieve_font_group(lua_State* L, int idx, clay::FontGroup& group) {
  group.fonts.fill(nullptr);
  if (lua_type(L, idx) != LUA_TTABLE) {
    RenFont** font = (RenFont**)luaL_checkudata(L, idx, API_TYPE_FONT);
    group.fonts[0] = *font;
  } else {
    int len = luaL_len(L, idx);
    len = len > FONT_FALLBACK_MAX ? FONT_FALLBACK_MAX : len;
    for (int i = 0; i < len; i++) {
      lua_rawgeti(L, idx, i+1);
      RenFont** font = (RenFont**)luaL_checkudata(L, -1, API_TYPE_FONT);
      group.fonts[i] = *font;
      lua_pop(L, 1);
    }
  }
  return true;
}

static int l_clay_initialize(lua_State *L) {
  int width = luaL_checkinteger(L, 1);
  int height = luaL_checkinteger(L, 2);

  if (!clay_arena.memory) {
    uint32_t memorySize = Clay_MinMemorySize();
    clay_arena =
        Clay_CreateArenaWithCapacityAndMemory(memorySize, malloc(memorySize));

    Clay_Dimensions dimensions = {static_cast<float>(width),
                                  static_cast<float>(height)};
    Clay_ErrorHandler errorHandler = {ClayErrorHandler, nullptr};

    clay_context = Clay_Initialize(clay_arena, dimensions, errorHandler);
    Clay_SetMeasureTextFunction(MeasureText, nullptr);
  }

  return 0;
}

static int l_clay_set_dimensions(lua_State *L) {
  float width = static_cast<float>(luaL_checknumber(L, 1));
  float height = static_cast<float>(luaL_checknumber(L, 2));

  Clay_SetLayoutDimensions({width, height});
  return 0;
}

static int l_clay_set_pointer(lua_State *L) {
  float x = static_cast<float>(luaL_checknumber(L, 1));
  float y = static_cast<float>(luaL_checknumber(L, 2));
  bool down = lua_toboolean(L, 3);

  Clay_SetPointerState({x, y}, down);
  return 0;
}

static int l_clay_begin_layout(lua_State *L) {
  clay_open_depth = 0;
  clay_string_storage.clear();
  clay::ClayRencache::ResetFonts();
  Clay_BeginLayout();
  return 0;
}

static int l_clay_end_layout(lua_State *L) {
  Clay_RenderCommandArray commands = Clay_EndLayout();
  lua_pushinteger(L, commands.length);

  return 1;
}

static int l_clay_open_element(lua_State *L) {
  Clay__OpenElement();
  clay_open_depth++;
  return 0;
}

static int l_clay_close_element(lua_State *L) {
  if (clay_open_depth <= 0) {
    return luaL_error(L, "clay:close_element called with no matching open_element");
  }
  clay_open_depth--;
  Clay__CloseElement();
  return 0;
}

static Clay_SizingAxis ParseSizingAxis(lua_State* L) {
    Clay_SizingAxis axis = CLAY_SIZING_GROW(0);
    if (lua_isnumber(L, -1)) {
        axis = CLAY_SIZING_FIXED(static_cast<float>(lua_tonumber(L, -1)));
    } else if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "type");
        const char* type = lua_tostring(L, -1);
        lua_pop(L, 1);
        
        float min = 0, max = 0;
        lua_getfield(L, -1, "min");
        if (lua_isnumber(L, -1)) min = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "max");
        if (lua_isnumber(L, -1)) max = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "value");
        float value = 0;
        if (lua_isnumber(L, -1)) value = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);

        if (type) {
            if (strcmp(type, "grow") == 0) {
                axis.type = CLAY__SIZING_TYPE_GROW;
                axis.size.minMax = {min, max};
            } else if (strcmp(type, "fixed") == 0) {
                axis.type = CLAY__SIZING_TYPE_FIXED;
                axis.size.minMax = {value, value};
            } else if (strcmp(type, "percent") == 0) {
                axis.type = CLAY__SIZING_TYPE_PERCENT;
                axis.size.percent = value;
            } else if (strcmp(type, "fit") == 0) {
                axis.type = CLAY__SIZING_TYPE_FIT;
                axis.size.minMax = {min, max};
            }
        }
    }
    return axis;
}

static int l_clay_configure_element(lua_State *L) {
  if (!lua_istable(L, 1)) {
    return luaL_error(L, "Expected table for element configuration");
  }

  Clay_ElementDeclaration config = {};

  lua_getfield(L, 1, "id");
  if (lua_isnumber(L, -1)) {
      config.id = { static_cast<uint32_t>(lua_tointeger(L, -1)) };
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "layout");
  if (lua_istable(L, -1)) {
    Clay_LayoutConfig layoutConfig = {};

    // Sizing
    lua_getfield(L, -1, "sizing");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "width");
        layoutConfig.sizing.width = ParseSizingAxis(L);
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "height");
        layoutConfig.sizing.height = ParseSizingAxis(L);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Padding
    lua_getfield(L, -1, "padding");
    if (lua_isnumber(L, -1)) {
      uint16_t padding = static_cast<uint16_t>(lua_tointeger(L, -1));
      layoutConfig.padding = {padding, padding, padding, padding};
    } else if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "x");
        if (lua_isnumber(L, -1)) {
            uint16_t p = static_cast<uint16_t>(lua_tointeger(L, -1));
            layoutConfig.padding.left = p;
            layoutConfig.padding.right = p;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "y");
        if (lua_isnumber(L, -1)) {
            uint16_t p = static_cast<uint16_t>(lua_tointeger(L, -1));
            layoutConfig.padding.top = p;
            layoutConfig.padding.bottom = p;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "left");
        if (lua_isnumber(L, -1)) layoutConfig.padding.left = static_cast<uint16_t>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "right");
        if (lua_isnumber(L, -1)) layoutConfig.padding.right = static_cast<uint16_t>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "top");
        if (lua_isnumber(L, -1)) layoutConfig.padding.top = static_cast<uint16_t>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "bottom");
        if (lua_isnumber(L, -1)) layoutConfig.padding.bottom = static_cast<uint16_t>(lua_tointeger(L, -1));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Child Gap
    lua_getfield(L, -1, "childGap");
    if (lua_isnumber(L, -1)) {
        layoutConfig.childGap = static_cast<uint16_t>(lua_tointeger(L, -1));
    }
    lua_pop(L, 1);
    
    // Layout Direction
    lua_getfield(L, -1, "layoutDirection");
    if (lua_isstring(L, -1)) {
        const char* dir = lua_tostring(L, -1);
        if (strcmp(dir, "row") == 0 || strcmp(dir, "leftToRight") == 0) {
            layoutConfig.layoutDirection = CLAY_LEFT_TO_RIGHT;
        } else if (strcmp(dir, "column") == 0 || strcmp(dir, "topToBottom") == 0) {
            layoutConfig.layoutDirection = CLAY_TOP_TO_BOTTOM;
        }
    }
    lua_pop(L, 1);
    
    // Child Alignment
    lua_getfield(L, -1, "childAlignment");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "x");
        if (lua_isstring(L, -1)) {
            const char* align = lua_tostring(L, -1);
            if (strcmp(align, "left") == 0) layoutConfig.childAlignment.x = CLAY_ALIGN_X_LEFT;
            else if (strcmp(align, "center") == 0) layoutConfig.childAlignment.x = CLAY_ALIGN_X_CENTER;
            else if (strcmp(align, "right") == 0) layoutConfig.childAlignment.x = CLAY_ALIGN_X_RIGHT;
        }
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "y");
        if (lua_isstring(L, -1)) {
            const char* align = lua_tostring(L, -1);
            if (strcmp(align, "top") == 0) layoutConfig.childAlignment.y = CLAY_ALIGN_Y_TOP;
            else if (strcmp(align, "center") == 0) layoutConfig.childAlignment.y = CLAY_ALIGN_Y_CENTER;
            else if (strcmp(align, "bottom") == 0) layoutConfig.childAlignment.y = CLAY_ALIGN_Y_BOTTOM;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    config.layout = layoutConfig;
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "backgroundColor");
  if (lua_istable(L, -1)) {
    lua_rawgeti(L, -1, 1);
    float r = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
    float g = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
    float b = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 4);
    float a = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    config.backgroundColor = {r, g, b, a};
  }
  lua_pop(L, 1);

  Clay__ConfigureOpenElement(config);
  return 0;
}

static int l_clay_text(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  
  clay_string_storage.emplace_back(text);
  const std::string& storedText = clay_string_storage.back();

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
      lua_rawgeti(L, -1, 1);
      textConfig.textColor.r = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 2);
      textConfig.textColor.g = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 3);
      textConfig.textColor.b = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);
      lua_rawgeti(L, -1, 4);
      textConfig.textColor.a = static_cast<float>(lua_tonumber(L, -1));
      lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "font");
    if (!lua_isnil(L, -1)) {
        clay::FontGroup group;
        if (retrieve_font_group(L, -1, group)) {
            textConfig.fontId = clay::ClayRencache::AddFont(group);
        }
    }
    lua_pop(L, 1);
  }

  Clay_String clayString = {
    .isStaticallyAllocated = false,
    .length = static_cast<int32_t>(storedText.length()),
    .chars = storedText.c_str()
  };

  Clay__OpenTextElement(clayString, Clay__StoreTextElementConfig(textConfig));

  return 0;
}

static int l_clay_id(lua_State *L) {
  const char *idString = luaL_checkstring(L, 1);
  
  clay_string_storage.emplace_back(idString);
  const std::string& storedId = clay_string_storage.back();

  Clay_String clayString = {
    .isStaticallyAllocated = false,
    .length = static_cast<int32_t>(storedId.length()),
    .chars = storedId.c_str()
  };

  Clay_ElementId id = Clay__HashString(clayString, 0, 0);
  lua_pushinteger(L, id.id);
  return 1;
}

static int l_clay_pointer_over(lua_State *L) {
  uint32_t elementId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
  Clay_ElementId id = {elementId, 0, 0, {}};
  bool isOver = Clay_PointerOver(id);
  lua_pushboolean(L, isOver);
  return 1;
}

static int l_clay_get_element_data(lua_State *L) {
  uint32_t elementId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
  Clay_ElementId id = {elementId, 0, 0, {}};
  Clay_ElementData data = Clay_GetElementData(id);

  if (data.found) {
    lua_newtable(L);
    lua_pushnumber(L, data.boundingBox.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, data.boundingBox.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, data.boundingBox.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, data.boundingBox.height);
    lua_setfield(L, -2, "height");
    return 1;
  }

  return 0;
}

static int l_clay_update_scroll(lua_State *L) {
  bool enableDrag = lua_toboolean(L, 1);
  float scrollX = static_cast<float>(luaL_checknumber(L, 2));
  float scrollY = static_cast<float>(luaL_checknumber(L, 3));
  float deltaTime = static_cast<float>(luaL_checknumber(L, 4));

  Clay_UpdateScrollContainers(enableDrag, {scrollX, scrollY}, deltaTime);
  return 0;
}

static int l_clay_set_debug_mode(lua_State *L) {
  bool enabled = lua_toboolean(L, 1);
  Clay_SetDebugModeEnabled(enabled);
  return 0;
}

static int l_clay_render(lua_State *L) {
  RenWindow *window = nullptr;

  if (lua_isuserdata(L, 1)) {
    RenWindow **window_ptr =
        (RenWindow **)luaL_checkudata(L, 1, API_TYPE_RENWINDOW);
    window = *window_ptr;
  } else {
    window = ren_get_target_window();
  }

  if (!window) {
    return luaL_error(L, "No render window available");
  }

  if (!clay::g_clay_renderer) {
    clay::g_clay_renderer = new clay::ClayRenderer();
    clay::g_clay_renderer->Initialize(window);
  }

  Clay_RenderCommandArray commands = Clay_EndLayout();

  rencache_begin_frame(window);
  clay::g_clay_renderer->RenderCommands(commands);
  rencache_end_frame(window);

  lua_pushinteger(L, commands.length);
  return 1;
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
int luaopen_clay(lua_State *L) {
  luaL_newlib(L, clay_lib);

  lua_pushinteger(L, CLAY_LEFT_TO_RIGHT);
  lua_setfield(L, -2, "LEFT_TO_RIGHT");

  lua_pushinteger(L, CLAY_TOP_TO_BOTTOM);
  lua_setfield(L, -2, "TOP_TO_BOTTOM");

  return 1;
}
}
