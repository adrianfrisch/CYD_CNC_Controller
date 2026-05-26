# CYD CNC Controller — Test Strategy

**Version:** 1.0  
**Date:** 2026-05-26

---

## 1. Overview

This document outlines the testing strategy for the CYD CNC Controller project. Due to the embedded nature of the system (ESP32), testing is divided into multiple layers:

1. **Unit Tests** — Test individual functions and modules in isolation on a native platform
2. **Integration Tests** — Test module interactions using mocks and stubs
3. **Hardware-in-the-Loop Tests** — Test on actual ESP32 hardware with GRBL simulator
4. **Manual Acceptance Tests** — Verify end-to-end functionality with real hardware

---

## 2. Test Architecture

### 2.1 Test Framework

- **PlatformIO Unity** — Native test runner for C/C++ unit tests
- **Native Environment** — Tests run on Linux/macOS/Windows without ESP32 hardware
- **GRBL Simulator** — Python simulator for integration testing with real firmware

### 2.2 Test Directory Structure

```
test/
├── test_grbl_parser/      # GRBL status/response parsing tests (20 tests)
├── test_gcode/            # GCode comment stripping, line parsing (17 tests)
├── test_sd_filter/        # File extension filtering (34 tests)
├── test_progress/         # Progress and ETA calculation tests (20 tests)
└── test_wifi_config/      # WiFi config file parsing (23 tests)

lib/
└── testable/              # Portable testable logic module
    ├── testable_logic.h   # Function declarations
    └── testable_logic.cpp # Platform-independent implementations

tools/
├── grbl_simulator.py      # Python GRBL 1.1 simulator for testing
└── run_integration_tests.py  # Integration test runner
```

---

## 3. Test Categories

### 3.1 Unit Tests (Native)

These tests run on the development machine without requiring ESP32 hardware.

| Module | Testable Functions | Test File |
|--------|-------------------|-----------|
| **GrblComm** | `parseStatus()`, `parseState()` | `test_grbl_parser.cpp` |
| **JobStreamer** | `stripComments()`, `percentComplete()`, `estimatedRemainingMs()` | `test_gcode.cpp`, `test_progress.cpp` |
| **SDManager** | File extension filtering logic | `test_sd_filter.cpp` |
| **WebServer** | WiFi config parsing | `test_wifi_config.cpp` |

### 3.2 Integration Tests

| Test | Description | Method |
|------|-------------|--------|
| **GRBL Communication** | Full send/receive cycle | ESP32 + GRBL Simulator |
| **Job Streaming** | GCode file streaming with flow control | ESP32 + GRBL Simulator |
| **Web Upload** | File upload via HTTP | ESP32 + curl/pytest |
| **Touch Calibration** | Calibration persistence | ESP32 + manual |

### 3.3 Hardware-in-the-Loop Tests

| Test | Setup | Verification |
|------|-------|--------------|
| **SD Card I/O** | ESP32 + SD card | Files created/read correctly |
| **Display Rendering** | ESP32 + TFT | Visual inspection |
| **Touch Input** | ESP32 + Touch panel | Input coordinates map correctly |
| **UART Communication** | ESP32 + Arduino/GRBL | Commands received and executed |

---

## 4. Test Cases

### 4.1 GRBL Parser Tests

| ID | Test Case | Input | Expected Output |
|----|-----------|-------|-----------------|
| GP-01 | Parse Idle state | `"Idle"` | `GrblState::Idle` |
| GP-02 | Parse Run state | `"Run"` | `GrblState::Run` |
| GP-03 | Parse Hold state | `"Hold:0"` | `GrblState::Hold` |
| GP-04 | Parse Alarm state | `"Alarm"` | `GrblState::Alarm` |
| GP-05 | Parse unknown state | `"Foo"` | `GrblState::Unknown` |
| GP-06 | Parse full status | `"<Idle\|WPos:1.5,2.5,3.5\|Bf:15,128\|FS:500,1000>"` | Correct struct values |
| GP-07 | Parse MPos instead of WPos | `"<Run\|MPos:10,20,30>"` | MPos copied to WPos |
| GP-08 | Parse overrides | `"<Idle\|WPos:0,0,0\|Ov:80,90,100>"` | Override values set |

### 4.2 GCode Comment Stripping Tests

| ID | Test Case | Input | Expected Output |
|----|-----------|-------|-----------------|
| GC-01 | Strip parenthetical comment | `"G1 X10 (move)"` | `"G1 X10"` |
| GC-02 | Strip semicolon comment | `"G1 Y5 ; comment"` | `"G1 Y5"` |
| GC-03 | Trim leading whitespace | `"  G0 Z0"` | `"G0 Z0"` |
| GC-04 | Trim trailing whitespace | `"M3 S1000   "` | `"M3 S1000"` |
| GC-05 | Full comment line | `"; This is a comment"` | `""` |
| GC-06 | Parenthesis only | `"(comment)"` | `""` |
| GC-07 | Mixed whitespace | `"  G1 X5 (move) ; end  "` | `"G1 X5"` |
| GC-08 | No modifications needed | `"G1X10Y20Z30"` | `"G1X10Y20Z30"` |

### 4.3 Progress Calculation Tests

| ID | Test Case | Input | Expected Output |
|----|-----------|-------|-----------------|
| PC-01 | 0% progress | bytes=0, total=1000 | 0% |
| PC-02 | 50% progress | bytes=500, total=1000 | 50% |
| PC-03 | 100% progress | bytes=1000, total=1000 | 100% |
| PC-04 | Zero file size | bytes=0, total=0 | 0% |
| PC-05 | ETA calculation | 50%, elapsed=60s | remaining≈60s |
| PC-06 | ETA at 0% | 0%, elapsed=5s | remaining=0 |

### 4.4 File Extension Filter Tests

| ID | Test Case | Input | Expected Output |
|----|-----------|-------|-----------------|
| FE-01 | Valid .nc | `"part.nc"` | `true` |
| FE-02 | Valid .gcode | `"job.gcode"` | `true` |
| FE-03 | Valid .gc | `"test.gc"` | `true` |
| FE-04 | Valid .ngc | `"program.ngc"` | `true` |
| FE-05 | Valid .tap | `"drill.tap"` | `true` |
| FE-06 | Valid .cnc | `"router.cnc"` | `true` |
| FE-07 | Valid .txt | `"notes.txt"` | `true` |
| FE-08 | Invalid .jpg | `"photo.jpg"` | `false` |
| FE-09 | Invalid .exe | `"virus.exe"` | `false` |
| FE-10 | Case insensitive | `"PART.NC"` | `true` |
| FE-11 | No extension | `"README"` | `false` |

### 4.5 WiFi Config Parsing Tests

| ID | Test Case | Input | Expected Output |
|----|-----------|-------|-----------------|
| WC-01 | Valid config | `"SSID=MyWiFi\nPASS=secret"` | ssid="MyWiFi", pass="secret" |
| WC-02 | With comments | `"# comment\nSSID=Net"` | ssid="Net" |
| WC-03 | With whitespace | `"SSID = Net \nPASS = pw"` | ssid="Net", pass="pw" |
| WC-04 | Empty password | `"SSID=OpenNet\nPASS="` | ssid="OpenNet", pass="" |
| WC-05 | Missing SSID | `"PASS=secret"` | Parse fails |

---

## 5. Running Tests

### 5.1 Native Unit Tests

```bash
# Run all native tests
pio test -e native

# Run specific test
pio test -e native -f test_grbl_parser

# Run with verbose output
pio test -e native -v
```

### 5.2 Hardware Tests

```bash
# Build and upload tests to ESP32
pio test -e esp32-2432S028R

# Run with GRBL simulator
python3 tools/grbl_simulator.py --port /dev/ttyUSB0 &
pio test -e esp32-2432S028R
```

### 5.3 Integration Tests with GRBL Simulator

```bash
# Terminal 1: Start GRBL simulator
python3 tools/grbl_simulator.py --port /dev/ttyUSB0 --delay 0.05

# Terminal 2: Run ESP32 with DEBUG_SERIAL_GRBL=1
pio run -e esp32-2432S028R -t upload && pio device monitor
```

---

## 6. Continuous Integration

### 6.1 GitHub Actions Workflow

```yaml
name: CYD CNC Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - name: Install PlatformIO
        run: pip install platformio
      - name: Run Native Tests
        run: pio test -e native
```

---

## 7. Code Coverage

For native tests, code coverage can be measured using:

```bash
# Build with coverage enabled
pio test -e native --coverage

# Generate HTML report (requires lcov)
lcov --capture --directory .pio/build/native/ -output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## 8. Manual Test Checklist

### 8.1 First Boot
- [ ] Touch calibration wizard appears on first boot
- [ ] Calibration saved to SD card
- [ ] Calibration not repeated on reboot

### 8.2 File Browser
- [ ] Files listed correctly from SD card
- [ ] Pagination works (>6 files)
- [ ] File selection highlighted
- [ ] OPEN button navigates to Preview
- [ ] DEL button removes file
- [ ] RFSH button refreshes list
- [ ] JOG button navigates to Jog screen

### 8.3 Job Execution
- [ ] START begins streaming
- [ ] Progress bar updates
- [ ] Position display updates
- [ ] PAUSE sends feed hold
- [ ] RESUME continues job
- [ ] STOP aborts job
- [ ] Job completion detected

### 8.4 Jog Control
- [ ] X/Y buttons move correct axis
- [ ] Z buttons move Z axis
- [ ] Step size changes
- [ ] Feed rate changes
- [ ] SET 0 sets work zero
- [ ] SET Z0 sets Z zero only
- [ ] GO 0 moves to zero
- [ ] HOME triggers homing

### 8.5 WiFi/Web Upload
- [ ] WiFi connects to network
- [ ] Web page accessible at IP
- [ ] Drag & drop upload works
- [ ] File appears in list
- [ ] Delete from web works

---

## 9. Known Limitations

1. **Native tests cannot test hardware-specific code** — SPI, UART, GPIO interactions require hardware
2. **Touch input cannot be unit tested** — Relies on physical touch panel
3. **Display rendering cannot be verified automatically** — Requires visual inspection
4. **WiFi tests require network access** — Cannot run in isolated CI environment

---

## 10. Future Improvements

- [ ] Add hardware-in-the-loop CI runner with real ESP32
- [ ] Implement visual regression testing for UI screens
- [ ] Add fuzz testing for GRBL protocol parser
- [ ] Add stress testing for job streaming (large files)
- [ ] Add network simulation for WiFi reconnection testing

