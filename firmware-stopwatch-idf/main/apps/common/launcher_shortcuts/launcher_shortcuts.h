#pragma once

#include <mooncake.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace launcher_shortcuts {

inline constexpr uint32_t HoldDurationMs = 1000;

enum class Target {
    Disabled,
    Codex,
    Focus,
    Xiaozhi,
    WatchFace,
    LuckyWheel,
    Maze,
    Settings,
};

inline constexpr std::array<Target, 8> SelectableTargets = {
    Target::Disabled,
    Target::Codex,
    Target::Focus,
    Target::Xiaozhi,
    Target::WatchFace,
    Target::LuckyWheel,
    Target::Maze,
    Target::Settings,
};

struct Config {
    Target longA = Target::Xiaozhi;
    Target longB = Target::Focus;
};

const char* storageKey(Target target);
const char* displayName(Target target);
Target targetFromStorageKey(const std::string& key);
std::size_t targetIndex(Target target);
Target targetAt(std::size_t index);
Config loadConfig();
void saveConfig(const Config& config);
int resolveAppId(Target target, const std::vector<mooncake::AppProps_t>& appProps);

}  // namespace launcher_shortcuts
