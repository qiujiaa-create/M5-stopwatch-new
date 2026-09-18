# M5 StopWatch Vibe Coding Firmware

Document version: 0.6. Last updated: 2026-09-18 11:44.

ESP-IDF firmware for the M5Stack StopWatch `factory` slot. The current Codex app has two themes, Codex Micro and OpenWatcher V2; Official V1 / Classic Pet has been removed. A separate local Focus app provides count-up and configurable 1–60-minute countdown timers, plus an optional completion chime. This firmware also provides standard BLE HID, Codex Vendor HID Agent/Encoder/Radial controls, a real-time BLE microphone, quota/activity panels, power management, and a checked switch into the separate Xiaozhi `ota_0` firmware.

For the module map, touch parameters, BLE protocols and safe secondary-development workflow, read [Agent and secondary-development guide](../docs/AGENT_DEVELOPMENT_GUIDE.md).

For Focus duration editing, the click-to-toggle speaker button, blue countdown arc, four-second completion chime, key controls, tenths display, and persistence, read the [Focus user guide](../docs/FOCUS.md).

## Build

### Fetch Dependencies

```bash
python3 ./fetch_repos.py
```

### Tool Chains

[ESP-IDF v5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/index.html)

### Build

```bash
idf.py build
```

### Flash

```bash
idf.py flash
```

Use `idf.py app-flash` only when the device already has a compatible Bootloader and partition table. Check the exact USB port and active boot slot before flashing; record build, installed image, and device behavior separately. Firmware v0.10.0 changed the HID layout; after upgrading from v0.9.x, remove the old `M5Codex-*` entry from macOS Bluetooth settings and pair it again once.
