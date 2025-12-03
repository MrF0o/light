local core = require "core"
local common = require "core.common"
local style = require "core.style"
local View = require "core.view"
local ContextMenu = require "core.contextmenu"
local command = require "core.command"

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

local core = require "core"
local common = require "core.common"
local style = require "core.style"
local View = require "core.view"
local ContextMenu = require "core.contextmenu"
local command = require "core.command"

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
end

function TitleBar:get_item_rect(index)
    local x = self.position.x + 10 * SCALE
    for i = 1, index - 1 do
        x = x + style.font:get_width(self.items[i].text) + 20 * SCALE
    end
    local w = style.font:get_width(self.items[index].text) + 20 * SCALE
    return x, self.position.y, w, self.size.y
end

function TitleBar:get_item_at(x, y)
    local dy = y - self.position.y
    if dy < 0 or dy > self.size.y then
        return nil
    end

    local cx = self.position.x + 10 * SCALE
    for i, item in ipairs(self.items) do
        local w = style.font:get_width(item.text) + 20 * SCALE
        if x >= cx and x < cx + w then
            return i
        end
        cx = cx + w
    end
    return nil
end

function TitleBar:update()
    self.menu:update()
    title_commands[2] = core.window_mode == "maximized" and restore_command or maximize_command
end

function TitleBar:each_control_item()
    local icon_h, icon_w = style.icon_font:get_height(), style.icon_font:get_width("_")
    local icon_spacing = icon_w
    local ox, oy = self.position.x, self.position.y
    ox = ox + self.size.x
    local i, n = 0, #title_commands
    local iter = function()
        i = i + 1
        if i <= n then
            local dx = -(icon_w + icon_spacing) * (n - i + 1)
            local dy = (self.size.y - icon_h) / 2
            return title_commands[i], ox + dx, oy + dy, icon_w, icon_h
        end
    end
    return iter
end

function TitleBar:draw_window_title()
    local title = core.compose_window_title(core.window_title)
    common.draw_text(style.font, style.text, title, "center", self.position.x, self.position.y, self.size.x, self.size.y)
end

function TitleBar:draw_window_controls()
    for item, x, y, w, h in self:each_control_item() do
        local color = item == self.hovered_control and style.text or style.dim
        common.draw_text(style.icon_font, color, item.symbol, nil, x, y, 0, h)
    end
end

function TitleBar:draw()
    -- Draw background
    renderer.draw_rect(self.position.x, self.position.y, self.size.x, self.size.y, style.background2)

    if self.borderless then
        -- Draw Title
        self:draw_window_title()

        -- Draw Controls
        self:draw_window_controls()
    end

    -- Draw items
    local x = self.position.x + 10 * SCALE
    for i, item in ipairs(self.items) do
        local w = style.font:get_width(item.text) + 20 * SCALE

        -- Draw hover/active background
        if i == self.hovered_item or self.open_menu_name == item.text then
            renderer.draw_rect(x, self.position.y, w, self.size.y, style.selection)
        end

        common.draw_text(style.font, style.text, item.text, "center", x, self.position.y, w, self.size.y)
        x = x + w
    end

    self.menu:draw()
end

function TitleBar:on_mouse_moved(x, y, dx, dy)
    if self.dragging then
        local w, h, wx, wy = system.get_window_size(core.window)
        system.set_window_size(core.window, w, h, wx + dx, wy + dy)
        return true
    end

    -- If menu is open, try to handle it first
    if self.menu.visible then
        if self.menu:on_mouse_moved(x, y, dx, dy) then
            return true
        end
    end

    -- Check controls hover
    self.hovered_control = nil
    if self.borderless then
        for item, cx, cy, cw, ch in self:each_control_item() do
            if x >= cx and x < cx + cw and y >= cy and y < cy + ch then
                self.hovered_control = item
                core.redraw = true
                return true
            end
        end
    end

    local prev_hover = self.hovered_item
    self.hovered_item = self:get_item_at(x, y)

    if prev_hover ~= self.hovered_item then
        core.redraw = true
    end

    -- If a menu is open and we hover another titlebar item, switch menu
    if self.open_menu_name and self.hovered_item then
        local item = self.items[self.hovered_item]
        if item.text ~= self.open_menu_name then
            self:open_menu(self.hovered_item)
        end
    end

    if self.hovered_item or self.hovered_control then
        return true
    end
    return false
end

function TitleBar:on_mouse_pressed(button, x, y, clicks)
    if self.menu.visible then
        if self.menu:on_mouse_pressed(button, x, y, clicks) then
            -- Menu handled it (clicked item or outside)
            -- If clicked outside, menu hides. We should check if we clicked a titlebar item.
            if not self.menu.visible then
                self.open_menu_name = nil
                -- Fallthrough to check titlebar click
            else
                return true
            end
        end
    end

    -- Check if click is within titlebar bounds
    if y < self.position.y or y >= self.position.y + self.size.y or x < self.position.x or x >= self.position.x +
        self.size.x then
        return false
    end

    -- Check controls click
    if self.borderless and self.hovered_control then
        self.hovered_control.action()
        return true
    end

    local item_idx = self:get_item_at(x, y)
    if item_idx then
        if self.open_menu_name == self.items[item_idx].text then
            self.menu:hide()
            self.open_menu_name = nil
        else
            self:open_menu(item_idx)
        end
        return true
    end

    -- Start dragging if clicked on empty space
    if button == "left" then
        if clicks == 2 then
            -- Toggle maximize
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
        -- Disable native title bar hit test to allow custom menu interaction
        -- We pass 0 for title_height but keep the resize border
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
    self.dragging = false
    if self.menu.visible then
        self.menu:on_mouse_released(button, x, y)
    end
end

return TitleBar
