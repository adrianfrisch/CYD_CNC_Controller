#pragma once
// =============================================================================
// XPT2046 Touch Driver
//
// On boards where touch shares the hardware SPI bus with SD card (e.g.
// ESP32-4827S043R), uses hardware SPI transactions with a separate CS pin.
// On boards with dedicated touch pins (e.g. CYD 2432), uses software SPI.
// =============================================================================

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// Detect shared SPI bus: touch and SD use same CLK/MOSI/MISO pins
#if (TOUCH_CLK_PIN == SD_SCK_PIN) && (TOUCH_MOSI_PIN == SD_MOSI_PIN) && (TOUCH_MISO_PIN == SD_MISO_PIN)
    #define TOUCH_SHARED_SPI 1
#else
    #define TOUCH_SHARED_SPI 0
#endif

class XPT2046_Soft {
public:
    void begin();
    bool touched();
    bool readRaw(int16_t& x, int16_t& y);

private:
    uint16_t readChannel(uint8_t cmd);

    static constexpr int Z_THRESHOLD = 300;  // Minimum pressure to register touch

#if TOUCH_SHARED_SPI
    SPIClass* _spi = nullptr;
#endif
};
