// =============================================================================
// GCode Preview Screen — Implementation
// =============================================================================

#include "screen_preview.h"
#include "ui_layout.h"
#include "../sd_manager.h"
#include "../job_streamer.h"

void PreviewScreen::enter() {
    loadPreview();
}

void PreviewScreen::loadPreview() {
    _lineCount = 0;
    memset(_lines, 0, sizeof(_lines));

    File f = sdCard.openFile(g_selectedFile);
    if (!f) return;

    _fileSize = f.size();

    // Read first PREVIEW_LINES non-empty lines only — do NOT scan entire file
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

        if (i > 0) {
            strncpy(_lines[_lineCount], buf, 79);
            _lines[_lineCount][79] = '\0';
            _lineCount++;
        }
    }

    _hasMore = f.available();
    f.close();
}

void PreviewScreen::draw() {
    DisplayDriver& tft = ui.tft();
    UIManager::drawHeader(tft, "GCode Preview");

    tft.setTextSize(UI_FONT_SM);

    // File info — size only (no slow full-file scan)
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    char info[64];
    if (_fileSize > 1024 * 1024) {
        snprintf(info, sizeof(info), "%s  (%.1f MB)", g_selectedFile, _fileSize / 1048576.0f);
    } else if (_fileSize > 1024) {
        snprintf(info, sizeof(info), "%s  (%.1f KB)", g_selectedFile, _fileSize / 1024.0f);
    } else {
        snprintf(info, sizeof(info), "%s  (%d B)", g_selectedFile, (int)_fileSize);
    }
    tft.drawString(info, UI_MARGIN, PRV_INFO_Y);

    // Preview lines
    tft.setTextColor(CLR_TEXT, CLR_BG);
    for (int i = 0; i < _lineCount; i++) {
        char lineDisp[48];
        snprintf(lineDisp, sizeof(lineDisp), "%3d: %.42s", i + 1, _lines[i]);
        tft.drawString(lineDisp, UI_MARGIN, PRV_LINES_Y + i * PRV_LINE_SPACING);
    }
    if (_hasMore) {
        tft.setTextColor(CLR_BORDER, CLR_BG);
        tft.drawString("... more lines in file", UI_MARGIN, PRV_LINES_Y + _lineCount * PRV_LINE_SPACING);
    }

    // Buttons
    Button btnBack  = {PRV_BTN_BACK_X, UI_BTN_ROW_Y, PRV_BTN_BACK_W, UI_BTN_ROW_H, CLR_BTN, "< BACK"};
    Button btnStart = {PRV_BTN_START_X, UI_BTN_ROW_Y, PRV_BTN_START_W, UI_BTN_ROW_H, CLR_BTN_ACTIVE, "START"};

    UIManager::drawButton(tft, btnBack);
    UIManager::drawButton(tft, btnStart);
}

void PreviewScreen::update() {
    // Nothing to update dynamically
}

void PreviewScreen::onTouch(int16_t x, int16_t y) {
    // BACK button
    if (x < PRV_BTN_BACK_X + PRV_BTN_BACK_W + UI_TOUCH_SLOP && y >= UI_BTN_ROW_Y) {
        ui.switchScreen(ScreenId::FileBrowser);
        return;
    }
    // START button
    if (x >= PRV_BTN_START_X - UI_TOUCH_SLOP && y >= UI_BTN_ROW_Y) {
        if (job.startJob(g_selectedFile)) {
            ui.switchScreen(ScreenId::Job);
        }
        return;
    }
}

