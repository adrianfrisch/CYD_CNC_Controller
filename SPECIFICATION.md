# CYD CNC Controller — Software Specification

**Version:** 1.1  
**Date:** 2026-06-08  
**Platform:** ESP32 with TFT touchscreen (multiple display sizes supported)  
**Target CNC Firmware:** GRBL 1.1 (Arduino Uno/Nano)

---

## 1. Overview

The CYD CNC Controller is a standalone, touchscreen-based GCode sender for CNC machines running GRBL firmware. It replaces a PC-based sender (such as UGS — Universal GCode Sender) with a compact, dedicated hardware controller built on ESP32 development boards with TFT touchscreens. The primary target is the ESP32-2432S028R (commonly known as the "Cheap Yellow Display" or CYD), but the UI is resolution-independent and supports multiple display sizes (320×240, 480×320, 800×480, or custom).

The device communicates with the CNC controller over a 3-wire UART connection and provides:

- A touchscreen interface for file management, job control, and manual jogging
- An SD card for local GCode file storage
- A WiFi access point with a web-based drag-and-drop file upload interface

---

## 2. Hardware Requirements

### 2.1 Supported Displays

The controller supports multiple ESP32 TFT display boards. The UI layout adapts automatically to different screen resolutions via proportional layout constants defined in `ui_layout.h`. Display resolution is configured via PlatformIO build flags (`-DUI_SCREEN_W=xxx -DUI_SCREEN_H=yyy`).

#### 2.1.1 Primary Target — ESP32-2432S028R (CYD 2.8")

| Component          | Specification                            |
|--------------------|------------------------------------------|
| MCU                | ESP32-WROOM-32, dual-core 240 MHz        |
| Display            | 2.8" ILI9341 TFT, 320×240 pixels (landscape mode) |
| Touch              | XPT2046 resistive touchscreen            |
| Storage            | Micro-SD card slot (FAT32, ≤32 GB)       |
| Programming Port   | USB-C (CH340 USB-to-serial)              |
| RAM                | 320 KB SRAM (no PSRAM)                   |
| Flash              | 4 MB                                     |

#### 2.1.2 Additional Supported Resolutions

| Board / Display              | Resolution | MCU       | Display Driver | Touch       | Build Environment        |
|------------------------------|------------|-----------|----------------|-------------|--------------------------|
| ESP32-2432S028R (CYD 2.8")   | 320×240    | ESP32     | TFT_eSPI (ILI9341 SPI) | XPT2046 resistive (SW SPI) | `esp32-2432S028R` (default) |
| ESP32-4827S043R (4.3")        | 480×272    | ESP32-S3  | LovyanGFX (RGB parallel) | XPT2046 resistive (SW SPI) | `esp32-4827S043R` |
| ESP32-8048S070C (7.0")        | 800×480    | ESP32-S3  | LovyanGFX (RGB parallel) | GT911 capacitive (I2C) | `esp32-8048S070C` |
| Custom                        | Any        | ESP32/S3  | TFT_eSPI or LovyanGFX | XPT2046 or GT911 | Custom env |

#### 2.1.3 Display Adaptation Strategy

- All UI element positions and sizes are derived proportionally from `SCREEN_W` and `SCREEN_H` in `src/ui/ui_layout.h`
- Font sizes scale: 1.0× for 320px, 2.0× for ≥480px, 1.8× for ≥800px wide displays (using float multipliers)
- No hardcoded pixel positions exist in screen implementations — all use layout constants
- Touch calibration is per-device and stored on SD card (`/touch_cal.dat`); GT911 capacitive touch is factory-calibrated (no wizard needed)
- Display driver selection: TFT_eSPI for SPI panels, LovyanGFX for RGB parallel panels (selected via `USE_LOVYANGFX` build flag)
- GT911 touch is selected via `USE_GT911_TOUCH` build flag; XPT2046 is the default

### 2.2 CNC Controller

| Component          | Specification                            |
|--------------------|------------------------------------------|
| Board              | Arduino Uno R3 (ATmega328P)              |
| Shield             | Arduino CNC Shield v3.0                  |
| Stepper Drivers    | A4988 or DRV8825 (plugged into CNC Shield) |
| Firmware           | GRBL 1.1                                 |
| Serial Baud Rate   | 115200                                   |
| Serial Interface   | Hardware UART on pins D0 (RX) / D1 (TX)  |
| Motor Power        | 12–36V DC (via CNC Shield terminal block) |

### 2.3 Wiring — CYD to Arduino Uno + CNC Shield

#### Wiring Diagram

```
                              ┌──────────────────────────────────────────────┐
  ┌───────────────────────┐   │  Arduino CNC Shield v3.0                    │
  │  CYD ESP32-2432S028R  │   │  (stacked on top of Arduino Uno)            │
  │                       │   │                                              │
  │  ┌─────────────────┐  │   │  Stepper Drivers      12-36V DC             │
  │  │   2.8" TFT      │  │   │  ┌─────┐ ┌─────┐ ┌─────┐  ↓               │
  │  │   Touchscreen   │  │   │  │  X  │ │  Y  │ │  Z  │ [PWR]            │
  │  │   320×240 px    │  │   │  └──┬──┘ └──┬──┘ └──┬──┘                   │
  │  └─────────────────┘  │   │     │       │       │  → Stepper motors     │
  │                       │   ├─────┴───────┴───────┴───────────────────────┤
  │  [USB-C]    [SD Card] │   │  Arduino Uno R3 (running GRBL 1.1)          │
  │   5V pwr     GCode   │   │                                              │
  │   & flash    files    │   │  [USB-B] 5V power & firmware flash          │
  │                       │   │                                              │
  │  P3 Expansion Header: │   │  Pin headers (accessible under/beside CNC   │
  │  ┌───────────────┐    │   │  Shield, or via pass-through):              │
  │  │ GND     ──────────────────── GND                                     │
  │  │ GPIO 27 (TX) ────────────── D0  (RX)  ← Yellow wire                │
  │  │ GPIO 22 (RX) ────────────── D1  (TX)  ← Green wire                 │
  │  │ 3V3           │    │   │                                              │
  │  └───────────────┘    │   │                                              │
  └───────────────────────┘   └──────────────────────────────────────────────┘
           ↑                          ↑                      ↑
       5V USB-C                   5V USB-B               12-36V PSU
```

#### Signal Table

| Signal   | CYD Pin (ESP32 GPIO) | Direction | Arduino Pin | Wire Color (suggested) |
|----------|-----------------------|-----------|-------------|------------------------|
| TX → RX  | GPIO 27 (P3 header)   | →         | D0 (RX)     | Yellow                 |
| RX ← TX  | GPIO 22 (P3 header)   | ←         | D1 (TX)     | Green                  |
| GND      | GND (P3 header)       | ↔         | GND         | Black                  |

#### CNC Shield Pin Access

The CNC Shield v3.0 stacks directly on the Arduino Uno's pin headers. The serial pins D0/D1 are passed through by the shield but may be partially obstructed. Options for accessing them:

1. **Pass-through headers:** Some CNC Shield v3 boards have spare pin rows — connect here.
2. **Side access:** Use angled Dupont wires to reach D0/D1 between the two stacked boards.
3. **Solder underneath:** Attach wires directly to the Arduino Uno's D0/D1 pads on the bottom side.
4. **Serial header:** Some CNC Shield v3 boards expose a dedicated serial header—use it if available.

#### Arduino Uno Pin Layout Reference

```
  Arduino Uno — Digital Pin Header (top edge of board):

  ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
  │ D0 │ D1 │ D2 │ D3 │ D4 │ D5 │ D6 │ D7 │ D8 │ D9 │D10 │D11 │D12 │D13 │
  │ RX │ TX │    │PWM │    │PWM │PWM │    │    │PWM │PWM │PWM │    │LED │
  └──┬─┴──┬─┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
     │    │
     │    └─── Green wire  ← CYD GPIO 22 (RX)
     └──────── Yellow wire ← CYD GPIO 27 (TX)

  Arduino Uno — Power Header (bottom edge of board):

  ┌──────┬─────┬─────┬─────┬─────┬─────┬──────┐
  │RESET │ 3V3 │  5V │ GND │ GND │ Vin │      │
  └──────┴─────┴─────┴──┬──┴─────┴─────┴──────┘
                         │
                         └── Black wire ← CYD GND
```

**Notes:**
- No logic-level shifter is required. ESP32 3.3V output is above the ATmega328P logic-high threshold (~2.0V at 5V Vcc).
- Disconnect D0/D1 wires when flashing new firmware to the Arduino via USB — they share the same hardware UART.
- All three power supplies are independent. The CYD and Arduino share only GND + two serial lines.
- The CNC Shield controls stepper motors, limit switches, spindle, and coolant. The CYD does not interact with the shield directly — all commands go through GRBL's serial interface.

#### Power Supply Summary

| Board / Shield | Power Source           | Purpose                    |
|----------------|------------------------|----------------------------|
| CYD            | 5V via USB-C           | ESP32, display, SD card    |
| Arduino Uno    | 5V via USB-B           | GRBL controller            |
| CNC Shield     | 12–36V DC via terminal | Stepper motor drivers      |

### 2.4 SD Card

| Property   | Requirement       |
|------------|-------------------|
| Capacity   | ≤ 32 GB           |
| Format     | FAT32             |
| File Types | `.nc`, `.gcode`, `.gc`, `.ngc`, `.tap`, `.cnc`, `.txt` |

---

## 3. Software Architecture

### 3.1 Module Overview

```
┌─────────────────────────────────────────────────────┐
│                    main.cpp                         │
│              setup() / loop()                       │
├──────────┬──────────┬───────────┬───────────────────┤
│ GrblComm │ SDManager│JobStreamer│  WebUploadServer   │
│ grbl_comm│sd_manager│job_stream │  web_server        │
├──────────┴──────────┴───────────┴───────────────────┤
│                   UIManager                         │
│  ┌──────────┬──────────┬──────────┬──────────┐      │
│  │FileBrowse│ Preview  │   Job    │   Jog    │      │
│  └──────────┴──────────┴──────────┴──────────┘      │
├─────────────────────────────────────────────────────┤
│           config.h  (pin defs, constants)           │
└─────────────────────────────────────────────────────┘
```

### 3.2 Module Descriptions

#### 3.2.1 GrblComm (`grbl_comm.h/cpp`)

**Purpose:** Manages all serial communication with the GRBL controller.

| Responsibility                        | Detail |
|---------------------------------------|--------|
| UART initialization                   | Serial2 at 115200 baud, GPIO 27 TX / GPIO 22 RX |
| Command sending                       | GCode lines with character-counting flow control |
| Realtime commands                     | Feed hold (`!`), cycle resume (`~`), status query (`?`), soft reset (`Ctrl-X`), jog cancel (`0x85`) |
| Status parsing                        | Parses `<State\|WPos:x,y,z\|Bf:n,m\|FS:f,s\|Ov:a,b,c>` reports |
| Connection monitoring                 | 3-second timeout for connection-lost detection |
| Automatic status polling              | Sends `?` every 250 ms |

**GRBL States tracked:** `Idle`, `Run`, `Hold`, `Jog`, `Alarm`, `Door`, `Check`, `Home`, `Sleep`, `Unknown`

**Streaming Protocol:** Character-counting method — tracks the cumulative length of unacknowledged commands against GRBL's 128-byte RX buffer. Up to 16 commands may be in-flight simultaneously.

#### 3.2.2 SDManager (`sd_manager.h/cpp`)

**Purpose:** Manages the micro-SD card for GCode file storage.

| Responsibility          | Detail |
|-------------------------|--------|
| Initialization          | HSPI bus (GPIO 18/19/23/5) at 4 MHz |
| File listing            | Scans root directory, filters by GCode extensions |
| File operations         | `exists()`, `remove()`, `fileSize()`, `openFile()` |
| Storage info            | `totalBytes()`, `usedBytes()` |
| Max files               | 64 files per directory listing |
| Max filename length     | 64 characters |

**Supported file extensions:** `.nc`, `.gcode`, `.gc`, `.ngc`, `.tap`, `.cnc`, `.txt`

#### 3.2.3 JobStreamer (`job_streamer.h/cpp`)

**Purpose:** Reads a GCode file from SD and streams it to GRBL with flow control.

| Responsibility              | Detail |
|-----------------------------|--------|
| File reading                | Line-by-line from SD card |
| Comment stripping           | Removes `(...)` and `;...` comments, trims whitespace |
| Streaming                   | Uses character-counting via `GrblComm::availableInBuffer()` |
| Job control                 | `startJob()`, `pause()`, `resume()`, `stop()` |
| Progress tracking           | Percentage (bytes-based), elapsed time, estimated remaining time |
| Line tracking               | Total lines, current line number, current GCode command text |
| Completion callback         | `onJobDone(cb)` fires with `true` (success) or `false` (stopped) |

**Job States:** `Idle`, `Running`, `Paused`, `Completed`, `Error`

**Pause/Resume:** Sends GRBL feed hold (`!`) / cycle resume (`~`).  
**Stop:** Sends GRBL soft reset (`Ctrl-X`), closes file, resets buffer tracking.

#### 3.2.4 WebUploadServer (`web_server.h/cpp`)

**Purpose:** Provides a WiFi access point and web interface for uploading GCode files.

| Responsibility          | Detail |
|-------------------------|--------|
| WiFi Station Mode       | Connects to an existing WiFi network (SSID/password from config.h) |
| Auto-reconnect          | Retries connection every 30 seconds if WiFi is lost |
| Connection timeout      | Configurable via `WIFI_CONNECT_TIMEOUT_MS` (default: 15 s) |
| Hostname                | `cyd-cnc` (for mDNS / router device list) |
| IP Address              | Assigned by the network's DHCP server |
| Web server              | ESPAsyncWebServer on port 80 |
| Upload page             | Responsive HTML with drag & drop, progress bar |
| REST API                | `GET /files` (JSON list), `POST /upload` (multipart), `DELETE /delete?name=` |

**Web API Endpoints:**

| Method   | Path               | Description                  | Response        |
|----------|--------------------|------------------------------|-----------------|
| `GET`    | `/`                | Upload page (HTML)           | `text/html`     |
| `GET`    | `/files`           | List all GCode files on SD   | `application/json` — `[{"name":"file.nc","size":1234}, ...]` |
| `POST`   | `/upload`          | Upload file (multipart form) | `text/plain`    |
| `DELETE` | `/delete?name=xxx` | Delete file from SD          | `text/plain`    |

#### 3.2.5 UIManager (`ui/ui_manager.h/cpp`)

**Purpose:** Manages the TFT display, touch input, and screen navigation.

| Responsibility            | Detail |
|---------------------------|--------|
| Display initialization    | ILI9341 via TFT_eSPI, landscape rotation, backlight on GPIO 21 |
| Touch handling            | XPT2046 with 200 ms debounce |
| Touch calibration         | 4-corner calibration via TFT_eSPI `calibrateTouch()`, saved as `/touch_cal.dat` on SD card |
| Screen management         | 5 screen instances (FileBrowser, Preview, Job, Jog, Calibration), `switchScreen()` triggers full redraw |
| Drawing primitives        | `drawButton()`, `drawHeader()`, `drawProgressBar()`, `hitTest()` |
| Touch calibration         | Pre-set calibration values for landscape rotation 1 |

**Display layout:** Resolution-independent — adapts to the configured `SCREEN_W` × `SCREEN_H` (default 320×240 pixels in landscape, USB port on left). All element positions are computed proportionally via `ui_layout.h`.

---

## 4. User Interface Screens

### 4.1 Screen Flow

```
┌──────────────┐     OPEN      ┌──────────────┐    START    ┌──────────────┐
│ File Browser │ ──────────→   │   Preview    │ ────────→   │     Job      │
│              │ ←──────────   │              │             │              │
└──────┬───────┘    BACK       └──────────────┘             └──────┬───────┘
       │                                                          │
  JOG  │  FILES                                          FILES    │
       ↓    ↑                                                     │
┌──────────────┐                                                  │
│     Jog      │ ←────────────────────────────────────────────────┘
│              │             (when job not running)
└──────────────┘
```

### 4.2 File Browser Screen

**Purpose:** List, select, and manage GCode files stored on the SD card.

| Element              | Position       | Description |
|----------------------|----------------|-------------|
| Header bar           | Top (0–24px)   | Title: "CYD CNC - File Browser" |
| File list            | Left (0–266px) | 6 files per page, shows name + size, selectable with highlight |
| Page indicator       | Bottom center  | "page/total" |
| Status bar           | Bottom (220–240px) | GRBL connection status, SD card status, file count |
| JOG button           | Right column   | Navigate to Jog screen |
| UP / DOWN buttons    | Right column   | Page navigation |
| OPEN button          | Right column   | Open selected file in Preview (enabled when file selected) |
| DEL button           | Right column   | Delete selected file (enabled when file selected) |
| RFSH button          | Right column   | Refresh file list from SD card |

**Behavior:**
- Tapping a file toggles selection (highlight)
- OPEN navigates to Preview screen with the selected file
- DEL removes the file from SD and refreshes the list
- Maximum 64 files in listing

### 4.3 Preview Screen

**Purpose:** Display a preview of the GCode file content before starting execution.

| Element              | Position       | Description |
|----------------------|----------------|-------------|
| Header bar           | Top (0–24px)   | Title: "GCode Preview" |
| File info line       | Below header   | Filename, total lines, file size in bytes |
| GCode preview        | Center area    | First 10 non-empty lines with line numbers |
| Overflow indicator   | Below preview  | "... +N more lines" if file exceeds 10 lines |
| BACK button          | Bottom left    | Return to File Browser |
| START button         | Bottom right   | Start job execution, navigate to Job screen |

### 4.4 Job Screen

**Purpose:** Monitor running job progress with controls to pause, resume, or stop.

| Element              | Position       | Description |
|----------------------|----------------|-------------|
| Header bar           | Top (0–24px)   | Title: "Job Running" |
| File name            | Below header   | Currently executing file |
| Progress bar         | 48–64px        | Visual bar with percentage overlay |
| Position display     | Below progress | `X: nnn.nn  Y: nnn.nn  Z: nnn.nn` (work coordinates) |
| Status line          | Below position | GRBL state + Job state (e.g., "GRBL: Run  Job: Running") |
| Feed & spindle       | Below status   | Current feed rate (mm/min) and spindle speed (RPM) |
| Line progress        | Below feed     | "Line: current / total" |
| Time display         | Below lines    | "Elapsed: MM:SS  ETA: MM:SS" |
| Current GCode line   | Dark bar area  | Shows the GCode command currently being sent |
| PAUSE button         | Bottom row     | Sends feed hold to GRBL |
| RESUME button        | Bottom row     | Sends cycle resume to GRBL |
| STOP button          | Bottom row     | Sends soft reset, aborts job |
| FILES button         | Bottom row     | Return to File Browser (disabled during active job) |

**Update rate:** Display refreshes every 500 ms.

### 4.5 Jog Screen

**Purpose:** Manually move the CNC head in X, Y, and Z axes with configurable step sizes.

| Element              | Position       | Description |
|----------------------|----------------|-------------|
| Header bar           | Top (0–24px)   | Title: "JOG Control" |
| XY jog pad           | Left area      | 3×3 grid: cardinal directions (Y+, X-, X+, Y-) in cross layout, diagonal movement buttons (X-Y+, X+Y+, X-Y-, X+Y-) in corners |
| Z jog buttons        | Center-right   | Z+ (up) and Z- (down) — uses current step size at Z feed rate |
| Z microstep buttons  | Far right      | Z+.05 / Z-.05 — fixed 0.05mm step at 50 mm/min for Z touch-off |
| Step size display     | Center row     | "Step: N.Nmm" with – / + buttons |
| Feed rate display     | Center row     | "Feed: NNNN  Z: NNNN" with – / + buttons, in mm/min |
| Position readout     | Below feed     | `X: Y: Z: [State]` — updates every 300 ms |
| SET 0 button         | Bottom row     | Sets current position as work zero for all axes (`G10 L20 P1 X0 Y0 Z0`) |
| SET Z0 button        | Bottom row     | Sets current Z position as Z work zero only (`G10 L20 P1 Z0`) |
| GO 0 button          | Bottom row     | Rapid move to work zero — if `CLEARANCE_HEIGHT` > 0 in machine.cfg, Z raises to clearance height first, then XY moves to zero, then Z lowers to zero |
| HOME button          | Bottom row     | Homes all axes (`$H`) |
| FILES button         | Bottom row     | Return to File Browser |

**Step sizes:** 0.1, 0.5, 1.0, 5.0, 10.0, 50.0 mm (default: 1.0 mm)  
**XY Feed rates:** 50, 100, 500, 1000, 2000, 3000, 5000 mm/min (default: 1000 mm/min)  
**Z Feed rate:** Automatically calculated as XY feed ÷ 2 (configurable via `JOG_Z_FEED_DIVISOR`)  
**Z Microstep:** Fixed 0.05 mm at 50 mm/min — intended for manually zeroing Z by touching off the workpiece  
**Diagonal movement:** Moves the configured step distance on each axis simultaneously (e.g., step=1mm sends X1.000 Y1.000). This means the actual diagonal travel distance is step × √2, but each axis moves exactly the configured step amount. Feed rate is the same as XY feed.  
**Jog command format:** `$J=G91 Xn.nnn Yn.nnn Zn.nnn Fnnn`

---

## 5. Communication Protocol

### 5.1 Serial Protocol (CYD → GRBL)

| Type            | Format                     | Example                |
|-----------------|----------------------------|------------------------|
| GCode command   | `<command>\n`              | `G1 X10 Y20 F500\n`   |
| Jog command     | `$J=G91 Xn Yn Zn Fn\n`    | `$J=G91 X1.000 F1000\n`|
| Status query    | `?` (realtime, no newline) | `?`                    |
| Feed hold       | `!` (realtime)             | `!`                    |
| Cycle resume    | `~` (realtime)             | `~`                    |
| Soft reset      | `0x18` (Ctrl-X)            | `\x18`                 |
| Jog cancel      | `0x85` (realtime)          | `\x85`                 |
| Home            | `$H\n`                     | `$H\n`                 |
| Unlock          | `$X\n`                     | `$X\n`                 |
| Set zero        | `G10 L20 P1 X0 Y0 Z0\n`   | `G10 L20 P1 X0 Y0 Z0\n`|
| Set Z zero      | `G10 L20 P1 Z0\n`         | `G10 L20 P1 Z0\n`      |
| Go to zero      | `G90 G0 X0 Y0 Z0\n`       | `G90 G0 X0 Y0 Z0\n`   |

### 5.2 Serial Protocol (GRBL → CYD)

| Type            | Format                                            | Example |
|-----------------|---------------------------------------------------|---------|
| OK response     | `ok`                                              | `ok`    |
| Error           | `error:<code>`                                    | `error:20` |
| Alarm           | `ALARM:<code>`                                    | `ALARM:1` |
| Status report   | `<State\|WPos:x,y,z\|Bf:n,m\|FS:f,s\|Ov:a,b,c>` | `<Run\|WPos:10.00,20.00,0.00\|Bf:15,128\|FS:500,0>` |
| Startup         | `Grbl 1.1h ['$' for help]`                        | — |

### 5.3 Character-Counting Streaming

The application uses GRBL's character-counting protocol for optimal throughput:

1. Maintain a counter `grblBufFree` initialized to 128 (GRBL's RX buffer size)
2. Before sending a command of length `len` (including `\n`):
   - Check that `grblBufFree >= len`
   - Send the command
   - Subtract `len` from `grblBufFree`
   - Record `len` in a FIFO queue
3. On receiving `ok` or `error:`:
   - Pop the oldest `len` from the FIFO
   - Add it back to `grblBufFree`
4. On soft reset:
   - Reset `grblBufFree` to 128 and clear the FIFO

---

## 6. Configuration Parameters

All configuration is defined in `include/config.h`:

| Parameter             | Default Value | Description |
|-----------------------|---------------|-------------|
| `GRBL_TX_PIN`         | 27            | ESP32 GPIO for UART TX to Arduino RX |
| `GRBL_RX_PIN`         | 22            | ESP32 GPIO for UART RX from Arduino TX |
| `GRBL_BAUD_RATE`      | 115200        | Serial baud rate |
| `GRBL_RX_BUFFER`      | 128           | GRBL's receive buffer size (bytes) |
| `GCODE_LINE_MAX`      | 256           | Maximum characters per GCode line |
| `GCODE_BUFFER_LINES`  | 16            | Max in-flight commands for streaming |
| `JOG_FEED_DEFAULT_IDX` | 3             | Default index into feed rate array (1000 mm/min) |
| `JOG_Z_FEED_DIVISOR`   | 2             | Z feed = XY feed ÷ this value |
| `STATUS_POLL_MS`      | 250           | GRBL status query interval (ms) |
| `TOUCH_DEBOUNCE_MS`   | 200           | Touch input debounce time (ms) |
| `WIFI_SSID`             | `"YOUR_WIFI_SSID"` | WiFi network name to connect to |
| `WIFI_PASSWORD`         | `"YOUR_WIFI_PASSWORD"` | WiFi network password |
| `WIFI_HOSTNAME`         | `"cyd-cnc"`  | mDNS / DHCP hostname |
| `WIFI_CONNECT_TIMEOUT_MS` | 15000       | Max time to wait for WiFi connection (ms) |
| `SD_CS_PIN`           | 5             | SD card chip select |
| `TFT_BACKLIGHT_PIN`   | 21            | Display backlight GPIO |
| `TOUCH_CAL_FILE`      | `"/touch_cal.dat"` | Calibration data file path on SD card |
| `SCREEN_W`            | 320           | Display width (landscape) — override with `-DUI_SCREEN_W=xxx` |
| `SCREEN_H`            | 240           | Display height (landscape) — override with `-DUI_SCREEN_H=yyy` |

### 6.1 Machine Configuration (`/machine.cfg` on SPIFFS)

| Parameter             | Default Value | Description |
|-----------------------|---------------|-------------|
| `HOMING`              | `yes`         | Enable/disable homing (`$H`) button |
| `CLEARANCE_HEIGHT`    | `0`           | Z safe height (mm) for GO 0 moves. When > 0, return-to-zero raises Z to this height before moving XY, then lowers Z to zero. Set to 0 to move all axes simultaneously. |

### 6.2 UI Color Scheme (RGB565)

| Constant          | Value    | Color    | Usage |
|-------------------|----------|----------|-------|
| `CLR_BG`          | `0x0000` | Black    | Screen background |
| `CLR_TEXT`         | `0xFFFF` | White    | Text and button borders |
| `CLR_HEADER`      | `0x1A3A` | Dark blue | Header bars, status bars |
| `CLR_BTN`         | `0x2C7F` | Blue     | Standard buttons |
| `CLR_BTN_ACTIVE`  | `0x07E0` | Green    | Positive action buttons (START, SET 0) |
| `CLR_BTN_DANGER`  | `0xF800` | Red      | Destructive buttons (STOP, DEL) |
| `CLR_BTN_WARN`    | `0xFD20` | Orange   | Warning buttons (PAUSE, HOME) |
| `CLR_BORDER`      | `0x4A69` | Grey     | Borders, separators, muted text |
| `CLR_PROGRESS`    | `0x07E0` | Green    | Progress bar fill |
| `CLR_ACCENT`      | `0xFFE0` | Yellow   | Filename highlights, current GCode line |

---

## 7. Dependencies

### 7.1 PlatformIO Libraries

| Library              | Version      | Purpose |
|----------------------|-------------|---------|
| TFT_eSPI (Bodmer)    | ^2.5.43     | ILI9341 display driver + XPT2046 touch |
| ESPAsyncWebServer    | latest (git) | Async HTTP server for file upload |
| AsyncTCP             | latest (git) | TCP layer for async web server |
| ArduinoJson          | ^7.0.0      | JSON serialization for REST API |

### 7.2 ESP32 Arduino Core Libraries (built-in)

| Library | Purpose |
|---------|---------|
| WiFi    | Access point for web upload |
| SD      | SD card file system access |
| SPI     | SPI bus for display, touch, SD card |
| FS      | File system abstraction |

---

## 8. Resource Usage

| Resource  | Usage     | Available   | Utilization |
|-----------|-----------|-------------|-------------|
| Flash     | ~902 KB   | 3,146 KB    | 28.7%       |
| RAM       | ~46 KB    | 320 KB      | 14.0%       |

---

## 9. SPI Bus Allocation

The ESP32-2432S028R uses three separate SPI peripherals:

| SPI Bus | Peripheral   | Pins (MOSI/MISO/CLK/CS) | Frequency |
|---------|-------------|--------------------------|-----------|
| VSPI    | ILI9341 TFT | GPIO 13/–/14/15          | 55 MHz    |
| –       | XPT2046 Touch | GPIO 32/39/25/33       | 2.5 MHz   |
| HSPI    | SD Card     | GPIO 23/19/18/5          | 4 MHz     |

---

## 10. Limitations & Known Constraints

1. **Single-directory file listing:** Only files in the SD card root directory are scanned.
2. **Max 64 files:** File browser supports up to 64 GCode files.
3. **No GRBL configuration UI:** GRBL settings (`$$`) cannot be changed from the touchscreen.
4. **No work coordinate offset selection:** Only WCS P1 is used for set-zero operations.
5. **No spindle/coolant manual control:** No touchscreen buttons for M3/M5/M7/M8/M9.
6. **Fixed WiFi credentials:** Network SSID and password are compile-time constants in `config.h`.
7. **Touch calibration:** Calibration wizard runs on first boot (when no calibration file exists on SD). Can be re-triggered from the File Browser `[CAL]` button. Calibration data is persisted to `/touch_cal.dat` on the SD card.
8. **No OTA updates:** Firmware must be flashed via USB-C cable.
9. **3-axis only:** No 4th axis (A-axis) support.
10. **No file transfer over serial:** Files can only be uploaded via WiFi web interface.
11. **Display driver fixed at compile time:** Changing display hardware requires updating TFT_eSPI build flags (`-DTFT_WIDTH` / `-DTFT_HEIGHT`) in `platformio.ini` and rebuilding. The landscape resolution is derived automatically (`SCREEN_W = TFT_HEIGHT`, `SCREEN_H = TFT_WIDTH`).
12. **Landscape only:** All display sizes are assumed landscape orientation.

