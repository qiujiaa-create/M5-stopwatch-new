/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <apps/app_setup/workers/workers.h>
#include <mooncake.h>
#include <memory>

/**
 * @brief 启动器一级页面上的「Xiaozhi」入口（与 Codex 同级）
 *
 * 点开后进入确认页：A 写入 otadata 指向 ota_0 并重启进小智，B 取消。
 * 复用设置菜单里的 SwitchSystemWorker，保证两条入口行为完全一致
 * （包括"ota_0 里没有可启动镜像就不让切"的保护）。
 */
class AppXiaozhi : public mooncake::AppAbility {
public:
    AppXiaozhi();

    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    std::unique_ptr<setup_workers::SwitchSystemWorker> _worker;
};
