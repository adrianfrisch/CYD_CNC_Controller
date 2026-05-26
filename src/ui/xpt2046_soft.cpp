// =============================================================================
// XPT2046 Software SPI Touch Driver — Implementation
//
// XPT2046 Control Byte: S A2 A1 A0 MODE SER/DFR PD1 PD0
//   Channels (differential, 12-bit, PD=01 keeps ADC on):
//     X position:  0xD1  (A=101)
//     Y position:  0x91  (A=001)
//     Z1 pressure: 0xB1  (A=011)
//     Z2 pressure: 0xC1  (A=100)
//     Power down:  0xD0  (PD=00, PENIRQ enabled)
// =============================================================================

#include "xpt2046_soft.h"

#define CMD_X   0xD1
#define CMD_Y   0x91
#define CMD_Z1  0xB1
#define CMD_Z2  0xC1
#define CMD_PWD 0xD0

void XPT2046_Soft::begin() {
    pinMode(TOUCH_CS_PIN, OUTPUT);
    pinMode(TOUCH_CLK_PIN, OUTPUT);
    pinMode(TOUCH_MOSI_PIN, OUTPUT);
    pinMode(TOUCH_MISO_PIN, INPUT);

    digitalWrite(TOUCH_CS_PIN, HIGH);
    digitalWrite(TOUCH_CLK_PIN, LOW);
    digitalWrite(TOUCH_MOSI_PIN, LOW);

    Serial.printf("[TOUCH] Pins: CS=%d CLK=%d MOSI=%d MISO=%d\n",
                  TOUCH_CS_PIN, TOUCH_CLK_PIN, TOUCH_MOSI_PIN, TOUCH_MISO_PIN);

    // Self-test: try reading Z, X, Y channels
    delay(50);
    digitalWrite(TOUCH_CS_PIN, LOW);
    uint16_t z1 = readChannel(CMD_Z1);
    uint16_t z2 = readChannel(CMD_Z2);
    uint16_t tx = readChannel(CMD_X);
    uint16_t ty = readChannel(CMD_Y);
    readChannel(CMD_PWD);
    digitalWrite(TOUCH_CS_PIN, HIGH);
    Serial.printf("[TOUCH] Self-test: Z1=%u Z2=%u X=%u Y=%u\n", z1, z2, tx, ty);
    Serial.printf("[TOUCH] Z pressure = %d (threshold=%d)\n",
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

    // Check pressure
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

// ---------------------------------------------------------------------------
// Read one 12-bit channel: send 8-bit command, then clock 16 more times
// reading MISO. Result = (16-bit read >> 3) & 0xFFF.
// This follows the standard 24-clock XPT2046 protocol.
// ---------------------------------------------------------------------------
uint16_t XPT2046_Soft::readChannel(uint8_t cmd) {
    uint16_t result = 0;

    // Phase 1: clock out 8-bit command on MOSI (ignore MISO)
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_MOSI_PIN, (cmd >> i) & 1);
        delayMicroseconds(2);
        digitalWrite(TOUCH_CLK_PIN, HIGH);
        delayMicroseconds(2);
        digitalWrite(TOUCH_CLK_PIN, LOW);
    }

    // Phase 2: clock in 16 bits on MISO (MOSI = 0)
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

    // Bits: [1 busy] [12 data MSB first] [3 trailing zeros]
    return (result >> 3) & 0x0FFF;
}
