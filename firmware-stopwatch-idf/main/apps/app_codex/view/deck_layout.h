#pragma once

#include <array>

namespace codex::deck {
struct Point { int x; int y; };
struct Rect {
    int x; int y; int width; int height;
    constexpr bool contains(int px, int py, int margin = 0) const {
        return px >= x - margin && px < x + width + margin &&
               py >= y - margin && py < y + height + margin;
    }
};
constexpr int kSize = 466;
constexpr Point kRadialCenter{233, 302};
constexpr int kRadialRadius = 44;
constexpr int kRadialHitRadius = 48;
// Expanded direction controls sit at the center of the circular display.
constexpr Rect kRadialDetail{81, 81, 304, 304};
constexpr Point kRadialDetailCenter{233, 233};
constexpr int kRadialDetailRadius = 152;
constexpr int kRadialArcRadius = 132;
constexpr int kRadialArcWidth = 64;
constexpr std::array<Point, 4> kRadialDirectionCenters = {{{233, 133}, {333, 233}, {233, 333}, {133, 233}}};
// Tight paint bounds include rounded caps and antialiasing. They all fit
// inside the opaque disc, so feedback never repaints the quota behind it.
constexpr std::array<Rect, 4> kRadialDirections = {{{164, 99, 139, 75},
    {293, 164, 74, 139}, {164, 293, 139, 74}, {99, 164, 75, 139}}};
constexpr Rect kRadialClose{201, 201, 64, 64};
constexpr int radialDetailAt(int x, int y) {
    const int dx = x - kRadialDetailCenter.x, dy = y - kRadialDetailCenter.y;
    const int squared = dx * dx + dy * dy;
    if (squared > kRadialDetailRadius * kRadialDetailRadius) return 5;
    if (squared <= 40 * 40) return 4;
    if (squared < 62 * 62 || squared > 140 * 140) return -1;
    const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    // tan(40 degrees): each cardinal sector spans 80 degrees with a 10-degree gap.
    if (ax * 1000 <= ay * 839) return dy < 0 ? 0 : 2;
    if (ay * 1000 <= ax * 839) return dx > 0 ? 1 : 3;
    return -1;
}
constexpr Rect kKnob{141, 55, 184, 34};
constexpr Rect kBatteryStatus{190, 434, 46, 22};
constexpr Rect kBluetoothStatus{254, 434, 22, 22};
// Two release-to-commit zones; a 16 px center gap rejects cross-zone contacts.
constexpr std::array<Rect, 2> kEncoderTargets = {{{135, 49, 90, 46}, {241, 49, 90, 46}}};
constexpr int encoderAt(int x, int y) {
    for (int side = 0; side < 2; ++side) if (kEncoderTargets[side].contains(x, y)) return side;
    return -1;
}
// Visible surfaces and finger targets share their bounds. No corner clipping.
constexpr std::array<Point, 4> kAgentCenters = {{{88, 304}, {164, 388}, {302, 388}, {378, 304}}};
constexpr int kAgentWidth = 90;
constexpr int kAgentHeight = 78;
constexpr Rect agentRect(int slot) {
    return {kAgentCenters[slot].x - kAgentWidth / 2,
            kAgentCenters[slot].y - kAgentHeight / 2, kAgentWidth, kAgentHeight};
}
constexpr bool insideRadial(int x, int y) {
    const int dx = x - kRadialCenter.x, dy = y - kRadialCenter.y;
    return dx * dx + dy * dy <= kRadialHitRadius * kRadialHitRadius;
}
constexpr int agentAt(int x, int y, int candidate = -1) {
    if (candidate >= 0 && candidate < 4 && agentRect(candidate).contains(x, y, 8)) return candidate;
    for (int slot = 0; slot < 4; ++slot) if (agentRect(slot).contains(x, y)) return slot;
    return -1;
}
}  // namespace codex::deck
