// =============================================================================
// Touch Calibration Screen — 2-point calibration using XPT2046_Touchscreen
//
// Strategy: Ask the user to touch two known screen positions (top-left area
// and bottom-right area).  From the raw XPT2046 values at those two points we
// derive a linear mapping: screenX = map(rawX, rawXMin, rawXMax, 0, 320) etc.
// =============================================================================

#include "screen_calibration.h"

// Calibration target positions (inset 30px from corners)
static constexpr int TGT1_X = 30;
static constexpr int TGT1_Y = 30;
static constexpr int TGT2_X = SCREEN_W - 30;  // 290
static constexpr int TGT2_Y = SCREEN_H - 30;  // 210

void CalibrationScreen::enter() {
    // Nothing — draw() handles the blocking workflow
}

void CalibrationScreen::draw() {
    TFT_eSPI& tft = ui.tft();

    // --- Instruction screen ---
    tft.fillScreen(CLR_BG);
    UIManager::drawHeader(tft, "Touch Calibration");

    tft.setTextSize(1);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Touch the crosshair as precisely", SCREEN_W / 2, 70);
    tft.drawString("as possible.  Two points needed.", SCREEN_W / 2, 90);
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.drawString("Touch screen to begin...", SCREEN_W / 2, 130);
    tft.setTextColor(CLR_BORDER, CLR_BG);
    tft.drawString("(Watching for touch input below)", SCREEN_W / 2, 160);

    // Live touch diagnostic — show raw values until user touches
    int16_t rx, ry;
    int attempts = 0;
    while (!ui.getRawTouch(rx, ry)) {
        if (++attempts % 50 == 0) { // Print every ~2.5 seconds
            Serial.printf("[CAL] Waiting for touch... (attempt %d)\n", attempts);
            // Show on screen that we're alive
            char msg[40];
            snprintf(msg, sizeof(msg), "Waiting... (%d)", attempts);
            tft.fillRect(0, 180, SCREEN_W, 20, CLR_BG);
            tft.drawString(msg, SCREEN_W / 2, 190);
        }
        delay(50);
    }
    Serial.printf("[CAL] Touch detected! Raw: X=%d Y=%d\n", rx, ry);

    // Wait for release
    while (ui.getRawTouch(rx, ry)) { delay(10); }
    delay(300);

    // --- Point 1: top-left area ---
    tft.fillScreen(CLR_BG);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Touch the crosshair (1/2)", SCREEN_W / 2, SCREEN_H / 2);

    int16_t raw1X, raw1Y, raw2X, raw2Y;
    collectPoint(TGT1_X, TGT1_Y, raw1X, raw1Y);

    delay(500); // small pause between points

    // --- Point 2: bottom-right area ---
    tft.fillScreen(CLR_BG);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Touch the crosshair (2/2)", SCREEN_W / 2, SCREEN_H / 2);

    collectPoint(TGT2_X, TGT2_Y, raw2X, raw2Y);

    // --- Compute calibration ---
    // Axes are swapped: rawX→screenY, rawY→screenX
    //   raw1 at screen (TGT1_X, TGT1_Y), raw2 at screen (TGT2_X, TGT2_Y)
    //   rawX drives screenY: scaleRawX based on TGT_Y spread
    //   rawY drives screenX: scaleRawY based on TGT_X spread

    float scaleRawX = (float)(raw2X - raw1X) / (float)(TGT2_Y - TGT1_Y); // rawX per screenY pixel
    float scaleRawY = (float)(raw2Y - raw1Y) / (float)(TGT2_X - TGT1_X); // rawY per screenX pixel

    int16_t rawXMin = (int16_t)(raw1X - scaleRawX * TGT1_Y);
    int16_t rawXMax = (int16_t)(raw1X + scaleRawX * (SCREEN_H - TGT1_Y));
    int16_t rawYMin = (int16_t)(raw1Y - scaleRawY * TGT1_X);
    int16_t rawYMax = (int16_t)(raw1Y + scaleRawY * (SCREEN_W - TGT1_X));

    TouchCalData cal;
    cal.rawXMin = rawXMin;
    cal.rawXMax = rawXMax;
    cal.rawYMin = rawYMin;
    cal.rawYMax = rawYMax;
    cal.magic   = TOUCH_CAL_MAGIC;

    // Apply and save
    ui.applyCalibration(cal);
    ui.saveCalibration(cal);

    // --- Confirmation ---
    tft.fillScreen(CLR_BG);
    UIManager::drawHeader(tft, "Calibration Complete");

    tft.setTextSize(1);
    tft.setTextColor(CLR_BTN_ACTIVE, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Calibration saved to SD card.", SCREEN_W / 2, 60);

    // Show debug info
    tft.setTextColor(CLR_BORDER, CLR_BG);
    char buf[80];
    snprintf(buf, sizeof(buf), "Pt1 raw: (%d, %d)  Pt2 raw: (%d, %d)", raw1X, raw1Y, raw2X, raw2Y);
    tft.drawString(buf, SCREEN_W / 2, 90);
    snprintf(buf, sizeof(buf), "X range: %d..%d  Y range: %d..%d", rawXMin, rawXMax, rawYMin, rawYMax);
    tft.drawString(buf, SCREEN_W / 2, 110);

    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.drawString("Touch screen to continue...", SCREEN_W / 2, 160);

    // Wait for a touch to proceed
    while (true) {
        int16_t rx, ry;
        if (ui.getRawTouch(rx, ry)) break;
        delay(50);
    }
    delay(300); // debounce

    // Navigate to file browser
    ui.switchScreen(ScreenId::FileBrowser);
}

void CalibrationScreen::update() {
    // Nothing — calibration is a blocking flow in draw()
}

void CalibrationScreen::onTouch(int16_t x, int16_t y) {
    // Not used — calibration handles its own touch via getRawTouch()
}

// ---------------------------------------------------------------------------
// Draw a crosshair at the given screen position
// ---------------------------------------------------------------------------
void CalibrationScreen::drawCrosshair(int x, int y, uint16_t color) {
    TFT_eSPI& tft = ui.tft();
    const int sz = 10;
    tft.drawLine(x - sz, y, x + sz, y, color);
    tft.drawLine(x, y - sz, x, y + sz, color);
    tft.drawCircle(x, y, sz - 2, color);
}

// ---------------------------------------------------------------------------
// Block until user touches the crosshair, collecting averaged raw values
// ---------------------------------------------------------------------------
bool CalibrationScreen::collectPoint(int screenX, int screenY, int16_t& rawX, int16_t& rawY) {
    drawCrosshair(screenX, screenY, CLR_ACCENT);

    // Wait for touch release first (in case still touching from previous)
    int16_t rx, ry;
    while (ui.getRawTouch(rx, ry)) { delay(10); }

    // Collect multiple samples for accuracy
    const int NUM_SAMPLES = 8;
    int32_t sumX = 0, sumY = 0;
    int collected = 0;

    while (collected < NUM_SAMPLES) {
        if (ui.getRawTouch(rx, ry)) {
            sumX += rx;
            sumY += ry;
            collected++;
            delay(20);
        } else {
            // If we already have some samples but touch was released, that's OK
            if (collected > 0) break;
            delay(10);
        }
    }

    if (collected == 0) return false;

    rawX = (int16_t)(sumX / collected);
    rawY = (int16_t)(sumY / collected);

    // Visual feedback — turn crosshair green
    drawCrosshair(screenX, screenY, CLR_BTN_ACTIVE);

    // Wait for release
    while (ui.getRawTouch(rx, ry)) { delay(10); }

    return true;
}
