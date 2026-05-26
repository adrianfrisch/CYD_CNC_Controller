#pragma once
// =============================================================================
// XPT2046 Software SPI Touch Driver
//
// Bit-bangs SPI to the XPT2046 touch controller on the CYD, avoiding
// hardware SPI bus conflicts with the display and SD card.
//
// The CYD ESP32-2432S028R has 3 SPI devices on 3 different pin sets:
//   Display (ILI9341):  VSPI (SPI3) — pins 13/14/15
//   SD Card:            HSPI (SPI2) — pins 23/19/18/5
//   Touch (XPT2046):    Software SPI — pins 32/39/25/33
//
// Touch detection uses Z-pressure measurement (no IRQ dependency).
// =============================================================================

#include <Arduino.h>
#include "config.h"

class XPT2046_Soft {
public:
    void begin();
    bool touched();
    bool readRaw(int16_t& x, int16_t& y);

private:
    uint16_t readChannel(uint8_t cmd);
    void spiWrite(uint8_t data);
    uint16_t spiRead12();

    static constexpr int Z_THRESHOLD = 300;  // Minimum pressure to register touch
};
