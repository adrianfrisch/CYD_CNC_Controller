#pragma once
// =============================================================================
// UI Manager — touch handling, screen switching, common drawing primitives
// =============================================================================

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// Forward declarations
class Screen;

// Button descriptor for touch
struct Button {
    int x, y, w, h;
    uint16_t color;
    const char* label;
    bool enabled;

    Button(int x_, int y_, int w_, int h_, uint16_t color_, const char* label_, bool enabled_ = true)
        : x(x_), y(y_), w(w_), h(h_), color(color_), label(label_), enabled(enabled_) {}
};

// Screen base class
class Screen {
public:
    virtual ~Screen() = default;
    virtual void enter() = 0;         // Called when screen becomes active
    virtual void draw() = 0;          // Full redraw
    virtual void update() = 0;        // Partial update (called in loop)
    virtual void onTouch(int16_t x, int16_t y) = 0;
};

// Screen IDs
enum class ScreenId : uint8_t {
    FileBrowser,
    Preview,
    Job,
    Jog,
    Calibration,
    _COUNT
};

class UIManager {
public:
    void begin();
    void loop();

    void switchScreen(ScreenId id);
    ScreenId currentScreenId() const { return _currentId; }

    // Touch calibration
    bool loadCalibration();                         // Load from SD card
    void saveCalibration(uint16_t calData[5]);      // Save to SD card
    void applyCalibration(uint16_t calData[5]);     // Apply to TFT_eSPI
    void runCalibration();                          // Switch to calibration screen

    // Drawing helpers
    static void drawButton(TFT_eSPI& tft, const Button& btn);
    static void drawHeader(TFT_eSPI& tft, const char* title);
    static void drawProgressBar(TFT_eSPI& tft, int x, int y, int w, int h, int percent, uint16_t color);
    static bool hitTest(int16_t tx, int16_t ty, const Button& btn);

    TFT_eSPI& tft() { return _tft; }

private:
    TFT_eSPI  _tft;
    static constexpr int NUM_SCREENS = (int)ScreenId::_COUNT;
    Screen*   _screens[NUM_SCREENS] = {};
    Screen*   _current = nullptr;
    ScreenId  _currentId = ScreenId::FileBrowser;

    unsigned long _lastTouch = 0;
    bool     _touched = false;
};

extern UIManager ui;



