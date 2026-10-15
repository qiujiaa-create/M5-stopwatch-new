// Executes the production view against LVGL. Only physical HAL is substituted.
#include <apps/app_codex/view/view.h>
#include <apps/app_codex/view/deck_layout.h>
#include <hal/hal.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
using view::CodexView;
static int touch_x = 0, touch_y = 0;
static bool touch_down = false;
static size_t flushed_pixels = 0, flush_count = 0, quota_draws = 0;
static std::vector<uint8_t> frame(466 * 466 * 3);
static void advance(CodexView& view, int ms) {
    for (int elapsed = 0; elapsed < ms; elapsed += 10) {
        lv_tick_inc(10); lv_timer_handler(); view.update();
    }
}
static void touch(CodexView& view, int x, int y, bool down, int ms) {
    touch_x = x; touch_y = y; touch_down = down; advance(view, ms);
}
static void save(const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    file << "P6\n466 466\n255\n";
    // RGB888 is stored BGR by LVGL. Apply the physical circular screen mask.
    for (int y = 0; y < 466; ++y) for (int x = 0; x < 466; ++x) {
        const int offset = (y * 466 + x) * 3;
        const bool outside = (x - 233) * (x - 233) + (y - 233) * (y - 233) > 232 * 232;
        const char pixel[3] = {static_cast<char>(outside ? 230 : frame[offset + 2]),
                               static_cast<char>(outside ? 230 : frame[offset + 1]),
                               static_cast<char>(outside ? 230 : frame[offset])};
        file.write(pixel, 3);
    }
}
int main(int argc, char** argv) {
    assert(argc == 2);
    std::filesystem::path output(argv[1]); std::filesystem::create_directories(output);
    lv_init(); lv_tick_inc(1000);
    auto* display = lv_display_create(466, 466);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    static std::vector<uint8_t> buffer(466 * 120 * 3);
    lv_display_set_buffers(display, buffer.data(), nullptr, buffer.size(), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t* area, uint8_t* pixels) {
        const int width = lv_area_get_width(area), height = lv_area_get_height(area);
        const int stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_RGB888);
        ++flush_count; flushed_pixels += width * height;
        for (int y = 0; y < height; ++y)
            std::copy(pixels + y * stride, pixels + y * stride + width * 3,
                      frame.begin() + ((area->y1 + y) * 466 + area->x1) * 3);
        lv_display_flush_ready(d);
    });
    auto* pointer = lv_indev_create(); lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer, [](lv_indev_t*, lv_indev_data_t* data) {
        if (touch_down) data->point = {touch_x, touch_y};
        data->state = touch_down ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    });
    GetHAL().lvTouchpad = pointer;
    CodexView view; view.init(lv_screen_active());
    auto* quota_canvas = lv_obj_get_child(lv_obj_get_child(lv_screen_active(), 0), 0);
    lv_obj_add_event_cb(quota_canvas, [](lv_event_t*) { ++quota_draws; }, LV_EVENT_DRAW_MAIN_BEGIN, nullptr);
    codex::QuotaSnapshot snapshot;
    snapshot.valid = true; snapshot.weeklyLeftPct = 56; snapshot.weeklyUsage = "4D 8H";
    snapshot.sessionLeftPct = 82; snapshot.sessionResetIn = "2H 14M";
    snapshot.weeklyDayStartLeftPct = 60; snapshot.weeklyTodayUsedPctPoints = 4;
    snapshot.message = "Ready"; snapshot.sequence = 1;
    for (int i = 0; i < 24; ++i) snapshot.activityBuckets[i] = (i % 7) / 6.0f;
    view.applySnapshot(snapshot); view.applyBleState(true, "", false);
    std::array<CodexView::NativeAgentState, 6> agents{};
    const uint32_t colors[] = {0x304FFE, 0x00FF4C, 0xFF6D00, 0xFF0033};
    for (int i = 0; i < 4; ++i) { agents[i].assigned = true; agents[i].color = colors[i]; agents[i].brightness = 1; }
    view.setNativeAgentStates(true, true, agents);
    auto steady_agents = agents;
    for (int i = 0; i < 4; ++i) steady_agents[i].color = 0x00FF4C;
    view.setNativeAgentStates(true, true, steady_agents); advance(view, 300);
    auto* main_panel = lv_obj_get_child(lv_screen_active(), 0);
    lv_obj_t* battery_icon = nullptr;
    lv_obj_t* bluetooth_icon = nullptr;
    for (uint32_t i = 0; i < lv_obj_get_child_count(main_panel); ++i) {
        auto* child = lv_obj_get_child(main_panel, i);
        if (lv_obj_get_width(child) == codex::deck::kBatteryStatus.width && lv_obj_get_height(child) == codex::deck::kBatteryStatus.height) battery_icon = child;
        if (lv_obj_get_width(child) == codex::deck::kBluetoothStatus.width && lv_obj_get_height(child) == codex::deck::kBluetoothStatus.height) bluetooth_icon = child;
        assert(!(lv_obj_get_width(child) == 8 && lv_obj_get_height(child) == 8)); // Old dots removed.
    }
    assert(battery_icon && bluetooth_icon);
    auto* battery_value = lv_obj_get_child(battery_icon, 0);
    auto pixel_is = [&](int x, int y, uint32_t rgb) {
        const size_t offset = (y * 466 + x) * 3;
        return frame[offset] == (rgb & 255) && frame[offset + 1] == ((rgb >> 8) & 255) &&
               frame[offset + 2] == ((rgb >> 16) & 255);
    };
    for (int reading : {0, 29, 30, 31, 100, 255, 78}) {
        GetHAL().batteryLevel = reading;
        flushed_pixels = flush_count = quota_draws = 0; advance(view, 1100);
        const std::string expected = reading <= 100 ? std::to_string(reading) : "--";
        assert(std::string(lv_label_get_text(battery_value)) == expected);
        const uint32_t border = reading > 100 ? 0xADB9CC : (reading >= 30 ? 0x55F36A : 0xFF405C);
        assert(pixel_is(codex::deck::kBatteryStatus.x + 1, codex::deck::kBatteryStatus.y + 11, border));
        assert(flushed_pixels <= 1100 && quota_draws == 0);
        if (reading == 29) save(output / "status-low-battery.ppm");
        if (reading == 30) save(output / "status-battery-30.ppm");
    }
    // Both icons and their spacing remain within the physical circular mask.
    for (auto rect : {codex::deck::kBatteryStatus, codex::deck::kBluetoothStatus})
        for (int x : {rect.x, rect.x + rect.width - 1}) for (int y : {rect.y, rect.y + rect.height - 1})
            assert((x - 233) * (x - 233) + (y - 233) * (y - 233) < 232 * 232);
    // Reserve a real gap below Agent 2/3, including 2px for antialiasing.
    for (int slot : {1, 2}) {
        const auto card = codex::deck::agentRect(slot);
        assert(codex::deck::kBatteryStatus.y >= card.y + card.height + 6);
        assert(codex::deck::kBluetoothStatus.y >= card.y + card.height + 6);
    }
    assert(codex::deck::kBluetoothStatus.x - (codex::deck::kBatteryStatus.x + codex::deck::kBatteryStatus.width) == 18);
    assert(codex::deck::kBatteryStatus.y == codex::deck::kBluetoothStatus.y);
    assert(codex::deck::kBatteryStatus.height == codex::deck::kBluetoothStatus.height);
    view.applyBleState(false, "BLE disconnected", true); advance(view, 100);
    assert(!lv_obj_has_flag(bluetooth_icon, LV_OBJ_FLAG_HIDDEN));
    int red_pixels = 0;
    for (int y = 434; y < 456; ++y) for (int x = 254; x < 276; ++x) red_pixels += pixel_is(x, y, 0xFF405C);
    assert(red_pixels > 8);
    int slash_pixels = 0;
    for (int y = 434; y < 438; ++y) for (int x = 255; x < 259; ++x)
        slash_pixels += pixel_is(x, y, 0xFF405C);
    assert(slash_pixels > 0); // Distinguishes the slash from the Bluetooth glyph itself.
    save(output / "status-disconnected.ppm");
    view.applyBleState(true, "BLE connected", true); advance(view, 100);
    int blue_pixels = 0;
    for (int y = 434; y < 456; ++y) for (int x = 254; x < 276; ++x)
        blue_pixels += pixel_is(x, y, 0x35B8FF);
    assert(blue_pixels > 8);
    save(output / "status-connected.ppm");
    view.showActionMessage("CHECK"); advance(view, 80);
    assert(lv_obj_has_flag(battery_icon, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(bluetooth_icon, LV_OBJ_FLAG_HIDDEN));
    advance(view, 2600);
    assert(!lv_obj_has_flag(battery_icon, LV_OBJ_FLAG_HIDDEN) && !lv_obj_has_flag(bluetooth_icon, LV_OBJ_FLAG_HIDDEN));
    flushed_pixels = flush_count = quota_draws = 0; advance(view, 1000);
    assert(flush_count == 0 && flushed_pixels == 0 && quota_draws == 0);
    std::cout << "PERF idle/1000ms flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    view.setVoiceMode(CodexView::VoiceMode::Recording); advance(view, 150);
    flushed_pixels = flush_count = quota_draws = 0; advance(view, 1000);
    assert(quota_draws == 0);
    std::cout << "PERF recording/1000ms flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    // Stopping capture must restore the pad on the next frame, even while
    // recognition is still processing. Touch must remain blocked until Idle.
    view.setVoiceMode(CodexView::VoiceMode::Processing); advance(view, 40);
    auto* panel = lv_obj_get_child(lv_screen_active(), 0);
    lv_obj_t* waveform = nullptr;
    lv_obj_t* radial = nullptr;
    lv_obj_t* detail = nullptr;
    for (uint32_t i = 0; i < lv_obj_get_child_count(panel); ++i) {
        auto* child = lv_obj_get_child(panel, i);
        if (lv_obj_get_width(child) == 118 && lv_obj_get_height(child) == 42) waveform = child;
        if (lv_obj_get_width(child) == 100 && lv_obj_get_height(child) == 100) radial = child;
        if (lv_obj_get_style_width(child, LV_PART_MAIN) == 304) detail = child;
    }
    assert(detail);
    assert(waveform && lv_obj_has_flag(waveform, LV_OBJ_FLAG_HIDDEN));
    assert(radial && !lv_obj_has_flag(radial, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_opa(radial, LV_PART_MAIN) < LV_OPA_COVER);
    const auto processing_arrow_brightness = frame[(270 * 466 + 233) * 3];
    save(output / "processing.ppm");
    flushed_pixels = flush_count = quota_draws = 0; advance(view, 5500);
    assert(flush_count == 0 && quota_draws == 0); // No lingering waveform animation.
    touch(view, 283, 70, true, 100); touch(view, 283, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0);
    for (const auto& center : codex::deck::kAgentCenters) {
        touch(view, center.x, center.y, true, 400); touch(view, center.x, center.y, false, 70);
        assert(view.consumeNativeAgentSlotRequest() == -1);
    }
    touch(view, 233, 302, true, 400); touch(view, 273, 302, true, 100);
    touch(view, 273, 302, false, 70);
    float processing_angle = 0, processing_distance = 0;
    assert(!view.consumeNativeRadialRequest(processing_angle, processing_distance));
    view.setVoiceMode(CodexView::VoiceMode::Idle); advance(view, 150);
    assert(!lv_obj_has_flag(radial, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_opa(radial, LV_PART_MAIN) == LV_OPA_COVER);
    assert(frame[(270 * 466 + 233) * 3] > processing_arrow_brightness);
    flushed_pixels = flush_count = quota_draws = 0;
    touch(view, 183, 70, true, 100); touch(view, 183, 70, false, 80);
    touch(view, 283, 70, true, 100); touch(view, 283, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0);
    assert(flushed_pixels < 40000 && quota_draws == 0);
    std::cout << "PERF encoder-two-taps/360ms flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    view.setNativeAgentStates(true, true, agents);
    advance(view, 100); save(output / "deck.ppm");
    assert(frame[(199 * 466 + 60) * 3 + 2] > frame[(199 * 466 + 60) * 3 + 1]);
    assert(frame[(199 * 466 + 340) * 3 + 1] > frame[(199 * 466 + 340) * 3 + 2]);
    // Safe corners, no Agent target overlaps the radial acquisition circle.
    for (int slot = 0; slot < 4; ++slot) {
        auto rect = codex::deck::agentRect(slot);
        for (int x : {rect.x, rect.x + rect.width - 1}) for (int y : {rect.y, rect.y + rect.height - 1})
            assert((x - 233) * (x - 233) + (y - 233) * (y - 233) < 232 * 232);
        for (int y = rect.y; y < rect.y + rect.height; ++y) for (int x = rect.x; x < rect.x + rect.width; ++x)
            assert(!codex::deck::insideRadial(x, y));
        auto center = codex::deck::kAgentCenters[slot];
        touch(view, center.x, center.y, true, 120); touch(view, center.x, center.y, false, 70);
        assert(view.consumeNativeAgentSlotRequest() == -1); // Short touch cancels.
        touch(view, center.x, center.y, true, 400);
        assert(view.consumeNativeAgentSlotRequest() == slot);
        advance(view, 400); assert(view.consumeNativeAgentSlotRequest() == -1); // One commit per contact.
        touch(view, center.x, center.y, false, 70);
    }
    // Sliding before confirmation acquires a new candidate and restarts its hold.
    touch(view, 88, 304, true, 100); touch(view, 164, 388, true, 100);
    assert(view.consumeNativeAgentSlotRequest() == -1);
    advance(view, 320); assert(view.consumeNativeAgentSlotRequest() == 1);
    // Sliding after confirmation cannot submit a second slot on the same contact.
    touch(view, 302, 388, true, 400); assert(view.consumeNativeAgentSlotRequest() == -1);
    touch(view, 302, 388, false, 70);
    // An unassigned light must not disable transport-side slot switching.
    agents[0].assigned = false; view.setNativeAgentStates(true, true, agents);
    touch(view, 88, 304, true, 400); assert(view.consumeNativeAgentSlotRequest() == 0);
    touch(view, 88, 304, false, 70);
    agents[0].assigned = true; view.setNativeAgentStates(true, true, agents);
    // Encoder commits only after release inside the original side.
    touch(view, 283, 70, true, 100); assert(view.consumeReasoningDeltaRequest() == 0);
    save(output / "knob.ppm"); touch(view, 283, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 1);
    touch(view, 183, 70, true, 800); assert(view.consumeReasoningDeltaRequest() == 0);
    touch(view, 183, 70, false, 80); assert(view.consumeReasoningDeltaRequest() == -1);
    touch(view, 233, 70, true, 100); touch(view, 233, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0); // Center gap.
    touch(view, 183, 70, true, 100); touch(view, 283, 70, true, 100); touch(view, 283, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0); // Crossing cancels.
    touch(view, 183, 70, true, 100); touch(view, 204, 70, true, 80); touch(view, 183, 70, true, 80);
    touch(view, 183, 70, false, 80); assert(view.consumeReasoningDeltaRequest() == 0); // Drag and return cannot re-arm.
    touch(view, 183, 70, true, 100); touch(view, 183, 130, true, 80); touch(view, 183, 130, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0); // Outside release.
    touch(view, 283, 70, true, 100); view.setVoiceMode(CodexView::VoiceMode::Recording);
    touch(view, 283, 70, false, 80); assert(view.consumeReasoningDeltaRequest() == 0);
    view.setVoiceMode(CodexView::VoiceMode::Idle);
    float angle = 0, distance = 0;
    const float angles[] = {.75f, 0.f, .25f, .5f};
    // A short center touch cancels; the panel expands at 200 ms, with no
    // direction emitted by the opening finger even if it moves onto a button.
    touch(view, 233, 302, true, 120); touch(view, 233, 302, false, 70);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    const std::vector<uint8_t> quota_before_detail(frame.begin(), frame.begin() + 466 * 258 * 3);
    flushed_pixels = flush_count = quota_draws = 0;
    touch(view, 233, 302, true, 180);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    advance(view, 70); lv_obj_update_layout(radial); assert(!lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    std::cout << "PERF detail-open/250ms flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    touch(view, 329, 233, true, 100);
    std::cout << "PERF detail-open/350ms flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    assert(flushed_pixels * 10 <= 124456 * 11); // Opening never invalidates the full screen.
    assert(!view.consumeNativeRadialRequest(angle, distance));
    touch(view, 329, 233, false, 70);
    assert(!lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN)); // Persistent after release.
    save(output / "radial-detail.ppm");
    const auto neutral_detail_frame = frame;
    // Every solid arc pixel belongs to its visible direction, including caps.
    int arc_pixels[4] = {};
    for (int y = 81; y < 385; ++y) for (int x = 81; x < 385; ++x) {
        const int offset = (y * 466 + x) * 3;
        if (frame[offset] == 0x22 && frame[offset + 1] == 0x1B && frame[offset + 2] == 0x17) {
            const int target = codex::deck::radialDetailAt(x, y);
            assert(target >= 0 && target < 4); ++arc_pixels[target];
        }
    }
    for (int count : arc_pixels) assert(count > 1000);
    assert(codex::deck::radialDetailAt(271, 233) == 4); // Larger circular close target.
    assert(codex::deck::radialDetailAt(82, 82) == 5); // Outside the disc, inside old box.
    // Diagonal gaps, inner dead space and outer padding never select a direction.
    for (auto p : {codex::deck::Point{303, 303}, {163, 163}, {303, 163}, {163, 303},
                   {233, 283}, {233, 90}}) {
        assert(codex::deck::radialDetailAt(p.x, p.y) == -1);
        touch(view, p.x, p.y, true, 60); touch(view, p.x, p.y, false, 70);
        assert(!view.consumeNativeRadialRequest(angle, distance));
        assert(!lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    }

    flushed_pixels = flush_count = quota_draws = 0; advance(view, 1000);
    assert(flush_count == 0 && quota_draws == 0);
    for (int direction = 0; direction < 4; ++direction) {
        const auto& target = codex::deck::kRadialDirectionCenters[direction];
        const int x = target.x, y = target.y;
        flushed_pixels = flush_count = quota_draws = 0;
        touch(view, x, y, true, 60);
        assert(view.consumeNativeRadialRequest(angle, distance));
        assert(std::fabs(angle - angles[direction]) < .001f && distance == 1.0f);
        if (direction == 0) { save(output / "radial.ppm"); save(output / "radial-detail-pressed.ppm"); }
        advance(view, 400); assert(!view.consumeNativeRadialRequest(angle, distance));
        touch(view, x, y, false, 70);
        assert(view.consumeNativeRadialRequest(angle, distance) && distance == 0);
        std::cout << "PERF direction-tap/" << direction << " flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
        assert(flushed_pixels <= 25000 && quota_draws == 0);
        assert(frame == neutral_detail_frame); // No clipped cap or stale highlight after release.
        assert(!lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
        assert(view.consumeNativeAgentSlotRequest() == -1); // No modal fall-through.
    }
    // Crossing/dragging cancels and returns neutral, never selects a second
    // direction on the same contact. Re-entry cannot re-arm it.
    touch(view, 233, 137, true, 60); assert(view.consumeNativeRadialRequest(angle, distance));
    touch(view, 329, 233, true, 60);
    assert(view.consumeNativeRadialRequest(angle, distance) && distance == 0);
    touch(view, 233, 137, true, 60); assert(!view.consumeNativeRadialRequest(angle, distance));
    touch(view, 233, 137, false, 70);
    // An outside tap closes without touching the underlying Agent/Encoder.
    touch(view, 60, 304, true, 400); touch(view, 60, 304, false, 70);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN) && view.consumeNativeAgentSlotRequest() == -1);
    assert(std::equal(quota_before_detail.begin(), quota_before_detail.end(), frame.begin()));
    touch(view, 233, 302, true, 260); touch(view, 233, 302, false, 70);
    touch(view, 283, 70, true, 60); touch(view, 283, 70, false, 70);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN) && view.consumeReasoningDeltaRequest() == 0);
    // Center X closes on press, without waiting for release or another hold.
    touch(view, 233, 302, true, 260); touch(view, 233, 302, false, 70);
    flushed_pixels = flush_count = quota_draws = 0;
    // Close must work in LVGL's input callback, without a controller/view poll.
    touch_x = 233; touch_y = 233; touch_down = true;
    for (int i = 0; i < 40; i += 10) { lv_tick_inc(10); lv_timer_handler(); }
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    advance(view, 70); // Finish painting while the finger is still held.
    std::cout << "PERF detail-close/press flushes=" << flush_count << " pixels=" << flushed_pixels << " quota_draws=" << quota_draws << std::endl;
    assert(flushed_pixels * 10 <= 107436 * 11);
    touch(view, 233, 233, true, 300);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN)); // No re-entry while held.
    touch(view, 233, 233, false, 70);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    touch(view, 233, 302, true, 70); touch(view, 260, 302, true, 250);
    touch(view, 233, 302, true, 250); touch(view, 233, 302, false, 70);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    // A touch beginning outside cannot slide in and open the detail.
    touch(view, 233, 250, true, 60); touch(view, 233, 302, true, 300);
    touch(view, 233, 302, false, 70); assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN));
    // Recording interrupts an engaged direction, closes detail and returns
    // neutral. The opening contact cannot leak into recording or re-arm Idle.
    touch(view, 233, 302, true, 260); touch(view, 233, 302, false, 70);
    touch(view, 233, 137, true, 60); assert(view.consumeNativeRadialRequest(angle, distance));
    view.setVoiceMode(CodexView::VoiceMode::Recording);
    assert(view.consumeNativeRadialRequest(angle, distance) && distance == 0);
    assert(lv_obj_has_flag(detail, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(radial, LV_OBJ_FLAG_HIDDEN));
    touch(view, 233, 137, false, 70);
    touch(view, 88, 304, true, 400); assert(view.consumeNativeAgentSlotRequest() == -1);
    touch(view, 88, 304, false, 70);
    touch(view, 283, 70, true, 100); touch(view, 283, 70, false, 80);
    assert(view.consumeReasoningDeltaRequest() == 0);
    advance(view, 100); save(output / "recording.ppm");
    view.setVoiceMode(CodexView::VoiceMode::Interrupted); advance(view, 70); save(output / "interrupted.ppm");
    view.setVoiceMode(CodexView::VoiceMode::Idle);
    snapshot.sessionLeftPct = 201; view.applySnapshot(snapshot); advance(view, 70);
    assert(frame[(199 * 466 + 60) * 3 + 1] == frame[(199 * 466 + 340) * 3 + 1]);
    snapshot.stale = true; snapshot.sessionLeftPct = -1; view.applySnapshot(snapshot);
    view.setNativeAgentStates(false, false, agents); view.applyBleState(false, "", false);
    advance(view, 100); save(output / "offline.ppm");
    std::cout << "PASS: compact footer/card clearance, digits-only battery 0/29/30/31/100/unknown, local status updates, blue Bluetooth/red slash, status dots removed, footer visibility, circle bounds, separated targets, four slots, short cancellation, single commit, unassigned slot, encoder release/left/right/cancel/no-repeat/voice gate, 200ms persistent detail, four direction taps, modal cancellation/isolation, release/recording neutral, immediate waveform stop, dimmed processing pad without animation and processing touch gate\n";
}
