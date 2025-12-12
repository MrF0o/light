#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_set>
#include <cstring>
#include "api/api.h"

#define CONFIG_STRING_MAX 256
#define CONFIG_PATTERN_MAX 512

enum ScrollbarStatus {
    SCROLLBAR_AUTO = 0,      // false - expands on hover
    SCROLLBAR_EXPANDED = 1,  // "expanded"
    SCROLLBAR_CONTRACTED = 2 // "contracted"
};

enum HighlightLineType {
    HIGHLIGHT_ALWAYS = 0,       // true
    HIGHLIGHT_NEVER = 1,        // false
    HIGHLIGHT_NO_SELECTION = 2  // "no_selection"
};

enum TabType {
    TAB_SOFT = 0, // spaces
    TAB_HARD = 1  // tabs
};

enum LineEndings {
    LINE_ENDINGS_LF = 0,
    LINE_ENDINGS_CRLF = 1
};

struct DisabledTransitions {
    bool scroll = false;
    bool commandview = false;
    bool contextmenu = false;
    bool logview = false;
    bool nagbar = false;
    bool tabs = false;
    bool tab_drag = false;
    bool statusbar = false;
};

struct Config {
    double fps = 60.0;
    double animation_rate = 1.0;
    double blink_period = 0.8;
    bool transitions = true;
    bool animate_drag_scroll = false;
    bool disable_blink = false;
    float scale = 1.0f;

    int max_log_items = 800;
    double message_timeout = 5.0;
    double mouse_wheel_scroll = 50.0; // Will be multiplied by SCALE in Lua
    double file_size_limit = 10.0;
    double undo_merge_timeout = 0.3;
    int max_undos = 10000;
    int max_tabs = 8;
    int max_visible_commands = 10;
    double line_height = 1.2;
    int indent_size = 2;
    int line_limit = 80;
    int max_clicks = 3;

    bool scroll_past_end = true;
    bool always_show_tabs = true;
    bool keep_newline_whitespace = false;
    bool draw_whitespace = false; // deprecated
    bool borderless = false;
    bool tab_close_button = true;
    bool skip_plugins_version = false;
    bool stonks = true;
    bool use_system_file_picker = false; // Will be set based on sandbox detection

    ScrollbarStatus force_scrollbar_status = SCROLLBAR_AUTO;
    HighlightLineType highlight_current_line = HIGHLIGHT_ALWAYS;
    TabType tab_type = TAB_SOFT;
    LineEndings line_endings = LINE_ENDINGS_LF;

    char symbol_pattern[CONFIG_PATTERN_MAX] = "[%a_][%w_]*";
    char non_word_chars[CONFIG_STRING_MAX] = " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-";

    DisabledTransitions disabled_transitions;
};

class ConfigManager {
public:
    static Config& instance() {
        static Config config;
        return config;
    }

    static bool isTransitionDisabled(const std::string& name);
    static void setTransitionDisabled(const std::string& name, bool disabled);

    // Helper to get disabled transition by name
    static bool* getTransitionPtr(const std::string& name);
};

extern "C" {
    API_EXPORT Config* api_get_native_config();
    API_EXPORT void api_set_transition_disabled(const char* name, bool disabled);
    API_EXPORT bool api_get_transition_disabled(const char* name);
    API_EXPORT void api_set_config_string(char* dest, const char* src, int max_len);
    API_EXPORT DisabledTransitions* api_get_disabled_transitions();
}

#endif // CONFIG_HPP
