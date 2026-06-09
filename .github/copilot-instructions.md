# Copilot Instructions — CYD CNC Controller

## Project Summary

ESP32-based touchscreen GCode sender for GRBL 1.1 CNC machines. Supports multiple display sizes — primarily the ESP32-2432S028R "Cheap Yellow Display" (CYD) with a 2.8" ILI9341 TFT (320×240), but also 4.3" (480×272), 7" (800×480), and custom resolutions. The 2.8" and 4.3" boards use XPT2046 resistive touch; the 7" board (ESP32-8048S070C) uses GT911 capacitive touch. All boards have SD card slot and WiFi.

The CYD connects to an Arduino Uno running GRBL via 3-wire UART (TX/RX/GND). Users upload GCode files over WiFi, browse/preview them on the touchscreen, and stream jobs to the CNC — no PC required.

## Tech Stack

- **Platform:** ESP32 (Arduino framework) via PlatformIO
- **Language:** C++17 (embedded subset — no STL containers, no exceptions, no RTTI)
- **Display:** TFT_eSPI (ILI9341 SPI for 2.8") or LovyanGFX (RGB parallel for 4.3"/7")
- **Touch:** XPT2046 resistive (SW SPI) or GT911 capacitive (I2C, 7" board)
- **Web:** ESPAsyncWebServer + AsyncTCP for file upload REST API
- **JSON:** ArduinoJson v7
- **Tests:** PlatformIO Unity framework, native environment (`pio test -e native`)

## Build & Test Commands

```bash
pio run                          # Build for ESP32
pio run -e native                # Build native tests
pio test -e native               # Run all unit tests
pio test -e native -f test_grbl_parser  # Run one test suite
pio run -t upload                # Flash to CYD via USB-C
```

## Code Conventions

### Style
- **Header guards:** `#pragma once` (no `#ifndef` guards)
- **Indentation:** 4 spaces, no tabs
- **Braces:** K&R style (opening brace on same line)
- **Naming:** `camelCase` for functions/variables, `PascalCase` for classes/structs/enums, `UPPER_SNAKE` for `#define` constants
- **Member variables:** Prefixed with underscore (`_state`, `_fileName`, `_rxBuf`)
- **Global singletons:** Lowercase (`grbl`, `job`, `sdCard`, `ui`, `webServer`) declared `extern` in headers
- **Comments:** `// single-line` preferred; section banners use `// ====` blocks
- **Includes:** Arduino.h first, then config.h, then module headers

### Architecture — Module Pattern
Each module follows this pattern:
- **Header** (`.h`): Class declaration with public API, private members, `extern` singleton
- **Source** (`.cpp`): Implementation, singleton instance definition
- **Lifecycle:** `begin()` for init, `loop()` called every iteration from `main.cpp`
- All modules are singletons — no dynamic allocation, no `new`/`delete`

### Testability Pattern
Hardware-independent logic is extracted into `lib/testable/testable_logic.h/.cpp`:
- Pure functions (no Arduino.h, no hardware deps)
- Compiled for both `native` (PC) and `esp32` environments
- Tests in `test/test_*/test_*.cpp` using Unity assertions

When adding new logic, always extract the pure computation into `testable_logic` and add tests.

### Memory Constraints
- **320 KB SRAM** — avoid heap allocation; use fixed-size buffers
- **4 MB Flash** — currently ~29% used; plenty of room
- No `String` class — use `char[]` buffers with known max sizes
- No STL containers (`std::vector`, `std::map`) — use C arrays with count
- Max filename: 64 chars (`MAX_FILENAME`), Max GCode line: 256 chars (`GCODE_LINE_MAX`)

## Key Constants (include/config.h)

| Constant | Value | Meaning |
|----------|-------|---------|
| `GRBL_TX_PIN` / `GRBL_RX_PIN` | 27 / 22 | UART2 to Arduino |
| `GRBL_RX_BUFFER` | 128 | GRBL's RX buffer size |
| `GCODE_LINE_MAX` | 256 | Max chars per GCode line |
| `GCODE_BUFFER_LINES` | 16 | Max in-flight streaming commands |
| `SCREEN_W` / `SCREEN_H` | 320 / 240 | Display resolution (landscape, configurable via build flags) |
| `STATUS_POLL_MS` | 250 | GRBL status poll interval |
| `TOUCH_DEBOUNCE_MS` | 200 | Touch debounce delay |

## GRBL Protocol Essentials

- **Streaming:** Character-counting method — track cumulative unacked command lengths against 128-byte RX buffer
- **Realtime commands:** `?` (status), `!` (feed hold), `~` (resume), `0x18` (soft reset), `0x85` (jog cancel) — sent as single bytes, no newline
- **Status reports:** `<State|WPos:x,y,z|Bf:n,m|FS:f,s>` format
- **Jog format:** `$J=G91 Xn.nnn Fn\n`
- **Responses:** `ok` or `error:N` for each line sent

## UI Screen Structure

5 screens managed by `UIManager::switchScreen()`:
1. **FileBrowser** — file list with paging, select, open, delete, refresh
2. **Preview** — file info + first 10 GCode lines, START/BACK
3. **Job** — progress bar, XYZ position, elapsed/ETA, pause/resume/stop
4. **Jog** — XY pad, Z buttons, step/feed selectors, SET 0/GO 0/HOME
5. **Calibration** — 4-corner touch calibration wizard

Each screen has `draw()` (full redraw) and `handleTouch(x, y)` methods.

## File Structure Quick Reference

```
include/config.h          — All pin defs, constants, colors
src/main.cpp              — setup() + loop() — calls begin()/loop() on each module
src/grbl_comm.h/cpp       — UART to GRBL, status parsing, character-counting
src/sd_manager.h/cpp      — SD card file I/O
src/job_streamer.h/cpp    — GCode file → GRBL streaming
src/web_server.h/cpp      — WiFi STA + async web server for uploads
src/ui/ui_manager.h/cpp   — Display init, touch, screen switching
src/ui/ui_layout.h        — Resolution-independent layout constants
src/ui/screen_*.h/cpp     — Individual screen implementations
src/ui/xpt2046_soft.h/cpp — Resistive touch driver (2.8"/4.3" boards)
src/ui/gt911_touch.h/cpp  — Capacitive touch driver (7" board)
src/ui/lgfx_config_*.h    — LovyanGFX panel configs (4.3", 7" boards)
lib/testable/             — Hardware-free pure logic (for unit tests)
test/test_*/              — Unity test suites
tools/grbl_simulator.py   — Python GRBL simulator for testing
```

## Common Tasks

### Adding a new testable function
1. Declare in `lib/testable/testable_logic.h`
2. Implement in `lib/testable/testable_logic.cpp` (no Arduino.h)
3. Add test in `test/test_<category>/test_<category>.cpp`
4. Run: `pio test -e native`

### Adding a new UI screen
1. Create `src/ui/screen_<name>.h/cpp`
2. Add `Screen<Name>` class with `draw()` and `handleTouch(int x, int y)`
3. Use layout constants from `ui_layout.h` — never hardcode pixel positions
4. Register in `UIManager` — add enum value, instance, and case in `switchScreen()`

### Adding a new GRBL command
1. Add method to `GrblComm` class in `grbl_comm.h`
2. Implement in `grbl_comm.cpp` using `sendLine()` or `sendRealtime()`
3. Add button in the appropriate screen's `handleTouch()`

