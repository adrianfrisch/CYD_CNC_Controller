#pragma once
// =============================================================================
// Job Execution Screen — progress, status, pause/resume/stop controls
// =============================================================================

#include "ui_manager.h"

class JobScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

private:
    void drawProgressBar();
    void drawPosition();
    void drawState();
    void drawFeedSpindle();
    void drawLine();
    void drawTime();
    void drawGCodeLine();

    unsigned long _lastUpdate = 0;

    // Cached values for dirty-checking (only redraw what changed)
    int    _prevPct = -1;
    float  _prevX = -9999, _prevY = -9999, _prevZ = -9999;
    int    _prevGrblState = -1;
    int    _prevJobState = -1;
    int    _prevFeed = -1;
    int    _prevSpindle = -1;
    int    _prevLine = -1;
    unsigned long _prevElapsed = 0;
    unsigned long _prevRemaining = 0;
    char   _prevGCode[54] = {};
};

