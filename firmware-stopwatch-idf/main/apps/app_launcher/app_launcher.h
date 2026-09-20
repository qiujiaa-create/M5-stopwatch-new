/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <apps/common/launcher_shortcuts/launcher_shortcut_gesture.h>
#include <apps/common/launcher_shortcuts/launcher_shortcuts.h>
#include "view/view.h"
#include <mooncake.h>
#include <mooncake_templates.h>
#include <cstdint>
#include <memory>

class AppLauncher : public mooncake::templates::AppLauncherBase {
public:
    void onLauncherCreate() override;
    void onLauncherOpen() override;
    void onLauncherRunning() override;
    void onLauncherClose() override;
    void onLauncherDestroy() override;

private:
    std::unique_ptr<view::LauncherView> _view;
    bool _is_first_open              = true;
    bool _pending_status_bar_create  = false;
    uint32_t _status_bar_create_tick = 0;
    uint32_t _last_battery_check_tick = 0;
    bool _should_play_boot_sfx       = true;
    launcher_shortcuts::Config _shortcut_config;
    launcher_shortcuts::ShortcutGesture _shortcut_gesture{launcher_shortcuts::HoldDurationMs};
    launcher_shortcuts::Target _pending_shortcut_target = launcher_shortcuts::Target::Disabled;
    bool _shortcut_pending = false;

    void create_launcher_view();
    void show_guide_page();
    void update_battery_status_bar(uint32_t now);
    bool handle_shortcut(uint32_t now);
};
