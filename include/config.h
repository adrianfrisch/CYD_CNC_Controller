#pragma once
// =============================================================================
// CYD CNC Controller — Configuration & Pin Definitions
// Board: ESP32-2432S028R (Cheap Yellow Display)
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// GRBL Serial (UART2 → Arduino D0/D1)
// -----------------------------------------------------------------------------
#define GRBL_TX_PIN       27   // CYD GPIO27 → Arduino D0 (RX)
#define GRBL_RX_PIN       22   // CYD GPIO22 ← Arduino D1 (TX)
#define GRBL_BAUD_RATE    115200
#define GRBL_SERIAL       Serial2

// -----------------------------------------------------------------------------
// SD Card (HSPI)
// -----------------------------------------------------------------------------
#define SD_CS_PIN         5
#define SD_MOSI_PIN       23
#define SD_MISO_PIN       19
#define SD_SCK_PIN        18

// -----------------------------------------------------------------------------
// Display (ILI9341 on VSPI — handled by TFT_eSPI build flags)
// -----------------------------------------------------------------------------
#define TFT_BACKLIGHT_PIN 21

// -----------------------------------------------------------------------------
// Touch (XPT2046 on separate SPI — handled by TFT_eSPI build flags)
// -----------------------------------------------------------------------------
#define TOUCH_CAL_FILE    "/touch_cal.dat"   // Calibration data file on SD card

// -----------------------------------------------------------------------------
// WiFi — Station mode (connect to existing network)
// -----------------------------------------------------------------------------
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"
#define WIFI_HOSTNAME     "cyd-cnc"
#define WIFI_CONNECT_TIMEOUT_MS  15000  // Max time to wait for connection

// -----------------------------------------------------------------------------
// UI Layout (320x240 landscape)
// -----------------------------------------------------------------------------
#define SCREEN_W          320
#define SCREEN_H          240

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

