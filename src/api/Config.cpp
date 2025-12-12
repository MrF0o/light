#include "Config.hpp"
#include <cstring>
#include <algorithm>

// Helper to get pointer to disabled transition by name
bool* ConfigManager::getTransitionPtr(const std::string& name) {
    Config& cfg = instance();
    if (name == "scroll") return &cfg.disabled_transitions.scroll;
    if (name == "commandview") return &cfg.disabled_transitions.commandview;
    if (name == "contextmenu") return &cfg.disabled_transitions.contextmenu;
    if (name == "logview") return &cfg.disabled_transitions.logview;
    if (name == "nagbar") return &cfg.disabled_transitions.nagbar;
    if (name == "tabs") return &cfg.disabled_transitions.tabs;
    if (name == "tab_drag") return &cfg.disabled_transitions.tab_drag;
    if (name == "statusbar") return &cfg.disabled_transitions.statusbar;
    return nullptr;
}

bool ConfigManager::isTransitionDisabled(const std::string& name) {
    bool* ptr = getTransitionPtr(name);
    return ptr ? *ptr : false;
}

void ConfigManager::setTransitionDisabled(const std::string& name, bool disabled) {
    bool* ptr = getTransitionPtr(name);
    if (ptr) {
        *ptr = disabled;
    }
}

extern "C" {
    API_EXPORT Config* api_get_native_config() {
        return &ConfigManager::instance();
    }

    API_EXPORT void api_set_transition_disabled(const char* name, bool disabled) {
        ConfigManager::setTransitionDisabled(name, disabled);
    }

    API_EXPORT bool api_get_transition_disabled(const char* name) {
        return ConfigManager::isTransitionDisabled(name);
    }

    API_EXPORT void api_set_config_string(char* dest, const char* src, int max_len) {
        if (dest && src && max_len > 0) {
            std::strncpy(dest, src, max_len - 1);
            dest[max_len - 1] = '\0';
        }
    }

    API_EXPORT DisabledTransitions* api_get_disabled_transitions() {
        return &ConfigManager::instance().disabled_transitions;
    }
}
