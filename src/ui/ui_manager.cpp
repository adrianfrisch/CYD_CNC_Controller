// =============================================================================
// UI Manager — Implementation
// =============================================================================

#include "ui_manager.h"
#include "ui_layout.h"
#include "screen_filebrowser.h"
#include "screen_preview.h"
#include "screen_job.h"
#include "screen_jog.h"
#include "screen_calibration.h"
#include "../sd_manager.h"
#include <SD.h>

UIManager ui;

void UIManager::begin() {
    _tft.init();
#ifdef USE_LOVYANGFX
    _tft.setRotation(0); // RGB panel is already landscape (480x272)
#else
    _tft.setRotation(1); // SPI panels need rotation for landscape
#endif
    _tft.fillScreen(CLR_BG);

    // Backlight (LovyanGFX handles it via panel config; TFT_eSPI needs manual control)
#ifndef USE_LOVYANGFX
    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
#else
    _tft.setBrightness(255);
#endif

    // Initialize touch
    _touch.begin();
#ifdef USE_GT911_TOUCH
    DebugSerial.println("[UI] GT911 capacitive touch initialized (I2C)");
#else
    DebugSerial.println("[UI] XPT2046 touch initialized (software SPI)");
#endif

    // Create screens
    _screens[(int)ScreenId::FileBrowser] = new FileBrowserScreen();
    _screens[(int)ScreenId::Preview]     = new PreviewScreen();
    _screens[(int)ScreenId::Job]         = new JobScreen();
    _screens[(int)ScreenId::Jog]         = new JogScreen();
    _screens[(int)ScreenId::Calibration] = new CalibrationScreen();

    // Load touch calibration from SD, or run calibration wizard
#ifdef USE_GT911_TOUCH
    // GT911 capacitive touch is factory-calibrated — no wizard needed
    _calibrated = true;
    DebugSerial.println("[UI] GT911 capacitive touch — no calibration needed");
    switchScreen(ScreenId::FileBrowser);
#else
    if (!loadCalibration()) {
        DebugSerial.println("[UI] No calibration found — starting calibration wizard");
        switchScreen(ScreenId::Calibration);
    } else {
        DebugSerial.println("[UI] Touch calibration loaded from SD");
        switchScreen(ScreenId::FileBrowser);
    }
#endif
}

void UIManager::loop() {
    // Touch handling
    int16_t sx, sy;
    bool pressed = getTouch(sx, sy);

    if (pressed && !_touched && (millis() - _lastTouch > TOUCH_DEBOUNCE_MS)) {
        _touched = true;
        _lastTouch = millis();
        DBG("Touch at screen(%d, %d) on screen %d", sx, sy, (int)_currentId);
        if (_current) {
            _current->onTouch(sx, sy);
        }
    }
    if (!pressed) {
        _touched = false;
    }

    // Screen update
    if (_current) {
        _current->update();
    }
}

void UIManager::switchScreen(ScreenId id) {
    int idx = (int)id;
    if (idx < 0 || idx >= NUM_SCREENS || !_screens[idx]) return;
    _currentId = id;
    _current = _screens[idx];
    _tft.fillScreen(CLR_BG);
    _current->enter();
    _current->draw();
#ifdef USE_LOVYANGFX
    // Force PSRAM framebuffer cache flush so DMA displays the drawn content
    _tft.display();
#endif
    DebugSerial.printf("[UI] Switched to screen %d\n", idx);
}

// ---------------------------------------------------------------------------
// Touch — raw access (for calibration)
// ---------------------------------------------------------------------------

bool UIManager::getRawTouch(int16_t& rawX, int16_t& rawY) {
    return _touch.readRaw(rawX, rawY);
}

// ---------------------------------------------------------------------------
// Touch — calibrated screen coordinates
// ---------------------------------------------------------------------------

bool UIManager::getTouch(int16_t& screenX, int16_t& screenY) {
    int16_t rawX, rawY;
    if (!getRawTouch(rawX, rawY)) return false;

#ifdef USE_GT911_TOUCH
    // GT911 capacitive touch reports directly in screen pixel coordinates
    // No calibration mapping needed
    int32_t sx = rawX;
    int32_t sy = rawY;
#elif defined(USE_LOVYANGFX)
    // RGB panel (rotation=0, native landscape): raw X → screen X, raw Y → screen Y
    int32_t sx = (int32_t)(rawX - _cal.rawXMin) * SCREEN_W / (_cal.rawXMax - _cal.rawXMin);
    int32_t sy = (int32_t)(rawY - _cal.rawYMin) * SCREEN_H / (_cal.rawYMax - _cal.rawYMin);
#else
    // ILI9341 (rotation=1, portrait panel rotated to landscape):
    //   raw X → screen Y,  raw Y → screen X
    int32_t sx = (int32_t)(rawY - _cal.rawYMin) * SCREEN_W / (_cal.rawYMax - _cal.rawYMin);
    int32_t sy = (int32_t)(rawX - _cal.rawXMin) * SCREEN_H / (_cal.rawXMax - _cal.rawXMin);
#endif

    // Clamp to screen bounds
    if (sx < 0) sx = 0;
    if (sx >= SCREEN_W) sx = SCREEN_W - 1;
    if (sy < 0) sy = 0;
    if (sy >= SCREEN_H) sy = SCREEN_H - 1;

    screenX = (int16_t)sx;
    screenY = (int16_t)sy;
    return true;
}

// ---------------------------------------------------------------------------
// Drawing Helpers
// ---------------------------------------------------------------------------

void UIManager::drawButton(DisplayDriver& tft, const Button& btn) {
    uint16_t col = btn.enabled ? btn.color : CLR_BORDER;
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 4, col);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 4, CLR_TEXT);

    tft.setTextColor(CLR_TEXT, col);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(UI_FONT_SM);
    tft.drawString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
}

void UIManager::drawHeader(DisplayDriver& tft, const char* title) {
    tft.fillRect(0, 0, SCREEN_W, UI_HEADER_H, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(UI_FONT_SM);
    tft.drawString(title, UI_HEADER_TEXT_X, UI_HEADER_TEXT_Y);
}

void UIManager::drawProgressBar(DisplayDriver& tft, int x, int y, int w, int h, int percent, uint16_t color) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    tft.drawRect(x, y, w, h, CLR_BORDER);
    int filled = (w - 2) * percent / 100;
    tft.fillRect(x + 1, y + 1, filled, h - 2, color);
    tft.fillRect(x + 1 + filled, y + 1, w - 2 - filled, h - 2, CLR_BG);
}

bool UIManager::hitTest(int16_t tx, int16_t ty, const Button& btn) {
    return btn.enabled && tx >= btn.x && tx < btn.x + btn.w &&
           ty >= btn.y && ty < btn.y + btn.h;
}

// ---------------------------------------------------------------------------
// Touch Calibration — persistence on SD card
// ---------------------------------------------------------------------------

bool UIManager::loadCalibration() {
    if (!sdCard.isReady()) {
        DebugSerial.println("[UI] SD not ready — using default calibration");
        _calibrated = false;
        return true; // Use defaults temporarily, will re-check after SD init
    }

    if (!SD.exists(TOUCH_CAL_FILE)) {
        return false; // No calibration file — trigger wizard
    }

    File f = SD.open(TOUCH_CAL_FILE, FILE_READ);
    if (!f) return false;

    TouchCalData cal;
    size_t bytesRead = f.read((uint8_t*)&cal, sizeof(cal));
    f.close();

    if (bytesRead != sizeof(cal) || cal.magic != TOUCH_CAL_MAGIC) {
        DebugSerial.println("[UI] Calibration file corrupt — recalibrating");
        SD.remove(TOUCH_CAL_FILE);
        return false;
    }

    applyCalibration(cal);
    DebugSerial.printf("[UI] Calibration loaded: X[%d..%d] Y[%d..%d]\n",
                  _cal.rawXMin, _cal.rawXMax, _cal.rawYMin, _cal.rawYMax);
    return true;
}

void UIManager::checkCalibrationAfterSD() {
    if (!sdCard.isReady()) return;       // SD still not available
    if (_calibrated) return;             // Already calibrated from file


    // SD is now ready — try loading calibration
    if (!loadCalibration()) {
        DebugSerial.println("[UI] No calibration found — starting calibration wizard");
        switchScreen(ScreenId::Calibration);
    } else {
        DebugSerial.println("[UI] Touch calibration loaded from SD");
    }
}

void UIManager::saveCalibration(const TouchCalData& cal) {
    if (!sdCard.isReady()) {
        DebugSerial.println("[UI] SD not ready — calibration NOT saved");
        return;
    }

    File f = SD.open(TOUCH_CAL_FILE, FILE_WRITE);
    if (!f) {
        DebugSerial.println("[UI] Failed to create calibration file");
        return;
    }

    f.write((uint8_t*)&cal, sizeof(cal));
    f.close();
    DebugSerial.printf("[UI] Calibration saved: X[%d..%d] Y[%d..%d]\n",
                  cal.rawXMin, cal.rawXMax, cal.rawYMin, cal.rawYMax);
}

void UIManager::applyCalibration(const TouchCalData& cal) {
    _cal = cal;
    _calibrated = true;
}

void UIManager::runCalibration() {
    switchScreen(ScreenId::Calibration);
}
