#pragma once
// =============================================================================
// UI Manager — touch handling, screen switching, common drawing primitives
// =============================================================================

#include <Arduino.h>
#include "display_driver.h"
#ifdef USE_GT911_TOUCH
  #include "gt911_touch.h"
#else
  #include "xpt2046_soft.h"
#endif
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
    bool loadCalibration();
    void checkCalibrationAfterSD();  // Call after SD is ready to trigger wizard if needed
    void saveCalibration(const TouchCalData& cal);
    void applyCalibration(const TouchCalData& cal);
    void runCalibration();

    // Raw touch access (for calibration screen)
    bool getRawTouch(int16_t& rawX, int16_t& rawY);
    // Mapped touch (applies calibration)
    bool getTouch(int16_t& screenX, int16_t& screenY);

    // Drawing helpers
    static void drawButton(DisplayDriver& tft, const Button& btn);
    static void drawHeader(DisplayDriver& tft, const char* title);
    static void drawProgressBar(DisplayDriver& tft, int x, int y, int w, int h, int percent, uint16_t color);
    static bool hitTest(int16_t tx, int16_t ty, const Button& btn);

    DisplayDriver& tft() { return _tft; }

private:
    DisplayDriver  _tft;
#ifdef USE_GT911_TOUCH
    GT911_Touch  _touch;
#else
    XPT2046_Soft _touch;
#endif

    static constexpr int NUM_SCREENS = (int)ScreenId::_COUNT;
    Screen*   _screens[NUM_SCREENS] = {};
    Screen*   _current = nullptr;
    ScreenId  _currentId = ScreenId::FileBrowser;

    TouchCalData _cal = {200, 3700, 200, 3700, 0};  // defaults
    bool _calibrated = false;

    unsigned long _lastTouch = 0;
    bool     _touched = false;
};

extern UIManager ui;



