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
    uint16_t tx, ty;
    bool pressed = _tft.getTouch(&tx, &ty);

    if (pressed && !_touched && (millis() - _lastTouch > TOUCH_DEBOUNCE_MS)) {
        _touched = true;
        _lastTouch = millis();
        if (_current) {
            _current->onTouch((int16_t)tx, (int16_t)ty);
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
        uint16_t calData[5] = {300, 3600, 300, 3600, 1};
        _tft.setTouch(calData);
        return true; // Use defaults, don't force calibration without SD
    }

    if (!SD.exists(TOUCH_CAL_FILE)) {
        return false; // No calibration file — trigger wizard
    }

    File f = SD.open(TOUCH_CAL_FILE, FILE_READ);
    if (!f) return false;

    uint16_t calData[5];
    size_t bytesRead = f.read((uint8_t*)calData, sizeof(calData));
    f.close();

    if (bytesRead != sizeof(calData)) {
        Serial.println("[UI] Calibration file corrupt — recalibrating");
        SD.remove(TOUCH_CAL_FILE);
        return false;
    }

    _tft.setTouch(calData);
    Serial.printf("[UI] Calibration loaded: %u %u %u %u %u\n",
                  calData[0], calData[1], calData[2], calData[3], calData[4]);
    return true;
}

void UIManager::saveCalibration(uint16_t calData[5]) {
    if (!sdCard.isReady()) {
        Serial.println("[UI] SD not ready — calibration NOT saved");
        return;
    }

    File f = SD.open(TOUCH_CAL_FILE, FILE_WRITE);
    if (!f) {
        Serial.println("[UI] Failed to create calibration file");
        return;
    }

    f.write((uint8_t*)calData, 5 * sizeof(uint16_t));
    f.close();
    Serial.printf("[UI] Calibration saved: %u %u %u %u %u\n",
                  calData[0], calData[1], calData[2], calData[3], calData[4]);
}

void UIManager::applyCalibration(uint16_t calData[5]) {
    _tft.setTouch(calData);
}

void UIManager::runCalibration() {
    switchScreen(ScreenId::Calibration);
}

