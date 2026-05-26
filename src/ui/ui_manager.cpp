// =============================================================================
// UI Manager — Implementation
// =============================================================================

#include "ui_manager.h"
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
    _tft.setRotation(1); // Landscape, USB on left
    _tft.fillScreen(CLR_BG);

    // Backlight
    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

    // Initialize touch (software SPI — avoids HSPI conflict with SD card)
    _touch.begin();
    Serial.println("[UI] XPT2046 touch initialized (software SPI)");

    // Create screens
    _screens[(int)ScreenId::FileBrowser] = new FileBrowserScreen();
    _screens[(int)ScreenId::Preview]     = new PreviewScreen();
    _screens[(int)ScreenId::Job]         = new JobScreen();
    _screens[(int)ScreenId::Jog]         = new JogScreen();
    _screens[(int)ScreenId::Calibration] = new CalibrationScreen();

    // Load touch calibration from SD, or run calibration wizard
    if (!loadCalibration()) {
        Serial.println("[UI] No calibration found — starting calibration wizard");
        switchScreen(ScreenId::Calibration);
    } else {
        Serial.println("[UI] Touch calibration loaded from SD");
        switchScreen(ScreenId::FileBrowser);
    }
}

void UIManager::loop() {
    // Touch handling
    int16_t sx, sy;
    bool pressed = getTouch(sx, sy);

    if (pressed && !_touched && (millis() - _lastTouch > TOUCH_DEBOUNCE_MS)) {
        _touched = true;
        _lastTouch = millis();
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
    Serial.printf("[UI] Switched to screen %d\n", idx);
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

    // Map raw touch values to screen coordinates
    int32_t sx = (int32_t)(rawX - _cal.rawXMin) * SCREEN_W / (_cal.rawXMax - _cal.rawXMin);
    int32_t sy = (int32_t)(rawY - _cal.rawYMin) * SCREEN_H / (_cal.rawYMax - _cal.rawYMin);

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

void UIManager::drawButton(TFT_eSPI& tft, const Button& btn) {
    uint16_t col = btn.enabled ? btn.color : CLR_BORDER;
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 4, col);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 4, CLR_TEXT);

    tft.setTextColor(CLR_TEXT, col);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
}

void UIManager::drawHeader(TFT_eSPI& tft, const char* title) {
    tft.fillRect(0, 0, SCREEN_W, 24, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.drawString(title, 4, 12);
}

void UIManager::drawProgressBar(TFT_eSPI& tft, int x, int y, int w, int h, int percent, uint16_t color) {
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
        Serial.println("[UI] SD not ready — using default calibration");
        _calibrated = false;
        return true; // Use defaults, don't force calibration without SD
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
        Serial.println("[UI] Calibration file corrupt — recalibrating");
        SD.remove(TOUCH_CAL_FILE);
        return false;
    }

    applyCalibration(cal);
    Serial.printf("[UI] Calibration loaded: X[%d..%d] Y[%d..%d]\n",
                  _cal.rawXMin, _cal.rawXMax, _cal.rawYMin, _cal.rawYMax);
    return true;
}

void UIManager::saveCalibration(const TouchCalData& cal) {
    if (!sdCard.isReady()) {
        Serial.println("[UI] SD not ready — calibration NOT saved");
        return;
    }

    File f = SD.open(TOUCH_CAL_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[UI] Failed to create calibration file");
        return;
    }

    f.write((uint8_t*)&cal, sizeof(cal));
    f.close();
    Serial.printf("[UI] Calibration saved: X[%d..%d] Y[%d..%d]\n",
                  cal.rawXMin, cal.rawXMax, cal.rawYMin, cal.rawYMax);
}

void UIManager::applyCalibration(const TouchCalData& cal) {
    _cal = cal;
    _calibrated = true;
}

void UIManager::runCalibration() {
    switchScreen(ScreenId::Calibration);
}
