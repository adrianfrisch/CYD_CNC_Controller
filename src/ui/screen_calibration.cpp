// =============================================================================
// Touch Calibration Screen — Implementation
// =============================================================================

#include "screen_calibration.h"

void CalibrationScreen::enter() {
    // Nothing — draw() handles the workflow
}

void CalibrationScreen::draw() {
    TFT_eSPI& tft = ui.tft();

    // --- Instruction screen ---
    tft.fillScreen(CLR_BG);
    UIManager::drawHeader(tft, "Touch Calibration");

    tft.setTextSize(1);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Touch the arrow in each corner", SCREEN_W / 2, 60);
    tft.drawString("as precisely as possible.", SCREEN_W / 2, 80);
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.drawString("Starting in 2 seconds...", SCREEN_W / 2, 120);

    delay(2000);

    // --- Run TFT_eSPI built-in calibration ---
    // calibrateTouch() draws crosshairs at 4 corners and waits for touches.
    // It fills calData with 5 uint16_t values.
    uint16_t calData[5];
    tft.fillScreen(CLR_BG);
    tft.calibrateTouch(calData, CLR_ACCENT, CLR_BG, 20);

    // --- Apply and save ---
    ui.applyCalibration(calData);
    ui.saveCalibration(calData);

    // --- Confirmation ---
    tft.fillScreen(CLR_BG);
    UIManager::drawHeader(tft, "Calibration Complete");

    tft.setTextSize(1);
    tft.setTextColor(CLR_BTN_ACTIVE, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Calibration saved to SD card.", SCREEN_W / 2, 80);

    // Show raw cal values for debug
    tft.setTextColor(CLR_BORDER, CLR_BG);
    char buf[64];
    snprintf(buf, sizeof(buf), "Cal: %u %u %u %u %u",
             calData[0], calData[1], calData[2], calData[3], calData[4]);
    tft.drawString(buf, SCREEN_W / 2, 110);

    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.drawString("Touch screen to continue...", SCREEN_W / 2, 160);

    // Wait for a touch to proceed
    uint16_t tx, ty;
    while (!tft.getTouch(&tx, &ty)) { delay(50); }
    delay(300); // debounce

    // Navigate to file browser
    ui.switchScreen(ScreenId::FileBrowser);
}

void CalibrationScreen::update() {
    // Nothing — calibration is a blocking flow in draw()
}

void CalibrationScreen::onTouch(int16_t x, int16_t y) {
    // Not used — calibration handles its own touch
}

