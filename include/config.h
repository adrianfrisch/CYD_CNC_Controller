#pragma once
// =============================================================================
// CYD CNC Controller — Configuration & Pin Definitions
// Board: ESP32-2432S028R (Cheap Yellow Display)
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// DEVELOP MODE — set to 1 to enable debug logging
// DEBUG_SERIAL_GRBL — set to 1 to route GRBL communication over USB Serial
//   instead of UART2. This lets you test with grbl_simulator.py connected
//   to the CYD's USB port. Debug log messages are prefixed with [DBG]/[UI]/etc.
//   and the simulator ignores lines starting with '['.
//   Set both to 0 for production with a real CNC machine.
// -----------------------------------------------------------------------------
#define DEVELOP_MODE          0
#define DEBUG_SERIAL_GRBL     0

// Debug logging macros (active only in DEVELOP_MODE)
#if DEVELOP_MODE
  #define DBG(fmt, ...)   Serial.printf("[DBG] " fmt "\n", ##__VA_ARGS__)
#else
  #define DBG(fmt, ...)   ((void)0)
#endif

// -----------------------------------------------------------------------------
// GRBL Serial (UART2 → Arduino D0/D1)
// -----------------------------------------------------------------------------
#ifndef GRBL_TX_PIN
  #define GRBL_TX_PIN       27   // CYD GPIO27 → Arduino D0 (RX)
#endif
#ifndef GRBL_RX_PIN
  #define GRBL_RX_PIN       22   // CYD GPIO22 ← Arduino D1 (TX)
#endif
#define GRBL_BAUD_RATE    115200
#define GRBL_SERIAL       Serial2

// -----------------------------------------------------------------------------
// SD Card (HSPI)
// -----------------------------------------------------------------------------
#ifndef SD_CS_PIN
  #define SD_CS_PIN         5
#endif
#ifndef SD_MOSI_PIN
  #define SD_MOSI_PIN       23
#endif
#ifndef SD_MISO_PIN
  #define SD_MISO_PIN       19
#endif
#ifndef SD_SCK_PIN
  #define SD_SCK_PIN        18
#endif

// -----------------------------------------------------------------------------
// Display (handled by TFT_eSPI or LovyanGFX build flags)
// -----------------------------------------------------------------------------
#ifndef TFT_BACKLIGHT_PIN
  #define TFT_BACKLIGHT_PIN 21
#endif

// -----------------------------------------------------------------------------
// Touch (XPT2046 on separate SPI bus from display)
// -----------------------------------------------------------------------------
#ifndef TOUCH_CS_PIN
  #define TOUCH_CS_PIN      33
#endif
#ifndef TOUCH_IRQ_PIN
  #define TOUCH_IRQ_PIN     36
#endif
#ifndef TOUCH_MOSI_PIN
  #define TOUCH_MOSI_PIN    32
#endif
#ifndef TOUCH_MISO_PIN
  #define TOUCH_MISO_PIN    39
#endif
#ifndef TOUCH_CLK_PIN
  #define TOUCH_CLK_PIN     25
#endif
#define TOUCH_CAL_FILE    "/touch_cal.dat"   // Calibration data file on SD card

// Calibration data: maps raw touch coordinates to screen pixels
struct TouchCalData {
    int16_t rawXMin;   // Raw X at screen left
    int16_t rawXMax;   // Raw X at screen right
    int16_t rawYMin;   // Raw Y at screen top
    int16_t rawYMax;   // Raw Y at screen bottom
    uint32_t magic;    // Validation marker (0xCAL1)
};
#define TOUCH_CAL_MAGIC  0x43414C31  // "CAL1"

// -----------------------------------------------------------------------------
// WiFi — Station mode (connect to existing network)
// Credentials are loaded from /wifi.cfg on the SD card at boot.
// The defines below are compile-time fallbacks only.
// -----------------------------------------------------------------------------
#define WIFI_CONFIG_FILE  "/wifi.cfg"
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"
#define WIFI_HOSTNAME     "cyd-cnc"
#define WIFI_CONNECT_TIMEOUT_MS  15000  // Max time to wait for connection

// -----------------------------------------------------------------------------
// UI Layout — resolution derived from TFT_eSPI build flags (landscape mode)
// TFT_WIDTH/TFT_HEIGHT are portrait dimensions set in platformio.ini.
// We swap them here to get landscape: SCREEN_W = TFT_HEIGHT, SCREEN_H = TFT_WIDTH.
// -----------------------------------------------------------------------------
#ifndef TFT_WIDTH
  #define TFT_WIDTH       240
#endif
#ifndef TFT_HEIGHT
  #define TFT_HEIGHT      320
#endif
#define SCREEN_W          TFT_HEIGHT
#define SCREEN_H          TFT_WIDTH

// Colours (RGB565)
#define CLR_BG            0x0000  // Black
#define CLR_TEXT          0xFFFF  // White
#define CLR_HEADER        0x1A3A  // Dark blue
#define CLR_BTN           0x2C7F  // Blue
#define CLR_BTN_ACTIVE    0x07E0  // Green
#define CLR_BTN_DANGER    0xF800  // Red
#define CLR_BTN_WARN      0xFD20  // Orange
#define CLR_BORDER        0x4A69  // Grey
#define CLR_PROGRESS      0x07E0  // Green
#define CLR_ACCENT        0xFFE0  // Yellow

// -----------------------------------------------------------------------------
// GCode streaming
// -----------------------------------------------------------------------------
#define GCODE_LINE_MAX    256    // Max chars per GCode line
#define GCODE_BUFFER_LINES 16   // Lines buffered ahead
#define GRBL_RX_BUFFER    128   // GRBL's receive buffer size

// -----------------------------------------------------------------------------
// Jog feed rates (mm/min)
// -----------------------------------------------------------------------------
#define JOG_FEED_DEFAULT_IDX  3    // Index into feed rate array (default: 1000)
#define JOG_Z_FEED_DIVISOR    2    // Z feed = XY feed / this value

// -----------------------------------------------------------------------------
// Job status polling
// -----------------------------------------------------------------------------
#define STATUS_POLL_MS    250    // Query GRBL status every 250ms
#define TOUCH_DEBOUNCE_MS 200   // Touch debounce

