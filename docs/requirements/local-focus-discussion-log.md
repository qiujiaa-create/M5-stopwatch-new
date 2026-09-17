# Local Focus Timer - Discussion Log

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

## Implementation choices to verify

- A running timer continues while another watch app is open or the display sleeps. A device reboot starts a fresh timer state.
- Countdown completion displays zero and gives one vibration, including when the Focus app is closed.
- Touch can select either timer and use the large central action; Reset is available only in PAUSED. A+B holds return to the Launcher.
- The reference layout is adapted to the 466×466 circular AMOLED using the firmware's available Latin fonts; the displayed labels are English.

## Acceptance still needed

- On the physical watch, inspect clipping and touch targets; test live tenths, A/B short press and paused long press, timer mutual exclusion, exit and return, screen sleep, countdown completion, and Codex A/B regression.
- Record build, flashed artifact, and physical behavior separately. Do not treat the source change as installed firmware.

## Implementation evidence

- Branch: `codex/local-focus-timers`; rollback source commit: `1e1805b41a2cc9167b32855ca05e05f80f23d61e`.
- Host timer state tests and ESP-IDF 5.5.4 factory build passed; `tools/check_version.sh` passed. Final application image: 4,111,264 bytes; SHA-256 `b01dc253421a3ae4f9655d865c7cf8c655ba70dba016da30989a4238cab4a2fc`.
- Connected device partition table matched the build, and `otadata` selected factory before flashing.
- `app-flash` wrote only the application at `0x20000`; esptool reported `Hash of data verified`. A reset boot log selected factory and opened Launcher. This does not prove the Focus page's on-screen appearance or button behavior.
- The user reported initial Focus interaction was normal. The device then restarted while I was flashing and resetting it to capture boot logs; after I stopped serial access, the user reported no further restarts. Those restarts were caused by my operations and are not evidence of a firmware crash.
- D-04 revision: host timer tests, `git diff --check`, ESP-IDF 5.5.4 build, and `tools/check_version.sh` passed. Revised factory image: 4,111,648 bytes; SHA-256 `25d5a7e919437c12d3b2c3302f9066277df7858fa48e08b64e802ca89e9c1df5`.
- The connected device appeared as `/dev/cu.usbmodem21201`, with no process holding the port. Based on the previously verified matching partition layout and the user's active Focus use from factory, one `app-flash` wrote only the application at `0x20000`; esptool reported `Hash of data verified` and one automatic hard reset. No further serial access or reset was performed. On-watch UI and input behavior still need the user's confirmation.
