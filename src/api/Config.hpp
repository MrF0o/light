#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <unordered_set>
#include "api/api.h"

struct Config {
    double fps = 60.0;
    double animation_rate = 1.0;
    double blink_period = 0.8;
    bool transitions = true;
    bool animate_drag_scroll = false;
    bool disable_blink = false;
    float scale = 1.0f;
};

class ConfigManager {
public:
    static Config& instance() {
        static Config config;
        return config;
    }

    static bool isTransitionDisabled(const std::string& name);
    static void setTransitionDisabled(const std::string& name, bool disabled);

private:
    static std::unordered_set<std::string> disabled_transitions;
};

extern "C" {
    API_EXPORT Config* api_get_native_config();
    API_EXPORT void api_set_transition_disabled(const char* name, bool disabled);
}

#endif // CONFIG_HPP
