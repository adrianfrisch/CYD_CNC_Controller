#pragma once
// =============================================================================
// JOG Screen — manual CNC head movement, set zero, return to zero
// =============================================================================

#include "ui_manager.h"

class JogScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

private:
    void drawPosition();
    void drawStepAndFeed();

    float _jogStep = 1.0f;    // mm per jog press
    float _jogFeed = 1000.0f; // XY feed mm/min
    int   _stepIdx = 2;       // index into step sizes
    int   _feedIdx = JOG_FEED_DEFAULT_IDX; // index into feed rates
    unsigned long _lastUpdate = 0;

    // Cached values for dirty-checking
    float _prevX = -9999, _prevY = -9999, _prevZ = -9999;
    int   _prevState = -1;

    static constexpr float STEPS[] = {0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 50.0f};
    static constexpr int NUM_STEPS = 6;

    static constexpr float FEEDS[] = {50.0f, 100.0f, 500.0f, 1000.0f, 2000.0f, 3000.0f, 5000.0f};
    static constexpr int NUM_FEEDS = 7;

    static constexpr float Z_MICRO_STEP = 0.01f;   // mm — for Z touch-off probing
    static constexpr float Z_MICRO_FEED = 50.0f;    // mm/min — slow feed for micro jog

    float zFeed() const { return _jogFeed / JOG_Z_FEED_DIVISOR; }
};

