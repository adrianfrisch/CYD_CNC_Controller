# Architecture — CYD CNC Controller

> Concise architecture reference. See SPECIFICATION.md for full details.

## System Diagram

```
┌──────────────────────────────────────────────────────────┐
│  ESP32-2432S028R (CYD)                                   │
│                                                          │
│  ┌────────────┐  ┌────────────┐  ┌────────────────────┐  │
│  │ UIManager   │  │ WebUpload  │  │ SDManager          │  │
│  │ 5 screens   │  │ Server     │  │ FAT32 file I/O     │  │
│  │ touch input │  │ WiFi STA   │  │ max 64 files       │  │
│  └──────┬─────┘  └─────┬──────┘  └──────┬─────────────┘  │
│         │              │                │                 │
│  ┌──────┴──────────────┴────────────────┴──────────────┐  │
│  │                  main.cpp                           │  │
│  │            setup() / loop()                         │  │
│  └──────┬──────────────────────────────────────────────┘  │
│         │                                                │
│  ┌──────┴──────────┐  ┌────────────────────────────────┐  │
│  │  JobStreamer     │  │  GrblComm                      │  │
│  │  file → GRBL    │──│  UART2 serial protocol         │  │ 
│  │  flow control   │  │  character-counting streaming  │  │
│  │  progress/ETA   │  │  status parsing                │  │
│  └─────────────────┘  └──────────┬─────────────────────┘  │
│                                  │ GPIO 27 TX / 22 RX     │
└──────────────────────────────────┼────────────────────────┘
                                   │ 3-wire UART (TX/RX/GND)
                    ┌──────────────┴──────────────┐
                    │  Arduino Uno + CNC Shield   │
                    │  GRBL 1.1 @ 115200 baud     │
                    │  Controls: X/Y/Z steppers   │
                    └─────────────────────────────┘
```

## Module Dependency Graph

```
config.h ←── all modules
                    
main.cpp ──→ grbl_comm ──→ config.h
         ──→ sd_manager ──→ config.h
         ──→ job_streamer ──→ grbl_comm, sd_manager, config.h
         ──→ web_server ──→ sd_manager, config.h
         ──→ ui_manager ──→ all of the above
              ├── screen_calibration
              ├── screen_filebrowser ──→ sd_manager
              ├── screen_preview ──→ sd_manager
              ├── screen_job ──→ job_streamer, grbl_comm
              └── screen_jog ──→ grbl_comm
```

## Data Flow: Job Execution

```
SD Card (.nc file)
    │
    ▼ readNextLine()
JobStreamer ──stripComments()──→ clean GCode line
    │
    ▼ sendLine() [if buffer space available]
GrblComm ──character-counting──→ UART2 TX (GPIO 27)
    │
    ▼ serial bytes
Arduino GRBL ──executes──→ stepper motors
    │
    ▼ "ok" / "error:N" / "<Status|...>"
GrblComm ──parseLine()──→ updates GrblStatus
    │
    ▼ status callbacks
UIManager ──redraws──→ Job screen (progress, position, ETA)
```

## Data Flow: WiFi File Upload

```
Browser (http://<ip>/)
    │
    ▼ POST /upload (multipart)
ESPAsyncWebServer
    │
    ▼ writes chunks to SD
SDManager ──→ /filename.nc on SD card
    │
    ▼ user taps RFSH
ScreenFileBrowser ──→ rescans SD, updates file list
```

## SPI Bus Allocation

```
VSPI ──→ ILI9341 TFT Display  (GPIO 13/14/15,  55 MHz)
Soft SPI → XPT2046 Touch       (GPIO 32/39/25/33, ~2.5 MHz)
HSPI ──→ SD Card               (GPIO 23/19/18/5,  4 MHz)
```

## Screen Navigation State Machine

```
                    ┌─────────────┐
           ┌──CAL──│ Calibration  │ (first boot or [CAL] button)
           │       └──────┬──────┘
           │              │ done
           ▼              ▼
     ┌─────────────┐  OPEN  ┌──────────┐  START  ┌──────────┐
     │ FileBrowser  │──────→│ Preview  │───────→│   Job    │
     │              │←──────│          │        │          │
     └──────┬──────┘  BACK  └──────────┘        └────┬─────┘
       JOG ↕ FILES                              FILES│(idle)
     ┌──────┴──────┐                                 │
     │    Jog      │←────────────────────────────────┘
     └─────────────┘
```

## Streaming Protocol Detail

```
GRBL RX Buffer: 128 bytes
Max in-flight commands: 16

Send:  Track grblBufFree (starts at 128)
       if (grblBufFree >= len(cmd+\n)):
           send cmd
           grblBufFree -= len
           push len to FIFO

Recv:  On "ok" or "error:N":
           pop oldest len from FIFO
           grblBufFree += len

Reset: On soft reset (0x18):
           grblBufFree = 128
           clear FIFO
```

