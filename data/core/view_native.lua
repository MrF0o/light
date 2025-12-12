-- This module provides direct transparent access to the C++ View class
-- Using FFI metatype for direct inheritance - no .native indirection needed
local ffi = require "ffi"
local Object = require "core.object"

-- FFI C declarations for View
ffi.cdef [[
  typedef struct {
    float x, y;
  } Position;
  
  typedef struct {
    float x, y;
  } Size;
  
  typedef struct {
    float x, y;
    Position to;
  } Scroll;
  
  typedef struct View View;

  View* view_new();
  void view_delete(View* view);
  const char* view_to_string(View* view);
  const char* view_get_name(View* view);
  float view_get_scrollable_size(View* view);
  float view_get_h_scrollable_size(View* view);
  bool view_supports_text_input(View* view);
  bool view_on_mouse_pressed(View* view, const char* button, float x, float y, int clicks);
  void view_on_mouse_released(View* view, const char* button, float x, float y);
  bool view_on_mouse_moved(View* view, float x, float y, float dx, float dy);
  void view_on_mouse_left(View* view);
  bool view_on_mouse_wheel(View* view, float y, float x);
  bool view_on_file_dropped(View* view, const char* filename, float x, float y);
  void view_on_text_input(View* view, const char* text);
  void view_on_ime_text_editing(View* view, const char* text, int start, int length);
  void view_on_touch_moved(View* view, float x, float y, float dx, float dy, int i);
  void view_on_scale_change(View* view, float new_scale, float prev_scale);
  void view_get_content_bounds(View* view, float* x1, float* y1, float* x2, float* y2);
  void view_get_content_offset(View* view, float* offset_x, float* offset_y);
  void view_clamp_scroll_position(View* view);
  void view_update(View* view);
  void view_draw(View* view, void* surface);
  void view_draw_background(View* view, void* surface, int r, int g, int b, int a);
  void* view_on_context_menu(View* view, float x, float y);  
  void view_try_close(View* view, void (*do_close)());
  void* view_get_position_ptr(View* view);
  void* view_get_size_ptr(View* view);
  void* view_get_scroll_ptr(View* view);
  bool* view_get_scrollable_ptr(View* view);
  const char* view_get_cursor(View* view);
  void view_set_cursor(View* view, const char* cursor);
  const char* view_get_context(View* view);
  void view_set_context(View* view, const char* context);
  bool view_get_scrollable(View* view);
  void view_set_scrollable(View* view, bool scrollable);
  float view_get_current_scale(View* view);
  void view_set_current_scale(View* view, float scale);
]]

local C = ffi.C
local Scrollbar = require "core.scrollbar"

---@class core.view : core.object
local View = Object:extend()

function View:new()
    local view_ptr = C.view_new()

    self.position = ffi.cast("Position*", C.view_get_position_ptr(view_ptr))
    self.size = ffi.cast("Size*", C.view_get_size_ptr(view_ptr))
    self.scroll = ffi.cast("Scroll*", C.view_get_scroll_ptr(view_ptr))
    self._scrollable_ptr = ffi.cast("bool*", C.view_get_scrollable_ptr(view_ptr))
    self._view_ptr = view_ptr

    self.cursor = "arrow"
    self.scrollable = false
    self.current_scale = SCALE

    self.v_scrollbar = Scrollbar({
        direction = "v",
        alignment = "e"
    })
    self.h_scrollbar = Scrollbar({
        direction = "h",
        alignment = "e"
    })
    
    -- cleanup
    ffi.gc(view_ptr, C.view_delete)
end

local simple_methods = {"get_scrollable_size", "get_h_scrollable_size", "supports_text_input", "on_mouse_pressed",
                        "on_mouse_moved", "on_mouse_left", "on_file_dropped", "on_text_input", "on_ime_text_editing",
                        "on_scale_change", "on_touch_moved", "on_context_menu"}

for _, name in ipairs(simple_methods) do
    local c_func = C["view_" .. name]
    View[name] = function(self, ...)
        return c_func(self._view_ptr, ...)
    end
end

local string_methods = {"get_name"}

for _, name in ipairs(string_methods) do
    local c_func = C["view_" .. name]
    View[name] = function(self, ...)
        return ffi.string(c_func(self._view_ptr, ...))
    end
end


function View:on_mouse_wheel(y, x)
    -- no-op
end

function View:scrollbar_overlaps_point(x, y)
    return not (not (self.v_scrollbar:overlaps(x, y) or self.h_scrollbar:overlaps(x, y)))
end

function View:scrollbar_dragging()
    return self.v_scrollbar.dragging or self.h_scrollbar.dragging
end

function View:scrollbar_hovering()
    return self.v_scrollbar.hovering.track or self.h_scrollbar.hovering.track
end

function View:on_mouse_pressed(button, x, y, clicks)
    if not self.scrollable then
        return
    end
    local result = self.v_scrollbar:on_mouse_pressed(button, x, y, clicks)
    if result then
        if result ~= true then
            self.scroll.to.y = result * (self:get_scrollable_size() - self.size.y)
        end
        return true
    end
    result = self.h_scrollbar:on_mouse_pressed(button, x, y, clicks)
    if result then
        if result ~= true then
            self.scroll.to.x = result * (self:get_h_scrollable_size() - self.size.x)
        end
        return true
    end
    return C.view_on_mouse_pressed(self._view_ptr, button, x, y, clicks)
end

function View:on_mouse_released(button, x, y)
    if not self.scrollable then
        return
    end
    self.v_scrollbar:on_mouse_released(button, x, y)
    self.h_scrollbar:on_mouse_released(button, x, y)
    C.view_on_mouse_released(self._view_ptr, button, x, y)
end

function View:on_mouse_moved(x, y, dx, dy)
    if not self.scrollable then
        return
    end
    local result
    if self.h_scrollbar.dragging then
        goto skip_v_scrollbar
    end
    result = self.v_scrollbar:on_mouse_moved(x, y, dx, dy)
    if result then
        if result ~= true then
            self.scroll.to.y = result * (self:get_scrollable_size() - self.size.y)
            if not require("core.config").animate_drag_scroll then
                self:clamp_scroll_position()
                self.scroll.y = self.scroll.to.y
            end
        end
        -- hide horizontal scrollbar
        self.h_scrollbar:on_mouse_left()
        return true
    end
    ::skip_v_scrollbar::
    result = self.h_scrollbar:on_mouse_moved(x, y, dx, dy)
    if result then
        if result ~= true then
            self.scroll.to.x = result * (self:get_h_scrollable_size() - self.size.x)
            if not require("core.config").animate_drag_scroll then
                self:clamp_scroll_position()
                self.scroll.x = self.scroll.to.x
            end
        end
        return true
    end
    return C.view_on_mouse_moved(self._view_ptr, x, y, dx, dy)
end

function View:on_mouse_left()
    if not self.scrollable then
        return
    end
    self.v_scrollbar:on_mouse_left()
    self.h_scrollbar:on_mouse_left()
    C.view_on_mouse_left(self._view_ptr)
end

function View:__tostring()
    return ffi.string(C.view_to_string(self._view_ptr))
end

function View:get_content_bounds()
    local x1, y1, x2, y2 = ffi.new("float[1]"), ffi.new("float[1]"), ffi.new("float[1]"), ffi.new("float[1]")
    C.view_get_content_bounds(self._view_ptr, x1, y1, x2, y2)
    return x1[0], y1[0], x2[0], y2[0]
end

function View:get_content_offset()
    local x, y = ffi.new("float[1]"), ffi.new("float[1]")
    C.view_get_content_offset(self._view_ptr, x, y)
    return x[0], y[0]
end

function View:draw()
end

function View:clamp_scroll_position()
    local max = self:get_scrollable_size() - self.size.y
    local common = require "core.common"
    self.scroll.to.y = common.clamp(self.scroll.to.y, 0, max)

    max = self:get_h_scrollable_size() - self.size.x
    self.scroll.to.x = common.clamp(self.scroll.to.x, 0, max)
end

function View:update_scrollbar()
    local v_scrollable = self:get_scrollable_size()
    self.v_scrollbar:set_size(self.position.x, self.position.y, self.size.x, self.size.y, v_scrollable)
    local v_percent = self.scroll.y / (v_scrollable - self.size.y)
    -- Avoid setting nan percent
    self.v_scrollbar:set_percent(v_percent == v_percent and v_percent or 0)
    self.v_scrollbar:update()

    local h_scrollable = self:get_h_scrollable_size()
    self.h_scrollbar:set_size(self.position.x, self.position.y, self.size.x, self.size.y, h_scrollable)
    local h_percent = self.scroll.x / (h_scrollable - self.size.x)
    -- Avoid setting nan percent
    self.h_scrollbar:set_percent(h_percent == h_percent and h_percent or 0)
    self.h_scrollbar:update()
end

function View:update()
    if self.current_scale ~= SCALE then
        self:on_scale_change(SCALE, self.current_scale)
        self.current_scale = SCALE
    end

    self:clamp_scroll_position()
    self:move_towards(self.scroll, "x", self.scroll.to.x, 0.3, "scroll")
    self:move_towards(self.scroll, "y", self.scroll.to.y, 0.3, "scroll")
    if not self.scrollable then
        return
    end
    self:update_scrollbar()
    C.view_update(self._view_ptr)
end

function View:draw_background(color)
    local renderer = require "renderer"
    local x, y = self.position.x, self.position.y
    local w, h = self.size.x, self.size.y
    renderer.draw_rect(x, y, w, h, color)
end

function View:draw_scrollbar()
    self.v_scrollbar:draw()
    self.h_scrollbar:draw()
end

function View:try_close(do_close)
    if do_close then
        do_close()
    end
end

function View:move_towards(t, k, dest, rate, name)
    local config = require "core.config"
    local common = require "core.common"
    local core = require "core"

    if type(t) ~= "table" and type(t) ~= "cdata" then
        return self:move_towards(self, t, k, dest, rate, name)
    end

    local val = t[k]
    local diff = math.abs(val - dest)
    if not config.transitions or diff < 0.5 or config.disabled_transitions[name] then
        t[k] = dest
    else
        rate = rate or 0.5
        if config.fps ~= 60 or config.animation_rate ~= 1 then
            local dt = 60 / config.fps
            rate = 1 - common.clamp(1 - rate, 1e-8, 1 - 1e-8) ^ (config.animation_rate * dt)
        end
        t[k] = common.lerp(val, dest, rate)
    end
    if diff > 1e-8 then
        core.redraw = true
    end
end

View.context = "application"

function View:__newindex(k, v)
    if k == "scrollable" then
        local ptr = rawget(self, "_scrollable_ptr")
        if ptr ~= nil then
            ptr[0] = v
            return
        end
    end
    rawset(self, k, v)
end

local view_cls_index = View.__index
function View:__index(k)
    if k == "scrollable" then
        local ptr = rawget(self, "_scrollable_ptr")
        if ptr ~= nil then
            return ptr[0]
        end
    end
    return view_cls_index[k]
end

function View:extend()
    local cls = Object.extend(self)
    local cls_index = cls.__index
    function cls:__index(k)
        if k == "scrollable" then
            local ptr = rawget(self, "_scrollable_ptr")
            if ptr ~= nil then
                return ptr[0]
            end
        end
        return cls_index[k]
    end
    return cls
end

return View
