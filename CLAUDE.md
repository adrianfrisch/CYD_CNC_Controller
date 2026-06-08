# CLAUDE.md — CYD CNC Controller

> Context file for AI coding assistants (Claude, Cursor, Windsurf, Copilot, etc.)

## What is this project?

A standalone **touchscreen GCode sender** for GRBL-based CNC machines. Runs on ESP32 boards with TFT touchscreens — primarily the ESP32-2432S028R "Cheap Yellow Display" (2.8", 320×240), but also supports the ESP32-4827S043R (4.3", 480×272 RGB parallel) and other display sizes via resolution-independent layout. Connects to an Arduino Uno running GRBL 1.1 via 3-wire UART. Users upload GCode files over WiFi, browse/preview on the touchscreen, and run CNC jobs without a PC.

**Status:** Work in progress — functional for basic use but has known bugs (see README.md "Known Issues").

## Quick Reference

| Aspect | Detail |
|--------|--------|
| Language | C++17 (embedded subset — no STL, no exceptions) |
| Platform | ESP32 / ESP32-S3 (Arduino framework) via PlatformIO |
| Build | `pio run -e esp32-2432S028R` / `pio run -e esp32-4827S043R` |
| Test | `pio test -e native` (Unity framework, runs on PC) |
| Display lib | TFT_eSPI (SPI displays) or LovyanGFX (RGB parallel displays) |
| Web server | ESPAsyncWebServer + AsyncTCP |
| Target firmware | GRBL 1.1 on Arduino Uno |

## Supported Boards

| Board | MCU | Display | Touch | Display Lib |
|-------|-----|---------|-------|-------------|
| ESP32-2432S028R (CYD 2.8") | ESP32 | 2.8" ILI9341 320×240 SPI | XPT2046 resistive (software SPI) | TFT_eSPI |
| ESP32-4827S043R (4.3") | ESP32-S3 N8R8 | 4.3" ST7262 480×272 RGB parallel | XPT2046 resistive (hardware SPI, shared with SD) | LovyanGFX |

## Repository Layout

```
include/config.h             — Pin defs, constants, colors, WiFi defaults (overridable via build flags)
src/main.cpp                 — Entry point: setup()/loop() calls begin()/loop() on each module
src/grbl_comm.h/cpp          — UART serial comms with GRBL, status parsing, character-counting streaming
src/sd_manager.h/cpp         — SD card file operations (list, open, delete)
src/job_streamer.h/cpp       — GCode file → GRBL streaming with flow control & progress tracking
src/web_server.h/cpp         — WiFi STA mode + async REST API for file upload/delete
src/ui/ui_manager.h/cpp      — Display init, touch input, screen management
src/ui/ui_layout.h           — Resolution-independent layout constants (derived from SCREEN_W/H)
src/ui/display_driver.h      — Abstraction: selects TFT_eSPI or LovyanGFX via USE_LOVYANGFX flag
src/ui/lgfx_config_4827S043.h — LovyanGFX panel config for ESP32-4827S043R RGB display
src/ui/screen_*.h/cpp        — 5 screens: calibration, filebrowser, preview, job, jog
src/ui/xpt2046_soft.h/cpp    — XPT2046 touch driver (software SPI or hardware SPI shared bus)
lib/testable/                — Pure logic extracted for native testing (no hardware deps)
test/test_*/                 — Unity test suites (142 total tests across 6 suites)
tools/grbl_simulator.py      — Python GRBL 1.1 simulator for development
tools/run_integration_tests.py — Integration test runner
data/wifi.cfg.example        — WiFi config file template
```

## Architecture Pattern

**Singleton modules** with `begin()`/`loop()` lifecycle:

```
main.cpp::setup()  → ui.begin() → sdCard.begin() → grbl.begin() → job.begin() → webServer.begin()
main.cpp::loop()   → grbl.loop() → job.loop() → ui.loop() → webServer.loop()
```

Global singletons (declared `extern` in headers): `grbl`, `sdCard`, `job`, `ui`, `webServer`

**Display abstraction:** `DisplayDriver` typedef resolves to `TFT_eSPI` or `LGFX` (LovyanGFX) based on the `USE_LOVYANGFX` build flag. Both expose compatible APIs (fillScreen, drawString, etc.)

**Touch driver:** `XPT2046_Soft` auto-detects shared SPI bus (when touch CLK/MOSI/MISO match SD pins) and uses hardware SPI transactions instead of bit-banging. Touch axis mapping is board-specific (swapped on rotated SPI panels, direct on RGB panels).

**Testability strategy:** Pure computational logic is extracted into `lib/testable/testable_logic.h/.cpp` which compiles on both native (PC) and ESP32. Tests use Unity framework in `test/` directory.

## Coding Conventions — IMPORTANT

Follow these strictly when generating code:

1. **No heap allocation** — use fixed `char[]` buffers, C arrays with counters. Never use `new`/`delete`, `std::vector`, `std::string`, `String`.
2. **`#pragma once`** for header guards
3. **4-space indentation**, K&R braces (opening brace on same line)
4. **Naming:** `camelCase` functions/vars, `PascalCase` classes/enums, `UPPER_SNAKE` for `#define`
5. **Private members:** underscore prefix (`_state`, `_rxBuf`)
6. **Global singletons:** lowercase (`grbl`, `job`, `sdCard`, `ui`, `webServer`)
7. **Section separators:** `// =======` banner comments between file sections
8. **Max line sizes:** filenames 64 chars (`MAX_FILENAME`), GCode lines 256 chars (`GCODE_LINE_MAX`)
9. **Arduino patterns:** Use `millis()` for timing, never `delay()` in loop(), `Serial.printf()` for debug
10. **GRBL protocol:** Character-counting streaming (128-byte buffer, 16 in-flight commands max)

## Hardware Constraints

### ESP32-2432S028R (CYD 2.8")
- **CPU:** ESP32 dual-core 240 MHz
- **RAM:** 320 KB SRAM (no PSRAM) — ~14% used
- **Flash:** 4 MB — ~29% used
- **Display:** ILI9341 320×240 SPI, rotation=1 for landscape
- **Touch:** XPT2046 on dedicated software SPI (GPIO 32/39/25/33)
- **SD:** HSPI (GPIO 23/19/18/5), FAT32, max 32 GB
- **UART to GRBL:** 115200 baud via GPIO 27 (TX) / GPIO 22 (RX)

### ESP32-4827S043R (4.3")
- **CPU:** ESP32-S3 dual-core 240 MHz
- **RAM:** 512 KB SRAM + 8 MB OPI PSRAM (required for RGB framebuffer)
- **Flash:** 8 MB
- **Display:** ST7262 480×272, 16-bit RGB parallel via LovyanGFX, rotation=0 (native landscape)
- **Touch:** XPT2046 on shared hardware SPI (GPIO 11/13/12, CS=38) — same bus as SD
- **SD:** FSPI (GPIO 11/13/12/10), FAT32
- **UART to GRBL:** 115200 baud via GPIO 17 (TX) / GPIO 0 (RX)
- **Note:** GPIO 33-37 reserved for OPI PSRAM — never use as GPIO

### Display Resolution Configuration

Resolution is set via PlatformIO build flags (portrait dimensions — swapped to landscape in `config.h`):

```ini
; platformio.ini example for 3.5" display (portrait: 320×480 → landscape: 480×320)
build_flags = -DTFT_WIDTH=320 -DTFT_HEIGHT=480
```

All UI layout is proportional via `src/ui/ui_layout.h`. Never hardcode pixel positions in screen code.

## Key Interfaces

### GrblComm
```cpp
grbl.sendLine("G1 X10 F500");      // Queue GCode line (character-counting)
grbl.sendRealtime('?');             // Status query (no newline)
grbl.feedHold();                    // Send '!'
grbl.cycleResume();                 // Send '~'
grbl.softReset();                   // Send 0x18
grbl.jog(1.0, 0.0, 0.0, 1000);    // $J=G91 X1.000 F1000
grbl.status();                      // Returns const GrblStatus&
grbl.availableInBuffer();           // Bytes free in GRBL's RX buffer
```

### JobStreamer  
```cpp
job.startJob("/file.nc");           // Begin streaming
job.pause(); job.resume(); job.stop();
job.percentComplete();              // 0–100
job.elapsedMs(); job.estimatedRemainingMs();
job.currentLine(); job.totalLines();
job.onJobDone([](bool ok){ ... });  // Completion callback
```

### UI Screens
Each screen implements: `void draw()` (full redraw) and `void handleTouch(int x, int y)`.
Screens: `ScreenCalibration`, `ScreenFileBrowser`, `ScreenPreview`, `ScreenJob`, `ScreenJog`.

## Testing

```bash
pio test -e native                  # All 142 unit tests
pio test -e native -f test_grbl_parser  # Just GRBL parser tests
pio test -e native -v               # Verbose output
```

Test suites: `test_grbl_parser` (20), `test_gcode` (17), `test_sd_filter` (34), `test_progress` (20), `test_wifi_config` (23), `test_machine_config`

When adding testable logic:
1. Add declaration to `lib/testable/testable_logic.h`
2. Implement in `lib/testable/testable_logic.cpp` (no Arduino.h dependency)
3. Add Unity tests in `test/test_<name>/test_<name>.cpp`

## Known Issues (don't duplicate these bugs)

- Homing ($H) is untested

## Files You Should NOT Modify Without Asking

- `platformio.ini` TFT_eSPI build flags for esp32-2432S028R (display will break)
- `include/config.h` pin definitions (hardware-specific, overridable via build flags)
- `data/wifi.cfg` (user credentials, gitignored)
- `src/ui/lgfx_config_4827S043.h` RGB panel timing (carefully tuned for stability)

