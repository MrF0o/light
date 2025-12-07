local core = require "core"
local common = require "core.common"
local style = require "core.style"
local View = require "core.view_native"
local ContextMenu = require "core.contextmenu"
local command = require "core.command"
local clay = require "clay"

-- Subclass ContextMenu to allow pass-through events
local TitleBarMenu = ContextMenu:extend()

function TitleBarMenu:on_mouse_moved(px, py)
    if not self.visible then
        return false
    end

    -- Check if mouse is inside the menu rect
    local inside =
        px >= self.position.x and px <= self.position.x + self.items.width and py >= self.position.y and py <=
            self.position.y + self.height

    if not inside then
        return false
    end

    return TitleBarMenu.super.on_mouse_moved(self, px, py)
end

function TitleBarMenu:on_mouse_pressed(button, x, y, clicks)
    if not self.visible then
        return false
    end
    local inside = x >= self.position.x and x < self.position.x + self.items.width and y >= self.position.y and y <
                       self.position.y + self.height

    if inside then
        return TitleBarMenu.super.on_mouse_pressed(self, button, x, y, clicks)
    end

    -- Clicked outside
    self:hide()
    return false -- Let parent handle it (e.g. checking if clicked on another title button)
end

local restore_command = {
    symbol = "w",
    action = function()
        system.set_window_mode(core.window, "normal")
    end
}

local maximize_command = {
    symbol = "W",
    action = function()
        system.set_window_mode(core.window, "maximized")
    end
}

local title_commands = {{
    symbol = "_",
    action = function()
        system.set_window_mode(core.window, "minimized")
    end
}, maximize_command, {
    symbol = "X",
    action = function()
        core.quit()
    end
}}

local TitleBar = View:extend()

function TitleBar:new()
    TitleBar.super.new(self)
    self.size.y = 35 * SCALE -- Adjust height as needed
    self.menu = TitleBarMenu()
    self.hovered_item = nil
    self.hovered_control = nil
    self.open_menu_name = nil
    self.mouse_down = false

    self.items = {{
        text = "File"
    }, {
        text = "Edit"
    }, {
        text = "View"
    }}

    -- Define the menu structure
    self.menus = {
        File = {{
            text = "New File",
            command = "core:new-doc"
        }, {
            text = "Open File",
            command = "core:open-file"
        }, {
            text = "Open Folder",
            command = "core:open-project-folder"
        }, ContextMenu.DIVIDER, {
            text = "Save",
            command = "doc:save"
        }, {
            text = "Exit",
            command = "core:quit"
        }},
        Edit = {{
            text = "Undo",
            command = "doc:undo"
        }, {
            text = "Redo",
            command = "doc:redo"
        }, ContextMenu.DIVIDER, {
            text = "Cut",
            command = "doc:cut"
        }, {
            text = "Copy",
            command = "doc:copy"
        }, {
            text = "Paste",
            command = "doc:paste"
        }, ContextMenu.DIVIDER, {
            text = "Find",
            command = "find-replace:find"
        }},
        View = {{
            text = "Toggle Log",
            command = "core:toggle-log"
        }, {
            text = "Toggle Sidebar",
            command = "core:toggle-sidebar"
        }}
    }

    self.clay_initialized = false
end

function TitleBar:get_item_rect(index)
    local id = clay.id("TitleBarItem" .. index)
    local data = clay.get_element_data(id)
    if data then
        return data.x, data.y, data.width, data.height
    end
    return 0, 0, 0, 0
end

function TitleBar:get_item_at(x, y)
    -- Not used anymore, we use clay.pointer_over
    return nil
end

function TitleBar:update()
    self.menu:update()
    title_commands[2] = core.window_mode == "maximized" and restore_command or maximize_command
end

function TitleBar:draw()
    local w, h = system.get_window_size(core.window)
    if not self.clay_initialized then
        clay.initialize(w, h)
        self.clay_initialized = true
    end
    clay.set_dimensions(w, h)

    clay.begin_layout()

    -- Root element
    clay.open_element()
    clay.configure_element({
        layout = {
            sizing = {
                width = {
                    type = "fixed",
                    value = self.size.x
                },
                height = {
                    type = "fixed",
                    value = self.size.y
                }
            },
            layoutDirection = "leftToRight",
            childAlignment = {
                y = "center"
            },
            padding = {
                x = 10 * SCALE
            }
        },
        backgroundColor = {style.background2[1], style.background2[2], style.background2[3], 255}
    })

    -- Items
    for i, item in ipairs(self.items) do
        local id = clay.id("TitleBarItem" .. i)
        clay.open_element()

        local hovered = clay.pointer_over(id)
        local bg_color = {0, 0, 0, 0}
        if hovered or self.open_menu_name == item.text then
            bg_color = {style.selection[1], style.selection[2], style.selection[3], 255}
        end

        clay.configure_element({
            id = id,
            layout = {
                padding = {
                    x = 10 * SCALE,
                    y = 5 * SCALE
                }
            },
            backgroundColor = bg_color
        })

        clay.text(item.text, {
            font = style.font,
            fontSize = style.font:get_size(),
            color = {style.text[1], style.text[2], style.text[3], 255}
        })

        clay.close_element()
    end

    -- Spacer
    clay.open_element()
    clay.configure_element({
        layout = {
            sizing = {
                width = {
                    type = "grow"
                }
            }
        }
    })
    clay.close_element()

    -- Controls
    if self.borderless then
        for i, item in ipairs(title_commands) do
            local id = clay.id("TitleBarControl" .. i)
            clay.open_element()

            local hovered = clay.pointer_over(id)
            local color = hovered and style.text or style.dim

            clay.configure_element({
                id = id,
                layout = {
                    padding = {
                        x = 10 * SCALE
                    }
                }
            })

            clay.text(item.symbol, {
                font = style.icon_font,
                fontSize = style.icon_font:get_size(),
                color = {color[1], color[2], color[3], 255}
            })

            clay.close_element()
        end
    end

    clay.close_element() -- Root

    clay.render(core.window)

    self.menu:draw()
end

function TitleBar:on_mouse_moved(x, y, dx, dy)
    if self.dragging then
        local w, h, wx, wy = system.get_window_size(core.window)
        system.set_window_size(core.window, w, h, wx + dx, wy + dy)
        return true
    end

    clay.set_pointer(x, y, self.mouse_down)

    if self.menu.visible then
        if self.menu:on_mouse_moved(x, y, dx, dy) then
            return true
        end
    end

    -- Check items hover
    self.hovered_item = nil
    for i, item in ipairs(self.items) do
        local id = clay.id("TitleBarItem" .. i)
        if clay.pointer_over(id) then
            self.hovered_item = i
            break
        end
    end

    -- Check controls hover
    self.hovered_control = nil
    if self.borderless then
        for i, item in ipairs(title_commands) do
            local id = clay.id("TitleBarControl" .. i)
            if clay.pointer_over(id) then
                self.hovered_control = item
                break
            end
        end
    end

    if self.open_menu_name and self.hovered_item then
        local item = self.items[self.hovered_item]
        if item.text ~= self.open_menu_name then
            self:open_menu(self.hovered_item)
        end
    end

    if self.hovered_item or self.hovered_control then
        core.redraw = true
        return true
    end
    return false
end

function TitleBar:on_mouse_pressed(button, x, y, clicks)
    self.mouse_down = true
    clay.set_pointer(x, y, true)

    if self.menu.visible then
        if self.menu:on_mouse_pressed(button, x, y, clicks) then
            if not self.menu.visible then
                self.open_menu_name = nil
            else
                return true
            end
        end
    end

    if y < self.position.y or y >= self.position.y + self.size.y or x < self.position.x or x >= self.position.x +
        self.size.x then
        return false
    end

    if self.borderless and self.hovered_control then
        self.hovered_control.action()
        return true
    end

    if self.hovered_item then
        local item_idx = self.hovered_item
        if self.open_menu_name == self.items[item_idx].text then
            self.menu:hide()
            self.open_menu_name = nil
        else
            self:open_menu(item_idx)
        end
        return true
    end

    if button == "left" then
        if clicks == 2 then
            local mode = system.get_window_mode(core.window)
            system.set_window_mode(core.window, mode == "maximized" and "normal" or "maximized")
        else
            self.dragging = true
        end
        return true
    end

    return false
end

function TitleBar:configure_hit_test(borderless)
    self.borderless = borderless
    if borderless then
        local icon_w = style.icon_font:get_width("_")
        local icon_spacing = icon_w
        local controls_width = (icon_w + icon_spacing) * #title_commands + icon_spacing
        system.set_window_hit_test(core.window, 0, controls_width, icon_spacing)
    else
        system.set_window_hit_test(core.window)
    end
end

function TitleBar:open_menu(index)
    local item = self.items[index]
    local menu_items = self.menus[item.text]
    if not menu_items then
        return
    end

    local x, y, w, h = self:get_item_rect(index)
    self.menu:show(x, y + h, menu_items)
    self.open_menu_name = item.text
end

function TitleBar:on_mouse_left()
    self.hovered_item = nil
end

function TitleBar:on_mouse_released(button, x, y)
    self.mouse_down = false
    clay.set_pointer(x, y, false)

    self.dragging = false
    if self.menu.visible then
        self.menu:on_mouse_released(button, x, y)
    end
end

return TitleBar
