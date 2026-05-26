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
    void drawStatus();
    void drawGCodeLine();

    unsigned long _lastUpdate = 0;
    int _lastPercent = -1;
};

