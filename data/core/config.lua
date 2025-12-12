local common = require "core.common"
local ffi = require "ffi"

ffi.cdef [[
  typedef enum {
    SCROLLBAR_AUTO = 0,
    SCROLLBAR_EXPANDED = 1,
    SCROLLBAR_CONTRACTED = 2
  } ScrollbarStatus;

  typedef enum {
    HIGHLIGHT_ALWAYS = 0,
    HIGHLIGHT_NEVER = 1,
    HIGHLIGHT_NO_SELECTION = 2
  } HighlightLineType;

  typedef enum {
    TAB_SOFT = 0,
    TAB_HARD = 1
  } TabType;

  typedef enum {
    LINE_ENDINGS_LF = 0,
    LINE_ENDINGS_CRLF = 1
  } LineEndings;

  typedef struct {
    bool scroll;
    bool commandview;
    bool contextmenu;
    bool logview;
    bool nagbar;
    bool tabs;
    bool tab_drag;
    bool statusbar;
  } DisabledTransitions;

  typedef struct {
    double fps;
    double animation_rate;
    double blink_period;
    bool transitions;
    bool animate_drag_scroll;
    bool disable_blink;
    float scale;

    int max_log_items;
    double message_timeout;
    double mouse_wheel_scroll;
    double file_size_limit;
    double undo_merge_timeout;
    int max_undos;
    int max_tabs;
    int max_visible_commands;
    double line_height;
    int indent_size;
    int line_limit;
    int max_clicks;

    bool scroll_past_end;
    bool always_show_tabs;
    bool keep_newline_whitespace;
    bool draw_whitespace;
    bool borderless;
    bool tab_close_button;
    bool skip_plugins_version;
    bool stonks;
    bool use_system_file_picker;

    ScrollbarStatus force_scrollbar_status;
    HighlightLineType highlight_current_line;
    TabType tab_type;
    LineEndings line_endings;

    char symbol_pattern[512];
    char non_word_chars[256];

    DisabledTransitions disabled_transitions;
  } Config;

  Config* api_get_native_config();
  void api_set_transition_disabled(const char* name, bool disabled);
  bool api_get_transition_disabled(const char* name);
  void api_set_config_string(char* dest, const char* src, int max_len);
  DisabledTransitions* api_get_disabled_transitions();
]]

local native_config = ffi.C.api_get_native_config()

if rawget(_G, "SCALE") then
    native_config.scale = SCALE
    native_config.mouse_wheel_scroll = 50 * SCALE
else
    native_config.mouse_wheel_scroll = 50
end

native_config.line_endings = (PLATFORM == "Windows") and ffi.C.LINE_ENDINGS_CRLF or ffi.C.LINE_ENDINGS_LF
native_config.use_system_file_picker = system.get_sandbox() ~= "none"

local scrollbar_to_enum = {
    [false] = ffi.C.SCROLLBAR_AUTO,
    ["expanded"] = ffi.C.SCROLLBAR_EXPANDED,
    ["contracted"] = ffi.C.SCROLLBAR_CONTRACTED
}
local enum_to_scrollbar = {
    [tonumber(ffi.C.SCROLLBAR_AUTO)] = false,
    [tonumber(ffi.C.SCROLLBAR_EXPANDED)] = "expanded",
    [tonumber(ffi.C.SCROLLBAR_CONTRACTED)] = "contracted"
}

local highlight_to_enum = {
    [true] = ffi.C.HIGHLIGHT_ALWAYS,
    [false] = ffi.C.HIGHLIGHT_NEVER,
    ["no_selection"] = ffi.C.HIGHLIGHT_NO_SELECTION
}
local enum_to_highlight = {
    [tonumber(ffi.C.HIGHLIGHT_ALWAYS)] = true,
    [tonumber(ffi.C.HIGHLIGHT_NEVER)] = false,
    [tonumber(ffi.C.HIGHLIGHT_NO_SELECTION)] = "no_selection"
}

local tab_type_to_enum = {
    ["soft"] = ffi.C.TAB_SOFT,
    ["hard"] = ffi.C.TAB_HARD
}
local enum_to_tab_type = {
    [tonumber(ffi.C.TAB_SOFT)] = "soft",
    [tonumber(ffi.C.TAB_HARD)] = "hard"
}

local line_endings_to_enum = {
    ["lf"] = ffi.C.LINE_ENDINGS_LF,
    ["crlf"] = ffi.C.LINE_ENDINGS_CRLF
}
local enum_to_line_endings = {
    [tonumber(ffi.C.LINE_ENDINGS_LF)] = "lf",
    [tonumber(ffi.C.LINE_ENDINGS_CRLF)] = "crlf"
}

local transition_names = {"scroll", "commandview", "contextmenu", "logview", "nagbar", "tabs", "tab_drag", "statusbar"}

local disabled_transitions_proxy = setmetatable({}, {
    __index = function(t, k)
        local dt = native_config.disabled_transitions
        if k == "scroll" then
            return dt.scroll
        end
        if k == "commandview" then
            return dt.commandview
        end
        if k == "contextmenu" then
            return dt.contextmenu
        end
        if k == "logview" then
            return dt.logview
        end
        if k == "nagbar" then
            return dt.nagbar
        end
        if k == "tabs" then
            return dt.tabs
        end
        if k == "tab_drag" then
            return dt.tab_drag
        end
        if k == "statusbar" then
            return dt.statusbar
        end
        return nil
    end,
    __newindex = function(t, k, v)
        ffi.C.api_set_transition_disabled(k, v)
    end,
    __pairs = function()
        return coroutine.wrap(function()
            local dt = native_config.disabled_transitions
            coroutine.yield("scroll", dt.scroll)
            coroutine.yield("commandview", dt.commandview)
            coroutine.yield("contextmenu", dt.contextmenu)
            coroutine.yield("logview", dt.logview)
            coroutine.yield("nagbar", dt.nagbar)
            coroutine.yield("tabs", dt.tabs)
            coroutine.yield("tab_drag", dt.tab_drag)
            coroutine.yield("statusbar", dt.statusbar)
        end)
    end
})

-- Lua-only
local config_impl = {
    ---A list of files and directories to ignore.
    ignore_files = { -- folders
    "^%.svn/", "^%.git/", "^%.hg/", "^CVS/", "^%.Trash/", "^%.Trash%-.*/", "^node_modules/", "^%.cache/",
    "^__pycache__/", -- files
    "%.pyc$", "%.pyo$", "%.exe$", "%.dll$", "%.obj$", "%.o$", "%.a$", "%.lib$", "%.so$", "%.dylib$", "%.ncb$", "%.sdf$",
    "%.suo$", "%.pdb$", "%.idb$", "%.class$", "%.psd$", "%.db$", "^desktop%.ini$", "^%.DS_Store$", "^%.directory$"}
}

local native_fields = {
    fps = true,
    animation_rate = true,
    blink_period = true,
    transitions = true,
    animate_drag_scroll = true,
    disable_blink = true,
    scale = true,
    max_log_items = true,
    message_timeout = true,
    mouse_wheel_scroll = true,
    file_size_limit = true,
    undo_merge_timeout = true,
    max_undos = true,
    max_tabs = true,
    max_visible_commands = true,
    line_height = true,
    indent_size = true,
    line_limit = true,
    max_clicks = true,
    scroll_past_end = true,
    always_show_tabs = true,
    keep_newline_whitespace = true,
    draw_whitespace = true,
    borderless = true,
    tab_close_button = true,
    skip_plugins_version = true,
    stonks = true,
    use_system_file_picker = true
}

-- Main config metatable
local config = setmetatable({}, {
    __index = function(t, k)
        if native_fields[k] then
            return native_config[k]
        end

        if k == "force_scrollbar_status" then
            return enum_to_scrollbar[tonumber(native_config.force_scrollbar_status)]
        end
        if k == "highlight_current_line" then
            return enum_to_highlight[tonumber(native_config.highlight_current_line)]
        end
        if k == "tab_type" then
            return enum_to_tab_type[tonumber(native_config.tab_type)]
        end
        if k == "line_endings" then
            return enum_to_line_endings[tonumber(native_config.line_endings)]
        end

        if k == "symbol_pattern" then
            return ffi.string(native_config.symbol_pattern)
        end
        if k == "non_word_chars" then
            return ffi.string(native_config.non_word_chars)
        end

        if k == "disabled_transitions" then
            return disabled_transitions_proxy
        end

        -- Lua-only fields
        return config_impl[k]
    end,

    __newindex = function(t, k, v)
        if native_fields[k] then
            native_config[k] = v
            return
        end

        if k == "force_scrollbar_status" then
            native_config.force_scrollbar_status = scrollbar_to_enum[v] or ffi.C.SCROLLBAR_AUTO
            return
        end
        if k == "highlight_current_line" then
            native_config.highlight_current_line = highlight_to_enum[v] or ffi.C.HIGHLIGHT_ALWAYS
            return
        end
        if k == "tab_type" then
            native_config.tab_type = tab_type_to_enum[v] or ffi.C.TAB_SOFT
            return
        end
        if k == "line_endings" then
            native_config.line_endings = line_endings_to_enum[v] or ffi.C.LINE_ENDINGS_LF
            return
        end

        if k == "symbol_pattern" then
            ffi.C.api_set_config_string(native_config.symbol_pattern, v, 512)
            return
        end
        if k == "non_word_chars" then
            ffi.C.api_set_config_string(native_config.non_word_chars, v, 256)
            return
        end

        if k == "disabled_transitions" then
            if type(v) == "table" then
                for dk, dv in pairs(v) do
                    disabled_transitions_proxy[dk] = dv
                end
            end
            return
        end

        config_impl[k] = v
    end,

    __pairs = function()
        return coroutine.wrap(function()
            for k in pairs(native_fields) do
                coroutine.yield(k, native_config[k])
            end
            coroutine.yield("force_scrollbar_status", enum_to_scrollbar[tonumber(native_config.force_scrollbar_status)])
            coroutine.yield("highlight_current_line", enum_to_highlight[tonumber(native_config.highlight_current_line)])
            coroutine.yield("tab_type", enum_to_tab_type[tonumber(native_config.tab_type)])
            coroutine.yield("line_endings", enum_to_line_endings[tonumber(native_config.line_endings)])
            coroutine.yield("symbol_pattern", ffi.string(native_config.symbol_pattern))
            coroutine.yield("non_word_chars", ffi.string(native_config.non_word_chars))
            coroutine.yield("disabled_transitions", disabled_transitions_proxy)
            for k, v in pairs(config_impl) do
                coroutine.yield(k, v)
            end
        end)
    end
})

-- holds the plugins real config table
local plugins_config = {}

---A table containing configuration for all the plugins.
---
---This is a metatable that automaticaly creates a minimal
---configuration when a plugin is initially configured.
---Each plugins will then call `common.merge()` to get the finalized
---plugin config.
---Do not use raw operations on this table.
---@type table
config_impl.plugins = setmetatable({}, {
    __index = function(_, k)
        if not plugins_config[k] then
            plugins_config[k] = {
                enabled = true,
                config = {}
            }
        end
        if plugins_config[k].enabled ~= false then
            return plugins_config[k].config
        end
        return false
    end,
    __newindex = function(_, k, v)
        if not plugins_config[k] then
            plugins_config[k] = {
                enabled = nil,
                config = {}
            }
        end
        if v == false and package.loaded["plugins." .. k] then
            local core = require "core"
            core.warn("[%s] is already enabled, restart the editor for the change to take effect", k)
            return
        elseif plugins_config[k].enabled == false and v ~= false then
            plugins_config[k].enabled = true
        end
        if v == false then
            plugins_config[k].enabled = false
        elseif type(v) == "table" then
            plugins_config[k].enabled = true
            plugins_config[k].config = common.merge(plugins_config[k].config, v)
        end
    end,
    __pairs = function()
        return coroutine.wrap(function()
            for name, status in pairs(plugins_config) do
                coroutine.yield(name, status.config)
            end
        end)
    end
})

return config
