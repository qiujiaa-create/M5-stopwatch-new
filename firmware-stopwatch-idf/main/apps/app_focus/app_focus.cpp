#include "app_focus.h"
#include <assets/assets.h>
#include <esp_timer.h>
#include <hal/hal.h>
#include <mooncake_log.h>

namespace {
uint64_t nowMs()
{
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}
}  // namespace

namespace focus {
FocusTimer& timer()
{
    static FocusTimer instance;
    return instance;
}

void pollBackground(uint64_t nowMs)
{
    if (timer().poll(nowMs)) {
        GetHAL().vibrate(300, 80);
        mclog::tagInfo("Focus", "25 minute countdown completed");
    }
}
}  // namespace focus

AppFocus::AppFocus()
{
    setAppInfo().name = "Focus";
    setAppInfo().icon = (void*)&icon_stopwatch;
}

void AppFocus::onCreate() {}

void AppFocus::onOpen()
{
    _keyManager = std::make_unique<input::KeyManager>();
    _chordActive = false;
    LvglLockGuard lock;
    _view = std::make_unique<focus::FocusView>();
    _view->init(lv_screen_active());
    _view->onPrimary = [this]() { actSingle(focus::timer().selectedMode(), nowMs()); };
    _view->onReset = [this]() { actReset(focus::timer().selectedMode()); };
    _view->onSelectCountUp = []() { focus::timer().selectMode(focus::Mode::CountUp); };
    _view->onSelectCountdown = []() { focus::timer().selectMode(focus::Mode::Countdown); };
    _view->render(focus::timer(), nowMs());
}

void AppFocus::onRunning()
{
    auto& hal = GetHAL();
    hal.updateButtonStates();
    const uint64_t now = nowMs();
    focus::pollBackground(now);

    if (hal.btnA.isPressed() && hal.btnB.isPressed()) {
        _chordActive = true;
    }
    if (_keyManager && _keyManager->update(false) == input::KeyEvent::GoHome) {
        close();
        return;
    }
    if (!_chordActive) {
        if (hal.btnA.wasClicked()) actSingle(focus::Mode::CountUp, now);
        if (hal.btnB.wasClicked()) actSingle(focus::Mode::Countdown, now);
        if (hal.btnA.wasHold()) actReset(focus::Mode::CountUp);
        if (hal.btnB.wasHold()) actReset(focus::Mode::Countdown);
    }
    if (hal.btnA.isReleased() && hal.btnB.isReleased()) {
        _chordActive = false;
    }
    LvglLockGuard lock;
    if (_view) _view->render(focus::timer(), now);
}

void AppFocus::onClose()
{
    LvglLockGuard lock;
    _view.reset();
    _keyManager.reset();
}

void AppFocus::actSingle(focus::Mode mode, uint64_t now)
{
    if (mode == focus::Mode::CountUp) focus::timer().clickCountUp(now);
    else focus::timer().clickCountdown(now);
}

void AppFocus::actReset(focus::Mode mode)
{
    if (mode == focus::Mode::CountUp) focus::timer().resetCountUp();
    else focus::timer().resetCountdown();
}
