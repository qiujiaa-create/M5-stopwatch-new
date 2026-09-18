#include "../main/apps/app_focus/focus_timer.h"
#include <cassert>

int main()
{
    focus::FocusTimer timer;
    constexpr uint64_t minute = 60ULL * 1000;
    assert(timer.countdownMinutes() == 25);
    assert(!timer.ringEnabled());
    assert(timer.countdownRemainingMs(0) == 25 * minute);
    assert(timer.countUpElapsedMs(0) == 0);

    timer.clickCountUp(0);
    assert(timer.countUpState() == focus::TimerState::Running);
    assert(timer.countUpElapsedMs(90'000) == 90'000);
    timer.clickCountUp(90'000);
    assert(timer.countUpState() == focus::TimerState::Paused);
    assert(timer.countUpElapsedMs(200'000) == 90'000);
    timer.clickCountUp(200'000);
    timer.resetCountUp();
    assert(timer.countUpState() == focus::TimerState::Running);
    timer.clickCountUp(230'000);
    assert(timer.countUpState() == focus::TimerState::Paused);
    assert(timer.countUpElapsedMs(500'000) == 120'000);
    timer.clickCountUp(500'000);
    assert(timer.countUpState() == focus::TimerState::Running);
    assert(timer.countUpElapsedMs(500'000) == 120'000);
    timer.clickCountUp(500'000);
    timer.resetCountUp();
    assert(timer.countUpState() == focus::TimerState::Ready);
    assert(timer.countUpElapsedMs(500'000) == 0);

    timer.clickCountUp(500'000);
    assert(timer.countUpElapsedMs(500'000) == 0);
    timer.clickCountdown(510'000);
    assert(timer.countUpState() == focus::TimerState::Paused);
    assert(timer.countUpElapsedMs(700'000) == 10'000);
    assert(timer.countdownState() == focus::TimerState::Running);
    timer.clickCountdown(520'000);
    assert(timer.countdownState() == focus::TimerState::Paused);
    assert(timer.countdownRemainingMs(700'000) == 25 * minute - 10'000);
    timer.clickCountdown(700'000);
    assert(timer.countdownRemainingMs(705'000) == 25 * minute - 15'000);
    timer.resetCountdown();
    assert(timer.countdownState() == focus::TimerState::Running);
    assert(!timer.poll(700'000 + 25 * minute - 10'001));
    assert(timer.poll(700'000 + 25 * minute - 10'000));
    assert(!timer.poll(700'000 + 25 * minute));
    assert(timer.countdownState() == focus::TimerState::Completed);
    assert(timer.countdownRemainingMs(700'000 + 25 * minute) == 0);

    timer.clickCountdown(3'000'000);
    assert(timer.countdownState() == focus::TimerState::Running);
    assert(timer.countdownRemainingMs(3'000'000) == 25 * minute);
    timer.clickCountdown(3'001'000);
    timer.resetCountdown();
    assert(timer.countdownState() == focus::TimerState::Ready);
    assert(timer.countdownRemainingMs(3'100'000) == 25 * minute);

    timer.clickCountUp(3'200'000);
    timer.clickCountdown(3'201'000);
    assert(timer.countUpState() == focus::TimerState::Paused);
    timer.clickCountUp(3'202'000);
    assert(timer.countdownState() == focus::TimerState::Paused);

    // A saved duration replaces the current countdown and is used by reset and restart.
    assert(!timer.configureCountdownMinutes(0));
    assert(!timer.configureCountdownMinutes(61));
    assert(timer.countdownMinutes() == 25);
    assert(timer.configureCountdownMinutes(1));
    assert(timer.countdownState() == focus::TimerState::Ready);
    assert(timer.countdownRemainingMs(3'202'000) == minute);
    timer.clickCountdown(4'000'000);
    assert(timer.countdownRemainingMs(4'030'000) == 30'000);
    assert(timer.configureCountdownMinutes(60));
    assert(timer.countdownState() == focus::TimerState::Ready);
    assert(timer.countdownRemainingMs(4'030'000) == 60 * minute);
    timer.clickCountdown(5'000'000);
    timer.clickCountdown(5'001'000);
    timer.resetCountdown();
    assert(timer.countdownRemainingMs(5'002'000) == 60 * minute);
    timer.setRingEnabled(true);
    assert(timer.ringEnabled());
    timer.setRingEnabled(false);
    assert(!timer.ringEnabled());
}
