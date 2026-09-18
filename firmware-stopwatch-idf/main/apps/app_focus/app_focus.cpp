#include "app_focus.h"
#include <apps/common/audio/audio.h>
#include <assets/assets.h>
#include <esp_timer.h>
#include <hal/hal.h>
#include <hal/utils/settings/settings.h>
#include <mooncake_log.h>

namespace {
uint64_t nowMs()
{
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}

constexpr const char* kFocusSettingsNs = "focus";
constexpr const char* kDurationKey = "minutes";
constexpr const char* kRingKey = "ring";
constexpr float kCompletionNoteSeconds = 0.6667f;
constexpr float kCompletionVolume = 1.0f;
}  // namespace

namespace focus {
FocusTimer& timer()
{
    static FocusTimer instance = []() {
        FocusTimer loaded;
        Settings settings(kFocusSettingsNs);
        loaded.configureCountdownMinutes(settings.GetInt(kDurationKey, FocusTimer::DefaultCountdownMinutes));
        loaded.setRingEnabled(settings.GetBool(kRingKey, false));
        return loaded;
    }();
    return instance;
}

void saveCountdownMinutes(int minutes)
{
    if (!timer().configureCountdownMinutes(minutes)) return;
    Settings settings(kFocusSettingsNs, true);
    settings.SetInt(kDurationKey, minutes);
}

void saveRingEnabled(bool enabled)
{
    if (timer().ringEnabled() == enabled) return;
    timer().setRingEnabled(enabled);
    Settings settings(kFocusSettingsNs, true);
    settings.SetBool(kRingKey, enabled);
}

void pollBackground(uint64_t nowMs)
{
    if (timer().poll(nowMs)) {
        GetHAL().vibrate(300, 80);
        if (timer().ringEnabled()) {
            // Six notes at roughly two-thirds of a second provide a four-second completion cue.
            audio::play_melody({76, 83, 88, 76, 83, 88}, kCompletionNoteSeconds,
                               kCompletionVolume);
        }
        mclog::tagInfo("Focus", "countdown completed: {} min, ring={}",
                       timer().countdownMinutes(), timer().ringEnabled());
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
    _view->onSaveCountdownMinutes = [](int minutes) { focus::saveCountdownMinutes(minutes); };
    _view->onRingChanged = [](bool enabled) { focus::saveRingEnabled(enabled); };
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
    if (!_chordActive && !(_view && _view->isEditingDuration())) {
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
    auto& focusTimer = focus::timer();
    if (focusTimer.selectedMode() != mode) {
        // A cross-mode key press only changes the visible mode. The next press
        // of the key belonging to that mode performs the timer action.
        focusTimer.selectMode(mode);
        return;
    }
    if (mode == focus::Mode::CountUp) focusTimer.clickCountUp(now);
    else focusTimer.clickCountdown(now);
}

void AppFocus::actReset(focus::Mode mode)
{
    if (mode == focus::Mode::CountUp) focus::timer().resetCountUp();
    else focus::timer().resetCountdown();
}
