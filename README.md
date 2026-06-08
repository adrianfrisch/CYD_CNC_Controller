# CYD CNC Controller

A standalone touchscreen GCode sender for GRBL-based CNC machines, built on the ESP32-2432S028R "Cheap Yellow Display" (CYD).

Upload GCode files over WiFi, preview them on the touchscreen, and run jobs — no PC required.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

> **⚠️ DISCLAIMER: This project is a work in progress and is NOT functional yet.**
> It is under active development. Features may be incomplete, untested, or broken.
> Do **not** use this to control a real CNC machine at this stage — you risk damage
> to your equipment or injury. Use at your own risk.

## Current status of the project
I am currently testing the controller with my GRBL firmware based CNC machines. 
I successfully ran a couple of .nc files. But test and use at your own risk. 
The configuration for your CYD needs to be adjusted in the platformio.ini file to match your model. 

--- 

## Know Issues
- Home is untested. My CNC machine does not support homing

## Improvement ideas
- Add buttons on the jog screen for diagonal movements

---

## Features

- **File Upload** — Drag-and-drop GCode files via a built-in WiFi web interface
- **File Browser** — Browse, select, and delete GCode files stored on the SD card
- **GCode Preview** — View the first lines of a file before sending it to the machine
- **Job Execution** — Stream GCode to GRBL with real-time progress, position, and ETA display
- **Job Control** — Start, pause, resume, and stop jobs from the touchscreen
- **Jog Control** — Move X, Y, Z axes manually with configurable step sizes (0.1–50 mm)
- **Set / Go Zero** — Set the current position as work zero or rapid-move back to zero
- **Home** — Trigger GRBL homing cycle from the touchscreen

---

## Hardware

### What You Need

| Item | Description |
|------|-------------|
| **CYD Board** | ESP32-2432S028R — ESP32 + 2.8" TFT + touchscreen + SD slot (~$10) |
| **Arduino + GRBL** | Arduino Uno R3 running GRBL 1.1 firmware |
| **CNC Shield** | Arduino CNC Shield v3.0 (with A4988/DRV8825 stepper drivers) |
| **Micro-SD Card** | ≤ 32 GB, formatted FAT32 |
| **Dupont Wires** | 3× female-to-female jumper wires (10–20 cm) |
| **USB Cables** | USB-C for CYD + USB-B for Arduino (power & flashing) |
| **Power Supply** | 12–36V for CNC Shield (motors), 5V USB for CYD and Arduino |

### Wiring Diagram

```
                            ┌────────────────────────────────────────────────┐
  ┌─────────────────────┐   │  Arduino CNC Shield v3.0                       │
  │  CYD ESP32-2432S028 │   │  (plugged on top of Arduino Uno)               │
  │                     │   │                                                │
  │  ┌───────────────┐  │   │  Stepper Drivers    12-36V                     │
  │  │  2.8" TFT     │  │   │  ┌───┐ ┌───┐ ┌───┐  ↓                          │
  │  │  Touchscreen  │  │   │  │ X │ │ Y │ │ Z │ [PWR]                       │
  │  │  320×240      │  │   │  └─┬─┘ └─┬─┘ └─┬─┘                             │
  │  └───────────────┘  │   │    │     │     │   To stepper motors           │
  │                     │   ├────┴─────┴─────┴───────────────────────────────┤
  │  [USB-C]  [SD Card] │   │  Arduino Uno R3 (GRBL 1.1)                     │
  │   Power    GCode    │   │                                                │
  │   & Flash  Storage  │   │  [USB-B]                                       │
  │                     │   │   Power & Flash                                │
  │  P3 / CN1 Header:   │   │                                                │
  │  ┌─────────────┐    │   │   Pin Header:                                  │
  │  │ GND         │────────────│ GND                                        │
  │  │ GPIO 27 (TX)│────────────│ D0 (RX)                                    │
  │  │ GPIO 22 (RX)│────────────│ D1 (TX)                                    │
  │  │ 3V3         │    │   │   │                                            │
  │  └─────────────┘    │   │   │                                            │
  └─────────────────────┘   └───┴────────────────────────────────────────────┘
         ↑                         ↑                    ↑
      5V USB-C                  5V USB-B            12-36V PSU
      (phone charger)          (or USB hub)         (motor power)
```

### Wire Connections (3 wires only)

| Wire | From (CYD P3 Header) | To (Arduino Uno) | Color (suggested) |
|------|----------------------|-------------------|-------------------|
| 1    | **GPIO 27** (TX)     | **D0** (RX)       | 🟡 Yellow          |
| 2    | **GPIO 22** (RX)     | **D1** (TX)       | 🟢 Green           |
| 3    | **GND**              | **GND**           | ⚫ Black           |

### Where to Connect on the Arduino + CNC Shield

The CNC Shield sits on top of the Arduino Uno, but the serial pins **D0** and **D1** are still accessible:

```
  Arduino Uno (bottom board) — Pin header along the edge:

  ┌─────────────────────────────────────────────────────┐
  │  DIGITAL PINS                                       │
  │  ┌────┬────┬────┬────┬────┬────┬────┬────┬────┐     │
  │  │ D0 │ D1 │ D2 │ D3 │ D4 │ D5 │ D6 │ D7 │ ...│     │
  │  │ RX │ TX │    │    │    │    │    │    │    │     │
  │  └──┬─┴──┬─┴────┴────┴────┴────┴────┴────┴────┘     │
  │     │    │                                          │
  │     │    └── Green wire ← CYD GPIO 22 (RX)          │
  │     └─────── Yellow wire ← CYD GPIO 27 (TX)         │
  │                                                     │
  │  GND pin (next to 5V in POWER header) ← Black wire  │
  └─────────────────────────────────────────────────────┘
```

> **⚠ Important:** The CNC Shield passes through D0/D1 — if the shield's pin headers block access, solder wires directly to the Arduino D0/D1 pins underneath, or use the exposed serial header on some CNC Shield v3 boards.

> **⚠ Flashing:** Disconnect the TX/RX wires (Yellow + Green) when uploading firmware to the Arduino via USB — they share the same UART.

> **Note:** No logic-level shifter is needed. ESP32 3.3V output exceeds the ATmega328P's logic-high threshold.

### Power

| Board | Power Source | Notes |
|-------|-------------|-------|
| CYD | USB-C (5V) | Phone charger, USB hub, or PC |
| Arduino Uno | USB-B (5V) | Separate USB cable or powered via CNC Shield |
| CNC Shield | 12–36V DC PSU | Powers stepper motors only — voltage depends on your motors |

> All three power sources can be independent. The CYD and Arduino share only GND + serial (3 wires).

---

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE plugin for VS Code / CLion)
- GRBL 1.1 flashed on the Arduino ([grbl/grbl](https://github.com/grbl/grbl))
- A micro-SD card with GCode files (`.nc`, `.gcode`, `.gc`, `.ngc`, `.tap`, `.cnc`, `.txt`)

### Build & Flash

```bash
# Clone / open the project
cd CYD-CNC-Controller

# Build
pio run

# Flash to the CYD board via USB-C
pio run -t upload

# Monitor debug output (optional)
pio device monitor
```

### First Boot

1. Insert a micro-SD card with GCode files into the CYD
2. Wire the CYD to the Arduino as shown above
3. Power both boards
4. **First boot:** The touch calibration wizard runs — touch the 4 corner arrows precisely. Calibration is saved to the SD card and won't run again unless you delete `/touch_cal.dat` or tap `[CAL]`.
5. The CYD displays the **File Browser** screen
6. The status bar shows **GRBL OK** when the connection is established

---

## WiFi File Upload

The CYD connects to your existing WiFi network for wireless file management — no SD card swapping needed.

1. Set your WiFi credentials in `include/config.h`:
   ```cpp
   #define WIFI_SSID     "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```
2. Flash the firmware and power on the CYD
3. Check the serial monitor for the assigned IP address (e.g., `192.168.1.42`)
4. On any device on the same network, open **http://&lt;ip-address&gt;/** in a browser
5. Drag and drop a GCode file or use the file picker
6. The file is saved directly to the SD card
7. Tap **RFSH** on the CYD to see the new file

> **Note:** If the WiFi connection is lost, the CYD will automatically reconnect every 30 seconds. The web server remains registered and will resume serving as soon as the connection is restored.

---

## Usage Guide

### Screen Navigation

```
File Browser  ──OPEN──→  Preview  ──START──→  Job
     │                      │                   │
     │←─────BACK────────────┘                   │
     │                                          │
     ↕ JOG / FILES                    FILES ────┘
   Jog Control                   (when job idle)
```

### File Upload

 - The file browser shows the IP of the device on your local network
 - open https://YOU_IP/ in your browser.
 - The file upload page is pretty simple and should be self explanatory

### File Browser

- Tap a file to **select** it (highlighted in blue)
- Tap **OPEN** to preview the file before running
- Tap **DEL** to delete the selected file
- Use **UP / DOWN** to page through the file list
- Tap **JOG** to switch to manual jog mode
- Tap **RFSH** to rescan the SD card
- Tap **[CAL]** in the status bar to re-run touch calibration

### GCode Preview

- Shows filename, total lines, file size, and the first 10 lines of GCode
- Tap **START** to begin streaming the file to the CNC
- Tap **< BACK** to return to file selection

### Job Screen

- **Progress bar** — visual percentage with elapsed time and ETA
- **Position** — live X, Y, Z work coordinates
- **Current line** — shows the GCode command being executed
- **PAUSE** — sends GRBL feed hold (`!`)
- **RESUME** — sends GRBL cycle resume (`~`)
- **STOP** — sends GRBL soft reset, aborts the job
- **FILES** — return to File Browser (only when job is not running)

### Jog Control

- **XY pad** — tap Y+, Y-, X-, X+ to jog in that direction
- **Z buttons** — Z+ (up) and Z- (down) at half the XY feed rate
- **Z microstep** — Z+.01 / Z-.01 buttons (0.01mm at 50 mm/min) for precise Z touch-off zeroing
- **Step size** — tap **–** / **+** to cycle through 0.1, 0.5, 1.0, 5.0, 10.0, 50.0 mm
- **Feed rate** — tap **–** / **+** to cycle through 50, 100, 500, 1000, 2000, 3000, 5000 mm/min; Z feed is shown automatically (XY ÷ 2)
- **SET 0** — set current position as work zero (all axes)
- **SET Z0** — set current Z position as Z work zero only (for after touch-off)
- **GO 0** — rapid move to work zero
- **HOME** — trigger GRBL homing cycle (`$H`)
- **FILES** — return to File Browser

---

## Project Structure

```
├── platformio.ini              Build config, TFT_eSPI pin setup, library deps
├── include/
│   └── config.h                Pin mappings, colors, timing constants
├── src/
│   ├── main.cpp                Entry point — setup() and loop()
│   ├── grbl_comm.h/cpp         GRBL serial protocol, status parsing, streaming
│   ├── sd_manager.h/cpp        SD card file I/O — list, open, delete
│   ├── job_streamer.h/cpp      GCode file → GRBL streaming with flow control
│   ├── web_server.h/cpp        WiFi AP + async web server for file upload
│   └── ui/
│       ├── ui_manager.h/cpp    TFT display, touch input, screen switching, calibration
│       ├── screen_calibration  4-corner touch calibration wizard (first boot + on-demand)
│       ├── screen_filebrowser  File list with paging and selection
│       ├── screen_preview      GCode preview before execution
│       ├── screen_job          Job progress and control
│       └── screen_jog          Manual XYZ jogging interface
├── lib/testable/               Platform-independent logic (unit-testable on PC)
├── test/                       Unity test suites (114 tests across 5 suites)
├── tools/                      GRBL simulator and integration test runner
├── ARCHITECTURE.md             System architecture and data flow diagrams
├── CONTRIBUTING.md             Development workflow and code conventions
├── SPECIFICATION.md            Detailed software specification
├── TEST_STRATEGY.md            Testing approach and test case catalog
├── CLAUDE.md                   AI assistant context file
├── .github/copilot-instructions.md  GitHub Copilot custom instructions
└── README.md                   This file
```

---

## Configuration

Key settings can be changed in `include/config.h`:

| Setting | Default | Description |
|---------|---------|-------------|
| `WIFI_SSID` | `"YOUR_WIFI_SSID"` | WiFi network name to connect to |
| `WIFI_PASSWORD` | `"YOUR_WIFI_PASSWORD"` | WiFi password |
| `WIFI_CONNECT_TIMEOUT_MS` | 15000 | Max time to wait for WiFi connection (ms) |
| `GRBL_BAUD_RATE` | 115200 | Must match GRBL firmware setting |
| `STATUS_POLL_MS` | 250 | How often to query GRBL status (ms) |
| `TOUCH_DEBOUNCE_MS` | 200 | Touch debounce delay (ms) |

See `SPECIFICATION.md` for the full parameter reference.

---

## Build Output

```
RAM:   [=         ]  14.0% (46 KB / 320 KB)
Flash: [===       ]  28.7% (902 KB / 3,146 KB)
```

---

## Testing with GRBL Simulator

A Python-based GRBL simulator is included for testing without a real CNC machine.
It simulates a GRBL 1.1 controller: acknowledges GCode commands, responds to status
queries with simulated positions, and handles realtime commands.

```bash
# Install dependency (if needed)
pip3 install pyserial

# Run simulator on the serial port connected to the CYD
python3 tools/grbl_simulator.py --port /dev/ttyUSB0 --baud 115200 --delay 0.2
```

**Options:**
| Flag | Default | Description |
|------|---------|-------------|
| `--port` | `/dev/ttyUSB0` | Serial port to listen on |
| `--baud` | `115200` | Baud rate |
| `--delay` | `0.2` | Seconds to wait before acknowledging each command |

The simulator ignores ESP32 debug messages (lines starting with `[`) and only
responds to valid GRBL commands.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **"NO GRBL" on status bar** | Check wiring (TX→RX, RX←TX, GND). Verify Arduino is powered and GRBL is running at 115200 baud. |
| **"NO SD CARD"** | Insert a FAT32-formatted micro-SD card (≤ 32 GB). Try reformatting if it doesn't mount. |
| **No files shown** | Files must have a supported extension: `.nc`, `.gcode`, `.gc`, `.ngc`, `.tap`, `.cnc`, `.txt`. Files must be in the root directory. |
| **Touch not responding** | Tap `[CAL]` in the File Browser status bar to re-run the touch calibration wizard. If touch is too far off to tap the button, delete `/touch_cal.dat` from the SD card and reboot — the calibration wizard will run automatically. |
| **Can't flash Arduino** | Disconnect the TX/RX wires from Arduino D0/D1 before uploading firmware via USB. |
| **WiFi not connecting** | Verify `WIFI_SSID` and `WIFI_PASSWORD` in `config.h`. Check serial monitor for connection status. The CYD must be within range of your WiFi router. Reconnection is attempted every 30 seconds. |
| **Upload fails on web page** | Ensure the SD card is mounted. Check serial monitor for error messages. Try a smaller file first. |
| **Job freezes mid-run** | Monitor GRBL state — it may be in `Hold` or `Alarm`. Check for `error:` messages on the serial monitor. Try RESUME or STOP. |

---

## Future Improvements

- [ ] GRBL settings viewer/editor (`$$` commands)
- [ ] Subdirectory navigation on SD card
- [ ] OTA firmware updates over WiFi
- [ ] Spindle and coolant manual controls (M3/M5/M7/M8/M9)
- [ ] Work coordinate system selection (G54–G59)
- [ ] 2D toolpath visualization on the preview screen
- [ ] Bluetooth serial as an alternative to WiFi file upload
- [ ] GRBL alarm code descriptions on-screen

---

## License

MIT License — see [LICENSE](LICENSE) for details.

