-- This module provides direct access to the C++ View structure
local ffi = require "ffi"
local view_native = require "view"
local Object = require "core.object"

ffi.cdef [[
  typedef struct {
    float x, y;
  } ViewPosition;
  
  typedef struct {
    float x, y;
  } ViewSize;
  
  typedef struct {
    float x, y;
    ViewPosition to;
  } ViewScroll;
]]

---@class core.view : core.object
local View = Object:extend()

function View:new()
    self.native = view_native.new()
    self.position = ffi.cast("ViewPosition*", self.native:get_position_ptr())
    self.size = ffi.cast("ViewSize*", self.native:get_size_ptr())
    self.scroll = ffi.cast("ViewScroll*", self.native:get_scroll_ptr())
    self._scrollable_ptr = ffi.cast("bool*", self.native:get_scrollable_ptr())
    self.scrollable = self._scrollable_ptr[0]
    self.cursor = self.native:get_cursor()
    self.context = self.native:get_context()
end

function View:set_scrollable(val)
    self._scrollable_ptr[0] = val
    self.scrollable = val
end

function View:__tostring()
    return tostring(self.native)
end

function View:get_name()
    return self.native:get_name()
end

function View:get_scrollable_size()
    return self.native:get_scrollable_size()
end

function View:get_h_scrollable_size()
    return self.native:get_h_scrollable_size()
end

function View:supports_text_input()
    return self.native:supports_text_input()
end

function View:scrollbar_overlaps_point(x, y)
    return self.native:scrollbar_overlaps_point(x, y)
end

function View:scrollbar_dragging()
    return self.native:scrollbar_dragging()
end

function View:scrollbar_hovering()
    return self.native:scrollbar_hovering()
end

function View:on_mouse_pressed(button, x, y, clicks)
    return self.native:on_mouse_pressed(button, x, y, clicks)
end

function View:on_mouse_released(button, x, y)
    self.native:on_mouse_released(button, x, y)
end

function View:on_mouse_moved(x, y, dx, dy)
    return self.native:on_mouse_moved(x, y, dx, dy)
end

function View:on_mouse_left()
    self.native:on_mouse_left()
end

function View:on_mouse_wheel(y, x)
    return self.native:on_mouse_wheel(y, x)
end

function View:on_file_dropped(filename, x, y)
    return self.native:on_file_dropped(filename, x, y)
end

function View:on_text_input(text)
    self.native:on_text_input(text)
end

function View:on_ime_text_editing(text, start, length)
    -- TODO: Implement in C++
end

function View:on_scale_change(new_scale, prev_scale)
    self.native:on_scale_change(new_scale, prev_scale)
end

function View:on_touch_moved(x, y, dx, dy, i)
    -- TODO: Implement in C++
end

function View:get_content_bounds()
    return self.native:get_content_bounds()
end

function View:get_content_offset()
    return self.native:get_content_offset()
end

function View:clamp_scroll_position()
    self.native:clamp_scroll_position()
end

function View:update()
    self.native:update()
    self.scrollable = self._scrollable_ptr[0]
    self.cursor = self.native:get_cursor()
end

function View:draw(surface)
end

function View:draw_background(color)
    local renderer = require "renderer"
    local x, y = self.position.x, self.position.y
    local w, h = self.size.x, self.size.y
    renderer.draw_rect(x, y, w, h, color)
end

function View:draw_scrollbar()
    if not self.scrollable then
        return
    end

    -- TODO: Implement Lua-side scrollbar drawing using native scrollbar state
    -- For now, keep it as no-op since we're still using Lua scrollbar from parent
end

function View:try_close(do_close)
    if do_close then
        do_close()
    end
end

function View:on_context_menu(x, y)
    return nil
end

-- Property delegation using metatable
local function setup_property_delegation(view)
    -- Note: Property access needs special handling
    -- For now, users should access native.position, native.size, etc.
    -- or we can add property getter/setter methods
end

-- Implement move_towards as a helper that calls native
-- ensures compatibility with existing code
function View:move_towards(t, k, dest, rate, name)
    if type(t) ~= "table" then
        return self:move_towards(self, t, k, dest, rate, name)
    end

    -- For native scroll animation, use native implementation
    -- For other animations, keep this implementation for now
    local val = t[k]
    local diff = math.abs(val - dest)
    local config = require "core.config"
    local common = require "core.common"

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
        local core = require "core"
        core.redraw = true
    end
end

View.context = "application"

return View
