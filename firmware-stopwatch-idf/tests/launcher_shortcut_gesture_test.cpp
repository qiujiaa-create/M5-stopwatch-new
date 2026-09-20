#include "../main/apps/common/launcher_shortcuts/launcher_shortcut_gesture.h"
#include <cassert>

using launcher_shortcuts::GestureEvent;
using launcher_shortcuts::ShortcutGesture;

int main()
{
    ShortcutGesture gesture(1000);

    assert(gesture.update(100, true, false) == GestureEvent::None);
    assert(gesture.update(1099, true, false) == GestureEvent::None);
    assert(gesture.update(1100, true, false) == GestureEvent::ArmA);
    assert(gesture.update(1500, true, false) == GestureEvent::None);
    assert(gesture.update(1600, false, false) == GestureEvent::LaunchA);

    assert(gesture.update(2000, false, true) == GestureEvent::None);
    assert(gesture.update(3000, false, true) == GestureEvent::ArmB);
    assert(gesture.update(3100, false, true) == GestureEvent::None);
    assert(gesture.update(3200, false, false) == GestureEvent::LaunchB);

    // A+B always suppresses both single-key shortcuts, including after one
    // side of the chord is released.
    assert(gesture.update(4000, true, true) == GestureEvent::None);
    assert(gesture.update(5500, true, true) == GestureEvent::None);
    assert(gesture.update(5600, false, true) == GestureEvent::None);
    assert(gesture.update(7000, false, true) == GestureEvent::None);
    assert(gesture.update(7100, false, false) == GestureEvent::None);

    // A chord formed after a single-key shortcut is armed cancels it before
    // release, so no target app inherits a still-held physical key.
    assert(gesture.update(7200, true, false) == GestureEvent::None);
    assert(gesture.update(8200, true, false) == GestureEvent::ArmA);
    assert(gesture.update(8250, true, true) == GestureEvent::Cancelled);
    assert(gesture.update(8300, false, false) == GestureEvent::None);

    // A new gesture is accepted after both buttons have been released.
    assert(gesture.update(9000, false, true) == GestureEvent::None);
    assert(gesture.update(10'000, false, true) == GestureEvent::ArmB);
    assert(gesture.update(10'100, false, false) == GestureEvent::LaunchB);
}
