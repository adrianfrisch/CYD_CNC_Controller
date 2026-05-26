# CNC control via CYD

## Introduction
The idea is to use CYD (Cheap yellow display) to control a CNC machine. 
CYD is a touch screen that can be programmed to display various interfaces and send commands to connected devices.
The Goal is to create a CYD program that functions similarly to the UGS (Universal GCode Sender).
The User can upload GCode files to the CYD, which will then send the commands to the CNC machine for execution.
## Features
-- File Upload: Users can upload GCode files directly to the CYD via a Web interface. The CYD will store these files for later use.
-- File Storage: The SD card on the CYD will be used to store the uploaded GCode files, allowing for easy access and management.
-- File Management: The CYD will have a simple interface for managing uploaded GCode files, allowing users to view, delete, or select files for execution.
-- File Selection and Execution: Users can select a GCode file from the CYD interface and initiate its execution on the CNC machine. The CYD will read the selected file and show a preview of the GCode commands before sending them to the CNC machine. A back buuton to return to the file selection screen will also be provided.
-- CNC Control Interface: The CYD will have a user-friendly interface that allows users to control the CNC machine, including starting, pausing, and stopping the execution of GCode commands.
-- CNC Control Interface: The application will have a JOG control interface to manually move the CNC head. The job display shall also have a "set Zero" and "Return to zero" button
-- Command Execution: The CYD will read the GCode files and send the appropriate commands to the CNC machine in real-time.
-- Job status display: The CYD will display the current status of the job, including the progress and any errors that may occur during execution. Percentage complete and an estimate of the remaining time will also be shown.
-- Job status display: A small preview of the GCode commands being executed will be shown on the CYD screen, allowing users to monitor the progress of the job.

## Project Structure (PlatformIO / ESP32 Arduino)

```
platformio.ini          — Build config, pin defs, library deps
include/config.h        — Pin mappings, colors, constants
src/
  main.cpp              — Entry point (setup/loop)
  grbl_comm.h/cpp       — GRBL serial protocol (UART2, status parsing, streaming)
  sd_manager.h/cpp      — SD card file I/O (list, read, delete GCode files)
  job_streamer.h/cpp    — GCode streaming with character-counting flow control
  web_server.h/cpp      — WiFi AP + async web server for drag & drop file upload
  ui/
    ui_manager.h/cpp    — TFT_eSPI display, touch handling, screen switching
    screen_filebrowser  — File list with paging, open/delete/refresh
    screen_preview      — GCode preview before starting job
    screen_job          — Job progress, position, pause/resume/stop
    screen_jog          — Manual XYZ jog pad, step size, set/go zero, home
```

## Wiring (CYD ↔ Arduino GRBL)

| Signal | CYD GPIO | Arduino |
|--------|----------|---------|
| TX→RX  | GPIO 27  | D0 (RX) |
| RX←TX  | GPIO 22  | D1 (TX) |
| GND    | GND      | GND     |

## Build & Flash

```bash
pio run                  # Build
pio run -t upload        # Flash to CYD via USB-C
pio device monitor       # Debug serial (115200 baud)
```

## WiFi File Upload

Set your WiFi credentials in `include/config.h`, then access **http://&lt;device-ip&gt;/** from any browser on the same network to upload GCode files.
