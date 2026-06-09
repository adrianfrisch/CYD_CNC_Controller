// =============================================================================
// GT911 Capacitive Touch Driver — Implementation
// =============================================================================

#include "gt911_touch.h"

void GT911_Touch::begin() {
    // Reset sequence: INT LOW during reset selects address 0x5D
    pinMode(GT911_INT_PIN, OUTPUT);
    pinMode(GT911_RST_PIN, OUTPUT);

    digitalWrite(GT911_INT_PIN, LOW);
    digitalWrite(GT911_RST_PIN, LOW);
    delay(10);
    digitalWrite(GT911_RST_PIN, HIGH);
    delay(50);
    pinMode(GT911_INT_PIN, INPUT);

    Wire.begin(GT911_SDA_PIN, GT911_SCL_PIN);
    delay(100);

    // Detect which address the GT911 responds on
    Wire.beginTransmission(GT911_ADDR);
    if (Wire.endTransmission() == 0) {
        _addr = GT911_ADDR;
    } else {
        Wire.beginTransmission(GT911_ADDR_ALT);
        if (Wire.endTransmission() == 0) {
            _addr = GT911_ADDR_ALT;
        }
    }

    // Read GT911's configured output resolution from its config registers
    // (Do NOT write config — Sunton boards have GT911 config in firmware/OTP)
    uint8_t resBuf[4] = {0};
    readRegs(GT911_REG_X_RES, resBuf, 4);
    _nativeX = (uint16_t)resBuf[0] | ((uint16_t)resBuf[1] << 8);
    _nativeY = (uint16_t)resBuf[2] | ((uint16_t)resBuf[3] << 8);

    // Sanity check — if config reads as 0 or implausible, assume display size
    if (_nativeX == 0 || _nativeX > 4096) _nativeX = _maxX;
    if (_nativeY == 0 || _nativeY > 4096) _nativeY = _maxY;

    DebugSerial.printf("[TOUCH] GT911 at I2C 0x%02X, native res: %dx%d, display: %dx%d\n",
                       _addr, _nativeX, _nativeY, _maxX, _maxY);
    DebugSerial.printf("[TOUCH] Pins: SDA=%d SCL=%d INT=%d RST=%d\n",
                       GT911_SDA_PIN, GT911_SCL_PIN, GT911_INT_PIN, GT911_RST_PIN);
}

bool GT911_Touch::readRaw(int16_t& x, int16_t& y) {
    uint16_t tx, ty;
    if (!readTouch(tx, ty)) return false;

    // Scale from GT911 native resolution to display pixel coordinates
    x = (int16_t)((uint32_t)tx * _maxX / _nativeX);
    y = (int16_t)((uint32_t)ty * _maxY / _nativeY);

    // Clamp to valid range
    if (x >= (int16_t)_maxX) x = _maxX - 1;
    if (y >= (int16_t)_maxY) y = _maxY - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    return true;
}

bool GT911_Touch::readTouch(uint16_t& x, uint16_t& y) {
    uint8_t status = readReg(GT911_REG_STATUS);

    // Bit 7 = buffer ready, bits 3:0 = number of touch points
    uint8_t touches = status & 0x0F;
    bool bufReady = (status & 0x80) != 0;

    if (!bufReady || touches == 0) {
        // Clear status register
        if (bufReady) writeReg(GT911_REG_STATUS, 0);
        return false;
    }

    // Read first touch point (6 bytes: xL, xH, yL, yH, sizeL, sizeH)
    // Note: This GT911 firmware reports touch data WITHOUT a leading track ID byte
    uint8_t buf[8] = {0};
    uint8_t bytesRead = readRegs(GT911_REG_TOUCH1, buf, 6);

    // Clear status
    writeReg(GT911_REG_STATUS, 0);

    // Validate we actually got the data
    if (bytesRead < 4) return false;

    // X = buf[0] (low) | buf[1] (high), Y = buf[2] (low) | buf[3] (high)
    x = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    y = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);

    return true;
}

void GT911_Touch::reset() {
    pinMode(GT911_INT_PIN, OUTPUT);
    pinMode(GT911_RST_PIN, OUTPUT);
    digitalWrite(GT911_INT_PIN, LOW);
    digitalWrite(GT911_RST_PIN, LOW);
    delay(10);
    digitalWrite(GT911_RST_PIN, HIGH);
    delay(50);
    pinMode(GT911_INT_PIN, INPUT);
    delay(50);
}

void GT911_Touch::writeReg(uint16_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t GT911_Touch::readReg(uint16_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

uint8_t GT911_Touch::readRegs(uint16_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, len);
    uint8_t count = 0;
    for (uint8_t i = 0; i < len && Wire.available(); i++) {
        buf[i] = Wire.read();
        count++;
    }
    return count;
}

