#pragma once
// =============================================================================
// Touch Calibration Screen — 2-point calibration using XPT2046_Touchscreen
// =============================================================================

#include "ui_manager.h"

class CalibrationScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

private:
    // Collect a raw touch point, blocking until the user touches and releases.
    // Draws a crosshair at (screenX, screenY) and returns the raw XPT2046 values.
    bool collectPoint(int screenX, int screenY, int16_t& rawX, int16_t& rawY);
    void drawCrosshair(int x, int y, uint16_t color);
};
