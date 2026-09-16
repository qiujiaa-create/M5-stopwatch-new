/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_xiaozhi.h"
#include <assets/assets.h>
#include <hal/hal.h>
#include <mooncake_log.h>

using namespace mooncake;

AppXiaozhi::AppXiaozhi()
{
    setAppInfo().name = "Xiaozhi";
    setAppInfo().icon = (void*)&icon_xiaozhi;
}

void AppXiaozhi::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

void AppXiaozhi::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    // Worker 会在构造函数里建全屏确认页，必须持锁
    LvglLockGuard lock;

    _worker = std::make_unique<setup_workers::SwitchSystemWorker>();
}

void AppXiaozhi::onRunning()
{
    if (!_worker) {
        close();
        return;
    }

    _worker->update();

    if (_worker->isDone()) {
        // 视图在 onClose 里持锁销毁
        close();
    }
}

void AppXiaozhi::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    LvglLockGuard lock;

    _worker.reset();
}
