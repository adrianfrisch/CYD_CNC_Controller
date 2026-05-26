#pragma once
// =============================================================================
// Touch Calibration Screen — guided 4-corner calibration using TFT_eSPI
// =============================================================================

#include "ui_manager.h"

class CalibrationScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

    // The actual calibration runs inside enter() using TFT_eSPI's calibrateTouch()
    // which blocks until all 4 corners are touched. After that, the results are
    // saved to SD and the screen navigates to FileBrowser.
};

