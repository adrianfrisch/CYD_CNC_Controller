// =============================================================================
// GCode Preview Screen — Implementation
// =============================================================================

#include "screen_preview.h"
#include "../sd_manager.h"
#include "../job_streamer.h"

void PreviewScreen::enter() {
    loadPreview();
}

void PreviewScreen::loadPreview() {
    _lineCount = 0;
    _totalLines = 0;
    memset(_lines, 0, sizeof(_lines));

    File f = sdCard.openFile(g_selectedFile);
    if (!f) return;

    _fileSize = f.size();

    // Read first PREVIEW_LINES non-empty lines
    char buf[256];
    while (f.available() && _lineCount < PREVIEW_LINES) {
        int i = 0;
        while (f.available() && i < 255) {
            char c = f.read();
            if (c == '\n') break;
            if (c == '\r') continue;
            buf[i++] = c;
        }
        buf[i] = '\0';
        _totalLines++;

        if (i > 0) {
            strncpy(_lines[_lineCount], buf, 79);
            _lines[_lineCount][79] = '\0';
            _lineCount++;
        }
    }

    // Count remaining lines
    while (f.available()) {
        char c = f.read();
        if (c == '\n') _totalLines++;
    }
    f.close();
}

void PreviewScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "GCode Preview");

    tft.setTextSize(1);

    // File info
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    char info[64];
    snprintf(info, sizeof(info), "%s  (%d lines, %d bytes)", g_selectedFile, _totalLines,
             (int)_fileSize);
    tft.drawString(info, 4, 34);

    // Preview lines
    tft.setTextColor(CLR_TEXT, CLR_BG);
    for (int i = 0; i < _lineCount; i++) {
        char lineDisp[48];
        snprintf(lineDisp, sizeof(lineDisp), "%3d: %.42s", i + 1, _lines[i]);
        tft.drawString(lineDisp, 4, 50 + i * 16);
    }
    if (_totalLines > PREVIEW_LINES) {
        char more[32];
        snprintf(more, sizeof(more), "... +%d more lines", _totalLines - PREVIEW_LINES);
        tft.setTextColor(CLR_BORDER, CLR_BG);
        tft.drawString(more, 4, 50 + _lineCount * 16);
    }

    // Buttons
    Button btnBack  = {4, 210, 70, 26, CLR_BTN, "< BACK"};
    Button btnStart = {240, 210, 76, 26, CLR_BTN_ACTIVE, "START"};

    UIManager::drawButton(tft, btnBack);
    UIManager::drawButton(tft, btnStart);
}

void PreviewScreen::update() {
    // Nothing to update dynamically
}

void PreviewScreen::onTouch(int16_t x, int16_t y) {
    // BACK button
    if (x < 78 && y >= 210) {
        ui.switchScreen(ScreenId::FileBrowser);
        return;
    }
    // START button
    if (x >= 236 && y >= 210) {
        if (job.startJob(g_selectedFile)) {
            ui.switchScreen(ScreenId::Job);
        }
        return;
    }
}

