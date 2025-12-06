#include <new>

extern "C"
{
#include "api.h"
}

#include "views/View.hpp"
#include "views/renviews/ViewRenderer.hpp"

using namespace view;

#define API_TYPE_VIEW "View"

static View *checkview(lua_State *L, int idx)
{
  void *ud = luaL_checkudata(L, idx, API_TYPE_VIEW);
  return *((View **)ud);
}

// push a View to Lua
static void pushview(lua_State *L, View *view)
{
  View **ud = (View **)lua_newuserdata(L, sizeof(View *));
  *ud = view;
  luaL_setmetatable(L, API_TYPE_VIEW);

  view->L = L;
  // We don't create a registry reference here to avoid circular dependency
  // The Lua object owns the C++ object, not the other way around
  view->luaRef = LUA_NOREF;
}

// View:new()
static int l_view_new(lua_State *L)
{
  View *view = new (std::nothrow) View();
  if (!view)
  {
    return luaL_error(L, "Failed to allocate View");
  }
  pushview(L, view);
  return 1;
}

// View:__gc()
static int l_view_gc(lua_State *L)
{
  View *view = checkview(L, 1);
  if (view)
  {
    delete view;
  }
  return 0;
}

// View:__tostring()
static int l_view_tostring(lua_State *L)
{
  View *view = checkview(L, 1);
  std::string str = view->toString();
  lua_pushstring(L, str.c_str());
  return 1;
}

// View:get_name()
static int l_view_get_name(lua_State *L)
{
  View *view = checkview(L, 1);
  std::string name = view->getName();
  lua_pushstring(L, name.c_str());
  return 1;
}

// View:get_scrollable_size()
static int l_view_get_scrollable_size(lua_State *L)
{
  View *view = checkview(L, 1);
  float size = view->getScrollableSize();
  lua_pushnumber(L, size);
  return 1;
}

// View:get_h_scrollable_size()
static int l_view_get_h_scrollable_size(lua_State *L)
{
  View *view = checkview(L, 1);
  float size = view->getHScrollableSize();
  lua_pushnumber(L, size);
  return 1;
}

// View:supports_text_input()
static int l_view_supports_text_input(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushboolean(L, view->supportsTextInput());
  return 1;
}

// View:scrollbar_overlaps_point(x, y)
static int l_view_scrollbar_overlaps_point(lua_State *L)
{
  View *view = checkview(L, 1);
  float x = luaL_checknumber(L, 2);
  float y = luaL_checknumber(L, 3);
  lua_pushboolean(L, view->scrollbarOverlapsPoint(x, y));
  return 1;
}

// View:scrollbar_dragging()
static int l_view_scrollbar_dragging(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushboolean(L, view->scrollbarDragging());
  return 1;
}

// View:scrollbar_hovering()
static int l_view_scrollbar_hovering(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushboolean(L, view->scrollbarHovering());
  return 1;
}

// View:on_mouse_pressed(button, x, y, clicks)
static int l_view_on_mouse_pressed(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *button = luaL_checkstring(L, 2);
  float x = luaL_checknumber(L, 3);
  float y = luaL_checknumber(L, 4);
  int clicks = luaL_checkinteger(L, 5);
  bool handled = view->onMousePressed(button, x, y, clicks);
  lua_pushboolean(L, handled);
  return 1;
}

// View:on_mouse_released(button, x, y)
static int l_view_on_mouse_released(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *button = luaL_checkstring(L, 2);
  float x = luaL_checknumber(L, 3);
  float y = luaL_checknumber(L, 4);
  view->onMouseReleased(button, x, y);
  return 0;
}

// View:on_mouse_moved(x, y, dx, dy)
static int l_view_on_mouse_moved(lua_State *L)
{
  View *view = checkview(L, 1);
  float x = luaL_checknumber(L, 2);
  float y = luaL_checknumber(L, 3);
  float dx = luaL_checknumber(L, 4);
  float dy = luaL_checknumber(L, 5);
  bool handled = view->onMouseMoved(x, y, dx, dy);
  lua_pushboolean(L, handled);
  return 1;
}

// View:on_mouse_left()
static int l_view_on_mouse_left(lua_State *L)
{
  View *view = checkview(L, 1);
  view->onMouseLeft();
  return 0;
}

// View:on_mouse_wheel(y, x)
static int l_view_on_mouse_wheel(lua_State *L)
{
  View *view = checkview(L, 1);
  float y = luaL_checknumber(L, 2);
  float x = luaL_checknumber(L, 3);
  bool handled = view->onMouseWheel(y, x);
  lua_pushboolean(L, handled);
  return 1;
}

// View:on_file_dropped(filename, x, y)
static int l_view_on_file_dropped(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *filename = luaL_checkstring(L, 2);
  float x = luaL_checknumber(L, 3);
  float y = luaL_checknumber(L, 4);
  bool handled = view->onFileDropped(filename, x, y);
  lua_pushboolean(L, handled);
  return 1;
}

// View:on_text_input(text)
static int l_view_on_text_input(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *text = luaL_checkstring(L, 2);
  view->onTextInput(text);
  return 0;
}

// View:on_scale_change(new_scale, prev_scale)
static int l_view_on_scale_change(lua_State *L)
{
  View *view = checkview(L, 1);
  float newScale = luaL_checknumber(L, 2);
  float prevScale = luaL_checknumber(L, 3);
  view->onScaleChange(newScale, prevScale);
  return 0;
}

// View:get_content_bounds()
static int l_view_get_content_bounds(lua_State *L)
{
  View *view = checkview(L, 1);
  float x1, y1, x2, y2;
  view->getContentBounds(x1, y1, x2, y2);
  lua_pushnumber(L, x1);
  lua_pushnumber(L, y1);
  lua_pushnumber(L, x2);
  lua_pushnumber(L, y2);
  return 4;
}

// View:get_content_offset()
static int l_view_get_content_offset(lua_State *L)
{
  View *view = checkview(L, 1);
  float x, y;
  view->getContentOffset(x, y);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  return 2;
}

// View:clamp_scroll_position()
static int l_view_clamp_scroll_position(lua_State *L)
{
  View *view = checkview(L, 1);
  view->clampScrollPosition();
  return 0;
}

// View:update()
static int l_view_update(lua_State *L)
{
  View *view = checkview(L, 1);
  view->update();
  return 0;
}

// View:draw()
static int l_view_draw(lua_State *L)
{
  View *view = checkview(L, 1);
  // Draw is a no-op in the base View class
  // The native view only handles state management and events
  view->draw((RenSurface *)nullptr);
  return 0;
}

// View:draw_background(color)
static int l_view_draw_background(lua_State *L)
{
  View *view = checkview(L, 1);

  // Color as table {r, g, b, a}
  luaL_checktype(L, 2, LUA_TTABLE);
  lua_rawgeti(L, 2, 1);
  lua_rawgeti(L, 2, 2);
  lua_rawgeti(L, 2, 3);
  lua_rawgeti(L, 2, 4);

  RenColor color;
  color.r = lua_tointeger(L, -4);
  color.g = lua_tointeger(L, -3);
  color.b = lua_tointeger(L, -2);
  color.a = lua_tointeger(L, -1);
  lua_pop(L, 4);

  // Note: Background drawing should be handled by Lua using renderer.draw_rect
  view->drawBackground(nullptr, color);
  return 0;
}

// View.position
static int l_view_get_position(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_createtable(L, 0, 2);
  lua_pushnumber(L, view->position.x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, view->position.y);
  lua_setfield(L, -2, "y");
  return 1;
}

static int l_view_set_position(lua_State *L)
{
  View *view = checkview(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  lua_getfield(L, 2, "x");
  lua_getfield(L, 2, "y");
  view->position.x = lua_tonumber(L, -2);
  view->position.y = lua_tonumber(L, -1);
  lua_pop(L, 2);
  return 0;
}

// View.size
static int l_view_get_size(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_createtable(L, 0, 2);
  lua_pushnumber(L, view->size.x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, view->size.y);
  lua_setfield(L, -2, "y");
  return 1;
}

static int l_view_set_size(lua_State *L)
{
  View *view = checkview(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  lua_getfield(L, 2, "x");
  lua_getfield(L, 2, "y");
  view->size.x = lua_tonumber(L, -2);
  view->size.y = lua_tonumber(L, -1);
  lua_pop(L, 2);
  return 0;
}

// View.scroll
static int l_view_get_scroll(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_createtable(L, 0, 4);
  lua_pushnumber(L, view->scroll.x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, view->scroll.y);
  lua_setfield(L, -2, "y");

  lua_createtable(L, 0, 2);
  lua_pushnumber(L, view->scroll.to.x);
  lua_setfield(L, -2, "x");
  lua_pushnumber(L, view->scroll.to.y);
  lua_setfield(L, -2, "y");
  lua_setfield(L, -2, "to");

  return 1;
}

static int l_view_set_scroll(lua_State *L)
{
  View *view = checkview(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  lua_getfield(L, 2, "x");
  lua_getfield(L, 2, "y");
  view->scroll.x = lua_tonumber(L, -2);
  view->scroll.y = lua_tonumber(L, -1);
  lua_pop(L, 2);

  lua_getfield(L, 2, "to");
  if (lua_istable(L, -1))
  {
    lua_getfield(L, -1, "x");
    lua_getfield(L, -2, "y");
    view->scroll.to.x = lua_tonumber(L, -2);
    view->scroll.to.y = lua_tonumber(L, -1);
    lua_pop(L, 2);
  }
  lua_pop(L, 1);

  return 0;
}

// View.scrollable
static int l_view_get_scrollable(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushboolean(L, view->scrollable);
  return 1;
}

static int l_view_set_scrollable(lua_State *L)
{
  View *view = checkview(L, 1);
  view->scrollable = lua_toboolean(L, 2);
  return 0;
}

// View.cursor
static int l_view_get_cursor(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushstring(L, view->cursor.c_str());
  return 1;
}

static int l_view_set_cursor(lua_State *L)
{
  View *view = checkview(L, 1);
  view->cursor = luaL_checkstring(L, 2);
  return 0;
}

// View.context
static int l_view_get_context(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *ctx = (view->context == ViewContext::Application) ? "application" : "session";
  lua_pushstring(L, ctx);
  return 1;
}

static int l_view_set_context(lua_State *L)
{
  View *view = checkview(L, 1);
  const char *ctx = luaL_checkstring(L, 2);
  view->context = (strcmp(ctx, "session") == 0) ? ViewContext::Session : ViewContext::Application;
  return 0;
}

// FFI memory accessors

// View:get_ptr()
static int l_view_get_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, view);
  return 1;
}

// View:get_position_ptr()
static int l_view_get_position_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, &view->position);
  return 1;
}

// View:get_size_ptr()
static int l_view_get_size_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, &view->size);
  return 1;
}

// View:get_scroll_ptr()
static int l_view_get_scroll_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, &view->scroll);
  return 1;
}

// View:get_scrollable_ptr() - returns pointer to scrollable bool
static int l_view_get_scrollable_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, &view->scrollable);
  return 1;
}

// View:get_current_scale_ptr()
static int l_view_get_current_scale_ptr(lua_State *L)
{
  View *view = checkview(L, 1);
  lua_pushlightuserdata(L, &view->currentScale);
  return 1;
}

static const luaL_Reg view_methods[] = {
    {"__tostring", l_view_tostring},
    {"__gc", l_view_gc},
    {"get_name", l_view_get_name},
    {"get_scrollable_size", l_view_get_scrollable_size},
    {"get_h_scrollable_size", l_view_get_h_scrollable_size},
    {"supports_text_input", l_view_supports_text_input},
    {"scrollbar_overlaps_point", l_view_scrollbar_overlaps_point},
    {"scrollbar_dragging", l_view_scrollbar_dragging},
    {"scrollbar_hovering", l_view_scrollbar_hovering},
    {"on_mouse_pressed", l_view_on_mouse_pressed},
    {"on_mouse_released", l_view_on_mouse_released},
    {"on_mouse_moved", l_view_on_mouse_moved},
    {"on_mouse_left", l_view_on_mouse_left},
    {"on_mouse_wheel", l_view_on_mouse_wheel},
    {"on_file_dropped", l_view_on_file_dropped},
    {"on_text_input", l_view_on_text_input},
    {"on_scale_change", l_view_on_scale_change},
    {"get_content_bounds", l_view_get_content_bounds},
    {"get_content_offset", l_view_get_content_offset},
    {"clamp_scroll_position", l_view_clamp_scroll_position},
    {"update", l_view_update},
    {"draw", l_view_draw},
    {"draw_background", l_view_draw_background},
    // Property accessors
    {"get_position", l_view_get_position},
    {"set_position", l_view_set_position},
    {"get_size", l_view_get_size},
    {"set_size", l_view_set_size},
    {"get_scroll", l_view_get_scroll},
    {"set_scroll", l_view_set_scroll},
    {"get_scrollable", l_view_get_scrollable},
    {"set_scrollable", l_view_set_scrollable},
    {"get_cursor", l_view_get_cursor},
    {"set_cursor", l_view_set_cursor},
    {"get_context", l_view_get_context},
    {"set_context", l_view_set_context},
    // FFI pointer accessors
    {"get_ptr", l_view_get_ptr},
    {"get_position_ptr", l_view_get_position_ptr},
    {"get_size_ptr", l_view_get_size_ptr},
    {"get_scroll_ptr", l_view_get_scroll_ptr},
    {"get_scrollable_ptr", l_view_get_scrollable_ptr},
    {"get_current_scale_ptr", l_view_get_current_scale_ptr},
    {NULL, NULL}};

static const luaL_Reg view_functions[] = {
    {"new", l_view_new},
    {NULL, NULL}};

extern "C"
{
  int luaopen_view(lua_State *L)
  {
    // metatable
    luaL_newmetatable(L, API_TYPE_VIEW);
    luaL_setfuncs(L, view_methods, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    // module table
    lua_newtable(L);
    luaL_setfuncs(L, view_functions, 0);

    return 1;
  }
}
