-- mod-version:4.0.0
local core = require "core"
local command = require "core.command"
local clay = require "clay"
local renwindow = require "renwindow"
local style = require "core.style"

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
            sizing = {
                width = {
                    type = "grow",
                    min = 0,
                    max = 0
                },
                height = {
                    type = "grow",
                    min = 0,
                    max = 0
                }
            },
            padding = {
                x = 16,
                y = 16
            },
            childGap = 16,
            layoutDirection = "topToBottom"
        },
        backgroundColor = {style.background[1], style.background[2], style.background[3], 255}
    })

    -- Menu Bar
    clay.open_element()
    clay.configure_element({
        layout = {
            sizing = {
                width = {
                    type = "grow",
                    min = 0,
                    max = 0
                },
                height = {
                    type = "fixed",
                    value = 40
                }
            },
            padding = {
                x = 8,
                y = 8
            },
            childGap = 32,
            layoutDirection = "leftToRight",
            childAlignment = {
                y = "center"
            }
        },
        backgroundColor = {style.background2[1], style.background2[2], style.background2[3], 255},
        cornerRadius = 8
    })
    clay.text("Menu Item 1", {
        fontSize = style.font:get_size(),
        color = {style.text[1], style.text[2], style.text[3], 255},
        font = style.font
    })
    clay.text("Menu Item 2", {
        fontSize = style.font:get_size(),
        color = {style.text[1], style.text[2], style.text[3], 255},
        font = style.font
    })
    clay.close_element()
    -- End Menu Bar
    -- Button
    clay.open_element()
    clay.configure_element({
        layout = {
            sizing = {
                width = {
                    type = "fixed",
                    value = 120
                },
                height = {
                    type = "fixed",
                    value = 40
                }
            },
            padding = {
                x = 16,
                y = 8
            },
            childAlignment = {
                x = "center",
                y = "center"
            }
        },
        backgroundColor = {style.accent[1], style.accent[2], style.accent[3], 255},
        cornerRadius = 8
    })
    clay.text("Click Me", {
        fontSize = style.font:get_size(),
        color = {255, 255, 255, 255},
        font = style.font
    })
    clay.close_element()
    -- End Button
    
    clay.close_element()
    -- End Root

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
                coroutine.yield(1 / 30) -- 30 FPS
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
    end
})

