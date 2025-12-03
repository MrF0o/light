-- mod-version:4.0.0
local core = require "core"
local command = require "core.command"
local clay = require "clay"
local renwindow = require "renwindow"

local clay_debug_window = nil
local clay_initialized = false
local clay_render_thread = nil
local should_stop_rendering = false

local function render_clay_ui()
  if not clay_debug_window or should_stop_rendering then
    return
  end
  
  local w, h = renwindow.get_size(clay_debug_window)
  clay.set_dimensions(w, h)
  
  local mx, my = 0, 0
  local mouse_down = false
  clay.set_pointer(mx, my, mouse_down)
  
  clay.begin_layout()
  
  -- Root
  clay.open_element()
  clay.configure_element({
    layout = {
      width = w,
      height = h,
      padding = 50
    },
    backgroundColor = {30, 30, 35, 255}
  })
  
    clay.open_element()
    clay.configure_element({
      layout = {
        width = 200,
        height = 150
      },
      backgroundColor = {220, 50, 50, 255},
      cornerRadius = 20
    })
    clay.close_element()
    
    clay.open_element()
    clay.configure_element({
      layout = {
        width = 200,
        height = 150
      },
      backgroundColor = {50, 200, 80, 255},
      border = {
        color = {255, 255, 255, 255},
        width = {left = 4, right = 4, top = 4, bottom = 4}
      }
    })
    clay.close_element()
    
    clay.open_element()
    clay.configure_element({
      layout = {
        width = 200,
        height = 150
      },
      backgroundColor = {50, 120, 220, 255},
      cornerRadius = 25,
      border = {
        color = {255, 200, 0, 255},
        width = {left = 6, right = 6, top = 6, bottom = 6},
        cornerRadius = 25
      }
    })
    clay.close_element()
  
  clay.close_element() -- End Root
  
  -- DON'T call end_layout here - let render() do it

  local success, err = pcall(function()
    clay.render(clay_debug_window)
  end)
  if not success then
    core.error("Error rendering Clay UI: " .. tostring(err))
  end
end

command.add(nil, {
  ["clay:open-debug-window"] = function()
    if clay_debug_window then
      return
    end
    
    should_stop_rendering = false
    
    clay_debug_window = renwindow.create("Clay UI Debug Window", 800, 600)
    
    if not clay_debug_window then
      core.error("Failed to create debug window")
      return
    end
    
    renwindow.show(clay_debug_window)
    
    if not clay_initialized then
      clay.initialize(800, 600)
      clay_initialized = true
    end
    
    render_clay_ui()
    
    clay_render_thread = core.add_thread(function()
      while clay_debug_window and not should_stop_rendering do
        render_clay_ui()
        coroutine.yield(1/30) -- 30 FPS
      end
    end)
    
    core.log("debug window opened")
  end,
  
  ["clay:close-debug-window"] = function()
    if clay_debug_window then
      should_stop_rendering = true
      
      if clay_debug_window.destroy then
        clay_debug_window:destroy()
      end
      
      clay_debug_window = nil
      clay_render_thread = nil
      
      core.log("debug window closed")
    else
      core.log("No debug window to close")
    end
  end,
})

