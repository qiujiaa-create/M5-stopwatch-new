#pragma once

#include <cstdint>

namespace focus {

enum class TimerState : uint8_t { Ready, Running, Paused, Completed };
enum class Mode : uint8_t { CountUp, Countdown };

class FocusTimer {
public:
    static constexpr uint64_t CountdownDurationMs = 25ULL * 60 * 1000;

    void clickCountUp(uint64_t nowMs)
    {
        poll(nowMs);
        _selected = Mode::CountUp;
        if (_countUpState == TimerState::Running) {
            pauseCountUp(nowMs);
            return;
        }
        if (_countdownState == TimerState::Running) {
            pauseCountdown(nowMs);
        }
        if (_countUpState == TimerState::Ready) {
            _countUpAccumulatedMs = 0;
        }
        _countUpStartedAtMs = nowMs;
        _countUpState = TimerState::Running;
    }

    void resetCountUp()
    {
        if (_countUpState != TimerState::Paused) return;
        _selected = Mode::CountUp;
        _countUpState = TimerState::Ready;
        _countUpAccumulatedMs = 0;
    }

    void clickCountdown(uint64_t nowMs)
    {
        poll(nowMs);
        _selected = Mode::Countdown;
        if (_countdownState == TimerState::Running) {
            pauseCountdown(nowMs);
            return;
        }
        if (_countUpState == TimerState::Running) {
            pauseCountUp(nowMs);
        }
        if (_countdownState == TimerState::Ready || _countdownState == TimerState::Completed) {
            _countdownRemainingMs = CountdownDurationMs;
        }
        _countdownStartedAtMs = nowMs;
        _countdownState = TimerState::Running;
    }

    void resetCountdown()
    {
        if (_countdownState != TimerState::Paused) return;
        _selected = Mode::Countdown;
        _countdownState = TimerState::Ready;
        _countdownRemainingMs = CountdownDurationMs;
    }

    // Returns true only for the transition to zero, including when the app is closed.
    bool poll(uint64_t nowMs)
    {
        if (_countdownState != TimerState::Running) {
            return false;
        }
        if (nowMs - _countdownStartedAtMs < _countdownRemainingMs) {
            return false;
        }
        _countdownRemainingMs = 0;
        _countdownState = TimerState::Completed;
        return true;
    }

    uint64_t countUpElapsedMs(uint64_t nowMs) const
    {
        return _countUpAccumulatedMs +
               (_countUpState == TimerState::Running ? nowMs - _countUpStartedAtMs : 0);
    }

    uint64_t countdownRemainingMs(uint64_t nowMs) const
    {
        if (_countdownState != TimerState::Running) {
            return _countdownRemainingMs;
        }
        const uint64_t elapsed = nowMs - _countdownStartedAtMs;
        return elapsed >= _countdownRemainingMs ? 0 : _countdownRemainingMs - elapsed;
    }

    TimerState countUpState() const { return _countUpState; }
    TimerState countdownState() const { return _countdownState; }
    Mode selectedMode() const { return _selected; }
    void selectMode(Mode mode) { _selected = mode; }

private:
    void pauseCountUp(uint64_t nowMs)
    {
        _countUpAccumulatedMs += nowMs - _countUpStartedAtMs;
        _countUpState = TimerState::Paused;
    }

    void pauseCountdown(uint64_t nowMs)
    {
        _countdownRemainingMs = countdownRemainingMs(nowMs);
        _countdownState = TimerState::Paused;
    }

    TimerState _countUpState = TimerState::Ready;
    TimerState _countdownState = TimerState::Ready;
    Mode _selected = Mode::CountUp;
    uint64_t _countUpAccumulatedMs = 0;
    uint64_t _countUpStartedAtMs = 0;
    uint64_t _countdownRemainingMs = CountdownDurationMs;
    uint64_t _countdownStartedAtMs = 0;
};

FocusTimer& timer();
void pollBackground(uint64_t nowMs);

}  // namespace focus
