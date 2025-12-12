#include "Config.hpp"

std::unordered_set<std::string> ConfigManager::disabled_transitions;

bool ConfigManager::isTransitionDisabled(const std::string& name) {
    return disabled_transitions.find(name) != disabled_transitions.end();
}

void ConfigManager::setTransitionDisabled(const std::string& name, bool disabled) {
    if (disabled) {
        disabled_transitions.insert(name);
    } else {
        disabled_transitions.erase(name);
    }
}

extern "C" {
    API_EXPORT Config* api_get_native_config() {
        return &ConfigManager::instance();
    }

    API_EXPORT void api_set_transition_disabled(const char* name, bool disabled) {
        ConfigManager::setTransitionDisabled(name, disabled);
    }
}
