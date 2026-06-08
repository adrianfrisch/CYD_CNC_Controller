// =============================================================================
// CYD CNC Controller — Main Entry Point
//
// Board: ESP32-2432S028R (Cheap Yellow Display)
// Purpose: Touchscreen GCode sender for GRBL CNC controllers
//
// Connections:
//   CYD GPIO27 (TX) → Arduino D0 (RX)
//   CYD GPIO22 (RX) ← Arduino D1 (TX)
//   CYD GND         ↔ Arduino GND
//
// WiFi STA: Connects to your network → http://<assigned-ip>/ for file upload
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "grbl_comm.h"
#include "sd_manager.h"
#include "job_streamer.h"
#include "web_server.h"
#include "machine_config.h"
#include "ui/ui_manager.h"

void setup() {
    // Debug serial (USB) — skip if GRBL shares the same UART0 pins
#ifndef GRBL_SHARED_SERIAL
    DebugSerial.begin(115200);
    delay(500);
    DebugSerial.println();
    DebugSerial.println("========================================");
    DebugSerial.println("  CYD CNC Controller v1.0");
    DebugSerial.println("========================================");
#endif

    // Initialize display & UI (first, so user sees something immediately)
    DebugSerial.println("[INIT] Display & UI...");
    ui.begin();

    // Initialize SD card
    DebugSerial.println("[INIT] SD Card...");
    if (sdCard.begin()) {
        DebugSerial.printf("[INIT] SD OK — %llu MB total, %llu MB used\n",
                      sdCard.totalBytes() / (1024 * 1024),
                      sdCard.usedBytes() / (1024 * 1024));
    } else {
        DebugSerial.println("[INIT] SD Card FAILED — file features disabled");
    }

    // Now that SD is ready, check for touch calibration
    ui.checkCalibrationAfterSD();

    // Initialize GRBL serial
    DebugSerial.println("[INIT] GRBL UART2...");
    grbl.begin();

    // Initialize job streamer
    DebugSerial.println("[INIT] Job streamer...");
    job.begin();

    // Load machine configuration
    DebugSerial.println("[INIT] Machine config...");
    machineConfig.begin();


    // Initialize WiFi & web server
    DebugSerial.println("[INIT] WiFi & Web Server...");
    webServer.begin();
    if (webServer.isConnected()) {
        DebugSerial.printf("[INIT] Upload files at: http://%s/\n", webServer.getIP().c_str());
    }

    DebugSerial.println("[INIT] Ready!");
    DebugSerial.println("========================================");
}

void loop() {
    // Process GRBL serial communication
    grbl.loop();

    // Stream GCode lines to GRBL if job is active
    job.loop();

    // Handle UI (touch + screen refresh)
    ui.loop();

    // Monitor WiFi connection, reconnect if needed
    webServer.loop();
}

