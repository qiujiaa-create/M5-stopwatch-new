#include "launcher_shortcuts.h"

#include <hal/utils/settings/settings.h>
#include <string_view>

namespace launcher_shortcuts {
namespace {

constexpr const char* kSettingsNamespace = "launcher";
constexpr const char* kLongAKey = "hold_a";
constexpr const char* kLongBKey = "hold_b";

const char* appName(Target target)
{
    switch (target) {
        case Target::Codex:
            return "Codex";
        case Target::Focus:
            return "Focus";
        case Target::Xiaozhi:
            return "Xiaozhi";
        case Target::WatchFace:
            return "WatchFace";
        case Target::LuckyWheel:
            return "LuckyWheel";
        case Target::Maze:
            return "Maze";
        case Target::Settings:
            return "Settings";
        case Target::Disabled:
        default:
            return "";
    }
}

}  // namespace

const char* storageKey(Target target)
{
    switch (target) {
        case Target::Codex:
            return "codex";
        case Target::Focus:
            return "focus";
        case Target::Xiaozhi:
            return "xiaozhi";
        case Target::WatchFace:
            return "watch_face";
        case Target::LuckyWheel:
            return "lucky_wheel";
        case Target::Maze:
            return "maze";
        case Target::Settings:
            return "settings";
        case Target::Disabled:
        default:
            return "disabled";
    }
}

const char* displayName(Target target)
{
    switch (target) {
        case Target::Codex:
            return "Codex";
        case Target::Focus:
            return "Focus";
        case Target::Xiaozhi:
            return "Xiaozhi";
        case Target::WatchFace:
            return "Watch Face";
        case Target::LuckyWheel:
            return "Lucky Wheel";
        case Target::Maze:
            return "Maze";
        case Target::Settings:
            return "Settings";
        case Target::Disabled:
        default:
            return "Disabled";
    }
}

Target targetFromStorageKey(const std::string& key)
{
    for (const auto target : SelectableTargets) {
        if (key == storageKey(target)) {
            return target;
        }
    }
    return Target::Disabled;
}

std::size_t targetIndex(Target target)
{
    for (std::size_t i = 0; i < SelectableTargets.size(); ++i) {
        if (SelectableTargets[i] == target) {
            return i;
        }
    }
    return 0;
}

Target targetAt(std::size_t index)
{
    return index < SelectableTargets.size() ? SelectableTargets[index] : Target::Disabled;
}

Config loadConfig()
{
    Settings settings(kSettingsNamespace, false);
    Config config;
    config.longA = targetFromStorageKey(settings.GetString(kLongAKey, storageKey(Target::Xiaozhi)));
    config.longB = targetFromStorageKey(settings.GetString(kLongBKey, storageKey(Target::Focus)));
    return config;
}

void saveConfig(const Config& config)
{
    Settings settings(kSettingsNamespace, true);
    settings.SetString(kLongAKey, storageKey(config.longA));
    settings.SetString(kLongBKey, storageKey(config.longB));
}

int resolveAppId(Target target, const std::vector<mooncake::AppProps_t>& appProps)
{
    const std::string_view expectedName = appName(target);
    if (expectedName.empty()) {
        return -1;
    }

    for (const auto& props : appProps) {
        if (props.info.name == expectedName) {
            return props.appID;
        }
    }
    return -1;
}

}  // namespace launcher_shortcuts
