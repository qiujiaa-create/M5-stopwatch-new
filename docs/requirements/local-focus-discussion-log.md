# Local Focus Timer - Discussion Log

Document version: 1.0. Last updated: 2026-09-18 11:51.

## Document note

- Topic: add a watch focus app inspired by the TickTick focus section of `isalicema/m5-stopwatch-dashboard`.
- Stage: Focus revision built and flashed to factory; revised UI and button behavior await physical acceptance.
- Sources: this conversation, the linked reference repository and its local source copy, `PROJECT_STATE.md`, and the current firmware.

## Current conclusions

| ID | Decision | Status | Effect |
| --- | --- | --- | --- |
| D-01 | Use the reference focus page's count-up timer, 25-minute countdown, and circular visual layout. | Confirmed as reference | A/B control mutually exclusive running timers. The original double-click action was superseded by D-04. |
| D-02 | Keep all timer state on the watch; do not connect to the Mac TickTick app or a Focus Bridge. | Confirmed by user | No account, network, BLE protocol, or Mac companion change is needed. |
| D-03 | Add a separate Focus app in the Launcher. | Confirmed by user | A/B control focus only inside the new app. Codex A/B voice behavior stays as it is. |
| D-04 | Show tenths of a second in smaller type to the right of count-up seconds. Remove double-click end; while PAUSED, a long press of the corresponding A/B key resets to 00:00 or 25:00, and a short press resumes. | Confirmed by user, 2026-09-17 | Short clicks act immediately; a hold does nothing outside PAUSED. The touch reset control is shown only in PAUSED. |
| D-05 | Make countdown duration editable by tapping the minute digits; allow 1–60 minutes, default 25, and remember the last saved duration. Remove the lower B touch controls and replace them with a completion sound setting. | Confirmed by user, 2026-09-17 | NVS stores the minute selection. Saving resets the current countdown to Ready; the physical B key retains timing control. |
| D-06 | Use a click-to-toggle sound button with a small speaker icon instead of a sliding sound switch. | Confirmed by user, 2026-09-17 | The lower button displays RING ON/OFF with a speaker or muted-speaker icon. |
| D-07 | Make the enabled completion chime louder and continue for 3 seconds. | Confirmed by user, 2026-09-18 | Use six 0.5-second notes at the audio interface's maximum volume scale; preserve the global speaker volume and ring switch. |
| D-08 | Color the entire upper-right countdown arc, including the settings page, `#46D1E0`. | Confirmed by user, 2026-09-18 | Use opaque fill for the full clipped circular shape; count-up coral and timer behavior stay unchanged. |
| D-09 | Require a two-step physical-key mode switch: a cross-mode key press changes page only, and the next matching key press controls that timer. | Confirmed by user, 2026-09-18 | A on Countdown and B on Count Up no longer start the other timer immediately. |

## Implementation choices to verify

- A running timer continues while another watch app is open or the display sleeps. A device reboot starts a fresh timer state.
- Countdown completion displays zero and gives one vibration, including when the Focus app is closed.
- Touch can select either timer. Count-up retains the large action and paused Reset; countdown uses the physical B key, while its lower touch control toggles the completion sound. A+B holds return to the Launcher.
- The reference layout is adapted to the 466×466 circular AMOLED using the firmware's available Latin fonts; the displayed labels are English.

## Acceptance still needed

- On the physical watch, inspect clipping and touch targets; test live tenths, A/B short press and paused long press, 1/60-minute boundaries, saved duration after reboot, speaker button and audible completion, timer mutual exclusion, exit and return, screen sleep, countdown completion, and Codex A/B regression.
- Record build, flashed artifact, and physical behavior separately. Do not treat the source change as installed firmware.

## Implementation evidence

- Branch: `codex/local-focus-timers`; rollback source commit: `1e1805b41a2cc9167b32855ca05e05f80f23d61e`.
- Host timer state tests and ESP-IDF 5.5.4 factory build passed; `tools/check_version.sh` passed. Final application image: 4,111,264 bytes; SHA-256 `b01dc253421a3ae4f9655d865c7cf8c655ba70dba016da30989a4238cab4a2fc`.
- Connected device partition table matched the build, and `otadata` selected factory before flashing.
- `app-flash` wrote only the application at `0x20000`; esptool reported `Hash of data verified`. A reset boot log selected factory and opened Launcher. This does not prove the Focus page's on-screen appearance or button behavior.
- The user reported initial Focus interaction was normal. The device then restarted while I was flashing and resetting it to capture boot logs; after I stopped serial access, the user reported no further restarts. Those restarts were caused by my operations and are not evidence of a firmware crash.
- D-04 revision: host timer tests, `git diff --check`, ESP-IDF 5.5.4 build, and `tools/check_version.sh` passed. Revised factory image: 4,111,648 bytes; SHA-256 `25d5a7e919437c12d3b2c3302f9066277df7858fa48e08b64e802ca89e9c1df5`.
- The connected device appeared as `/dev/cu.usbmodem21201`, with no process holding the port. Based on the previously verified matching partition layout and the user's active Focus use from factory, one `app-flash` wrote only the application at `0x20000`; esptool reported `Hash of data verified` and one automatic hard reset. No further serial access or reset was performed. On-watch UI and input behavior still need the user's confirmation.
- D-05/D-06 revision: host timer tests, ESP-IDF v5.5.4 build and version check passed. Image: 4,115,984 bytes; SHA-256 `4704941a2035039bd76ab9e00de7308ff80c045e2db021714d64428e559fa052`.
- On 2026-09-18, the device at `/dev/cu.usbmodem21201` received one `app-flash` write to factory offset `0x20000`. The command completed successfully with its configured automatic hard reset. No further serial access or reset followed. This proves the prepared application was installed; physical screen, touch, button, sound, and NVS-retention behavior still await user confirmation.
- D-07 implementation: Focus now plays `{76, 83, 88, 76, 83, 88}` with 0.5 seconds per note and audio volume scale 1.0, producing a three-second completion chime without changing the persisted global speaker volume. Host timer tests and the ESP-IDF v5.5.4 build passed. The 4,116,000-byte image has SHA-256 `8f410119fa2354ecde53e2286a55bdb524bf010b13d0dc29026f6da07062b66e`. On 2026-09-18, one `app-flash` write installed it at factory offset `0x20000` and completed with the configured automatic hard reset. Physical loudness and duration still await user acceptance.
- D-07 duration revision: keep the same six-note sequence and volume scale, but use 0.6667 seconds per note for an approximately four-second completion chime. Host timer tests and the ESP-IDF v5.5.4 build passed. The new 4,116,000-byte image has SHA-256 `23015afe67368363d36f7a9a84d3d6ff0e635a60d1ec44cd9e5a354bf9071fee`; it has not been flashed yet.
- On 2026-09-18, the four-second image was written once to factory offset `0x20000` via `/dev/cu.usbmodem21201`; the command completed with its configured automatic hard reset. Physical sound duration remains for user acceptance.
- D-08 implementation: the main and editor accent discs use opaque `#46D1E0` fill for Countdown mode. Host timer tests, `git diff --check`, and ESP-IDF v5.5.4 build passed. New image: 4,116,128 bytes, SHA-256 `921e30aff211d4a9b4f94115a86deb6cf2eb83d689224aa9228ba3ec400fc1ce`; not flashed yet.
- D-09 implementation: `AppFocus::actSingle` selects the requested mode and returns when the physical key does not match the current mode; only a subsequent matching key press calls the timer action. Host timer tests, `git diff --check`, and ESP-IDF v5.5.4 build passed. New image: 4,116,128 bytes, SHA-256 `7fa9d34dbc9d403b0b50e912a50485114a082d8b2d078b7f561156f5d93e243a`; not flashed yet.
- On 2026-09-18, the D-09 image was written once to factory offset `0x20000` via `/dev/cu.usbmodem21201`; the command completed with its configured automatic hard reset. Physical two-step key behavior remains for user acceptance.
