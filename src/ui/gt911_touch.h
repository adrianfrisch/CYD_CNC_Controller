#pragma once
// =============================================================================
// GT911 Capacitive Touch Driver (I2C)
// Used on ESP32-8048S070C and similar boards with capacitive touch panels.
// Provides the same interface as XPT2046_Soft for UIManager compatibility.
// =============================================================================

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// GT911 I2C address (determined by INT pin state during reset)
#define GT911_ADDR        0x5D
#define GT911_ADDR_ALT    0x14

// GT911 registers
#define GT911_REG_STATUS  0x814E
#define GT911_REG_TOUCH1  0x8150
#define GT911_REG_CONFIG  0x8047
#define GT911_REG_X_RES   0x8048
#define GT911_REG_Y_RES   0x804A

#ifndef GT911_SDA_PIN
  #define GT911_SDA_PIN   19
#endif
#ifndef GT911_SCL_PIN
  #define GT911_SCL_PIN   20
#endif
#ifndef GT911_INT_PIN
  #define GT911_INT_PIN   18
#endif
#ifndef GT911_RST_PIN
  #define GT911_RST_PIN   38
#endif

class GT911_Touch {
public:
    void begin();
    bool readRaw(int16_t& x, int16_t& y);

private:
    uint8_t _addr = GT911_ADDR;
    uint16_t _maxX = 800;
    uint16_t _maxY = 480;
    uint16_t _nativeX = 800;  // GT911 firmware-configured resolution
    uint16_t _nativeY = 480;

    void reset();
    bool readTouch(uint16_t& x, uint16_t& y);
    void writeReg(uint16_t reg, uint8_t val);
    uint8_t readReg(uint16_t reg);
    uint8_t readRegs(uint16_t reg, uint8_t* buf, uint8_t len);
};

