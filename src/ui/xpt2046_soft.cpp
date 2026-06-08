// =============================================================================
// XPT2046 Touch Driver — Implementation
//
// Supports two modes:
// 1. Shared hardware SPI (TOUCH_SHARED_SPI=1): Uses SPI transactions on the
//    same bus as SD card, with separate CS. Required when pins are shared.
// 2. Software bit-bang SPI (TOUCH_SHARED_SPI=0): Bit-bangs on dedicated pins.
//    Used on CYD 2432 where touch has its own SPI bus.
// =============================================================================

#include "xpt2046_soft.h"

#define CMD_X   0xD1
#define CMD_Y   0x91
#define CMD_Z1  0xB1
#define CMD_Z2  0xC1
#define CMD_PWD 0xD0

#if TOUCH_SHARED_SPI
// =============================================================================
// Hardware SPI mode (shared bus with SD card)
// =============================================================================

static SPISettings touchSPISettings(1000000, MSBFIRST, SPI_MODE0);

void XPT2046_Soft::begin() {
    pinMode(TOUCH_CS_PIN, OUTPUT);
    digitalWrite(TOUCH_CS_PIN, HIGH);

    // Use the same SPI instance as SD card (already initialized)
#if CONFIG_IDF_TARGET_ESP32S3
    _spi = new SPIClass(FSPI);
#else
    _spi = new SPIClass(HSPI);
#endif
    _spi->begin(TOUCH_CLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN);

    DebugSerial.printf("[TOUCH] Pins: CS=%d CLK=%d MOSI=%d MISO=%d (hardware SPI, shared bus)\n",
                  TOUCH_CS_PIN, TOUCH_CLK_PIN, TOUCH_MOSI_PIN, TOUCH_MISO_PIN);

    // Self-test
    delay(50);
    _spi->beginTransaction(touchSPISettings);
    digitalWrite(TOUCH_CS_PIN, LOW);
    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    uint16_t tx = readChannel(CMD_X);
    uint16_t ty = readChannel(CMD_Y);
    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);
    _spi->endTransaction();

    DebugSerial.printf("[TOUCH] Self-test: Z1=%u Z2=%u X=%u Y=%u\n", z1, z2, tx, ty);
    DebugSerial.printf("[TOUCH] Z pressure = %d (threshold=%d)\n",
                  (int)z1 - (int)z2 + 4095, Z_THRESHOLD);
}

bool XPT2046_Soft::touched() {
    _spi->beginTransaction(touchSPISettings);
    digitalWrite(TOUCH_CS_PIN, LOW);
    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);
    _spi->endTransaction();

    int z = (int)z1 - (int)z2 + 4095;
    return z > Z_THRESHOLD;
}

bool XPT2046_Soft::readRaw(int16_t& x, int16_t& y) {
    _spi->beginTransaction(touchSPISettings);
    digitalWrite(TOUCH_CS_PIN, LOW);

    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    int z = (int)z1 - (int)z2 + 4095;

    if (z <= Z_THRESHOLD) {
        readChannel(CMD_PWD);
        digitalWrite(TOUCH_CS_PIN, HIGH);
        _spi->endTransaction();
        return false;
    }

    // Discard first read (noisy after Z)
    readChannel(CMD_X);
    readChannel(CMD_Y);

    // Average samples
    const int SAMPLES = 4;
    int32_t sumX = 0, sumY = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sumX += readChannel(CMD_X);
        sumY += readChannel(CMD_Y);
    }

    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);
    _spi->endTransaction();

    x = (int16_t)(sumX / SAMPLES);
    y = (int16_t)(sumY / SAMPLES);

    return (x > 100 && x < 4000 && y > 100 && y < 4000);
}

uint16_t XPT2046_Soft::readChannel(uint8_t cmd) {
    _spi->transfer(cmd);
    uint16_t hi = _spi->transfer(0x00);
    uint16_t lo = _spi->transfer(0x00);
    return ((hi << 8) | lo) >> 3;
}

#else
// =============================================================================
// Software SPI mode (dedicated pins, bit-bang)
// =============================================================================

void XPT2046_Soft::begin() {
    pinMode(TOUCH_CS_PIN, OUTPUT);
    pinMode(TOUCH_CLK_PIN, OUTPUT);
    pinMode(TOUCH_MOSI_PIN, OUTPUT);
    pinMode(TOUCH_MISO_PIN, INPUT);

    digitalWrite(TOUCH_CS_PIN, HIGH);
    digitalWrite(TOUCH_CLK_PIN, LOW);
    digitalWrite(TOUCH_MOSI_PIN, LOW);

    DebugSerial.printf("[TOUCH] Pins: CS=%d CLK=%d MOSI=%d MISO=%d (software SPI)\n",
                  TOUCH_CS_PIN, TOUCH_CLK_PIN, TOUCH_MOSI_PIN, TOUCH_MISO_PIN);

    // Self-test
    delay(50);
    digitalWrite(TOUCH_CS_PIN, LOW);
    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    uint16_t tx = readChannel(CMD_X);
    uint16_t ty = readChannel(CMD_Y);
    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);
    DebugSerial.printf("[TOUCH] Self-test: Z1=%u Z2=%u X=%u Y=%u\n", z1, z2, tx, ty);
    DebugSerial.printf("[TOUCH] Z pressure = %d (threshold=%d)\n",
                  (int)z1 - (int)z2 + 4095, Z_THRESHOLD);
}

bool XPT2046_Soft::touched() {
    digitalWrite(TOUCH_CS_PIN, LOW);
    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);

    int z = (int)z1 - (int)z2 + 4095;
    return z > Z_THRESHOLD;
}

bool XPT2046_Soft::readRaw(int16_t& x, int16_t& y) {
    digitalWrite(TOUCH_CS_PIN, LOW);

    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    int z = (int)z1 - (int)z2 + 4095;

    if (z <= Z_THRESHOLD) {
        readChannel(CMD_PWD);
        digitalWrite(TOUCH_CS_PIN, HIGH);
        return false;
    }

    // Discard first read (noisy after Z)
    readChannel(CMD_X);
    readChannel(CMD_Y);

    // Average samples
    const int SAMPLES = 4;
    int32_t sumX = 0, sumY = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sumX += readChannel(CMD_X);
        sumY += readChannel(CMD_Y);
    }

    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);

    x = (int16_t)(sumX / SAMPLES);
    y = (int16_t)(sumY / SAMPLES);

    return (x > 100 && x < 4000 && y > 100 && y < 4000);
}

uint16_t XPT2046_Soft::readChannel(uint8_t cmd) {
    uint16_t result = 0;

    // Phase 1: clock out 8-bit command on MOSI
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_MOSI_PIN, (cmd >> i) & 1);
        delayMicroseconds(2);
        digitalWrite(TOUCH_CLK_PIN, HIGH);
        delayMicroseconds(2);
        digitalWrite(TOUCH_CLK_PIN, LOW);
    }

    // Phase 2: clock in 16 bits on MISO
    digitalWrite(TOUCH_MOSI_PIN, LOW);
    for (int i = 15; i >= 0; i--) {
        delayMicroseconds(2);
        digitalWrite(TOUCH_CLK_PIN, HIGH);
        delayMicroseconds(2);
        if (digitalRead(TOUCH_MISO_PIN)) {
            result |= (1 << i);
        }
        digitalWrite(TOUCH_CLK_PIN, LOW);
    }

    return (result >> 3) & 0x0FFF;
}

#endif
