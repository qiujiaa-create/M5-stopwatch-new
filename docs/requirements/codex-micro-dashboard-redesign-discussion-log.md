# Codex Micro Dashboard Redesign - Discussion Log

## Document note

- Topic: reshape the existing Codex app around `digitsisyph/codex-micro-stopwatch`
- Date: 2026-09-15
- Stage: implemented; physical interaction acceptance pending
- Scope covered: the current conversation, the saved local firmware baseline, the current repository architecture, and upstream commit `e4f5103`

## Current conclusion summary

### Confirmed

- The current working project must be saved before redesign work begins.
- Keep the existing StopWatch operating-system/application framework so Codex remains one app alongside other present and future apps such as Stopwatch and Pomodoro.
- Keep the current microphone implementation and its working behavior, including the BLE microphone path, macOS Bridge, virtual microphone, Typeless integration, and external-microphone mode.
- Redesign the Codex app toward the appearance and functional form of `digitsisyph/codex-micro-stopwatch`.
- Provide both the existing Typeless workflow and the target native Codex Micro controls as selectable modes.
- Keep the existing 480 ms hold-to-confirm behavior for Agent selection.
- Keep the current activity, reasoning, Pet, and related Codex features in the legacy themes; the new target dashboard becomes the default Codex theme without deleting those implementations.

### Recommended

- Port the target dashboard and selected interaction behavior into the current `AppCodex` boundary; do not replace the ESP-IDF, Mooncake, LVGL, launcher, HAL, BLE Bridge, or microphone foundation with the target repository's standalone Arduino/PlatformIO main loop.
- Treat the target dashboard as a new Codex theme and make it the default after acceptance. Keep the existing themes temporarily as a rollback path until real-device acceptance is complete.
- Continue using the current unified HID and Bridge services as the transport source of truth; adapt their state into the target dashboard rather than importing the target BLE stack wholesale.

### Pending confirmation

- No scope decision remains open for the first implementation batch. Minor visual tuning may be adjusted during physical-device acceptance without changing the confirmed interaction contract.

## Key decisions

| ID | Decision | Status | Impact |
| --- | --- | --- | --- |
| D-01 | Save the current working source before redesign | Confirmed | Baseline commit `efc54f5`; local backup branch `backup/pre-codex-micro-redesign-20260915` |
| D-02 | Preserve the multi-app operating-system/application framework | Confirmed | The redesign stays inside the Codex app and shared Codex adapters; launcher and unrelated apps remain intact |
| D-03 | Preserve the current microphone implementation | Confirmed | No replacement with the target repository's optional USB-mic architecture; both M5 BLE mic and external mic behavior remain supported |
| D-04 | Use `digitsisyph/codex-micro-stopwatch` as the target Codex product form | Confirmed | Dashboard layout, six Agent presentation, central quota dial, status semantics, and selected controls become the design reference |
| D-05 | Port rather than merge firmware foundations | Recommended | Avoids replacing ESP-IDF/Mooncake/LVGL and avoids disrupting BLE bonds, microphone transport, launcher, and future apps |
| D-06 | Keep Typeless and native Codex Micro as selectable input modes | Confirmed | Typeless remains available; native mode adds the target A/B/center/swipe controls without replacing the existing microphone path |
| D-07 | Retain 480 ms Agent confirmation | Confirmed | All six Agent targets require a continuous 480 ms hold before the command is emitted |
| D-08 | Preserve legacy Codex features in legacy themes | Confirmed | Activity, reasoning, Pet, and related views remain in source and continue to be available outside the new default dashboard |

## Discussion process

### Baseline preservation

The working firmware changes for BLE identity and external-microphone behavior were committed locally. No remote push was performed. A local backup branch points to the same commit for recovery before any redesign changes.

### Initial architecture comparison

The current project is a multi-app ESP-IDF firmware using Mooncake and LVGL. Its Codex app consumes shared HAL, unified Codex Micro HID, BLE Bridge state, quota data, and microphone state.

The target project is a standalone Arduino/PlatformIO firmware whose main loop owns the entire dashboard, buttons, gestures, power lifecycle, BLE HID, quota service, and optional USB microphone. Copying that firmware wholesale would remove the current launcher/app model and conflict with the microphone path that must be retained.

The compatible direction is therefore a bounded port: reproduce the target dashboard and approved controls inside `AppCodex`, while keeping the current platform and transports underneath it.

## Next actions

1. Visually confirm the new dashboard on the physical C152 and exercise all six 480 ms Agent targets.
2. Select Codex Micro in the Bridge and verify A/ACT10, B/ACT09, center/ACT12, and four radial swipe directions against the live Codex app.
3. Reconfirm Typeless with the external DJI microphone, then separately test the M5 virtual microphone path when that input is intentionally enabled.
4. Confirm the Launcher and unrelated apps still open and close normally before accepting the branch.

## Implementation evidence

- Implemented on branch `feat/codex-micro-dashboard` without changing the launcher, app registry, partition table, BLE identity, HID report map, or microphone services.
- Added the default Codex Micro dashboard plus retained `Official V1` and `OpenWatcher V2` in Device Setup.
- Added six-Agent display/state adaptation, 480 ms hold confirmation, central quota/link/battery telemetry, center Send, radial swipes, native Mic, and Voice Chat actions.
- Added selectable `Codex Micro` Bridge mode while leaving the live configuration at `Typeless` with `virtualMicrophoneEnabled=false` for the external DJI microphone.
- ESP-IDF 5.5.4 build passed with 38% of the smallest app partition free; Bridge Swift compilation, voice-start gate tests, audio-route tests, image hash verification, and app-only flash all passed.
- A first physical launch exposed an overlong NVS migration key; it was shortened to `theme_gen`, rebuilt, and reflashed. A second physical opening/visual interaction pass still requires the user at the watch.

## Related sources

- Current source baseline: local commit `efc54f5`
- Recovery branch: `backup/pre-codex-micro-redesign-20260915`
- Target reference: `https://github.com/digitsisyph/codex-micro-stopwatch`, upstream commit `e4f5103`
