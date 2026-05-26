// =============================================================================
// File Browser Screen — Implementation
// =============================================================================

#include "screen_filebrowser.h"
#include "../grbl_comm.h"
#include "../web_server.h"

// Shared selected file path (used by Preview & Job screens)
char g_selectedFile[MAX_FILENAME] = {};

void FileBrowserScreen::enter() {
    _page = 0;
    _selectedIndex = -1;
    refreshFiles();
}

void FileBrowserScreen::refreshFiles() {
    _fileCount = sdCard.listGCodeFiles("/", _files, MAX_FILES);
    _needsRedraw = true;
}

void FileBrowserScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "CYD CNC - File Browser");

    // Status bar at bottom
    tft.fillRect(0, 220, SCREEN_W, 20, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);

    const char* stateStr = grbl.isConnected() ? "GRBL OK" : "NO GRBL";
    tft.drawString(stateStr, 4, 230);

    // Show WiFi IP or SD status
    if (webServer.isConnected()) {
        String ip = webServer.getIP();
        char buf[32];
        snprintf(buf, sizeof(buf), "http://%s", ip.c_str());
        tft.setTextColor(CLR_ACCENT, CLR_HEADER);
        tft.drawString(buf, 60, 230);
    } else if (sdCard.isReady()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "SD:%d files", _fileCount);
        tft.drawString(buf, 60, 230);
    } else {
        tft.drawString("NO SD CARD", 60, 230);
    }

    // Calibrate touch button in status bar
    tft.setTextColor(CLR_BORDER, CLR_HEADER);
    tft.setTextDatum(MR_DATUM);
    tft.drawString("[CAL]", 264, 230);

    // Navigation buttons on right side
    Button btnJog   = {270, 28, 46, 28, CLR_BTN, "JOG"};
    Button btnUp    = {270, 60, 46, 28, CLR_BTN, "UP"};
    Button btnDown  = {270, 92, 46, 28, CLR_BTN, "DOWN"};
    Button btnOpen  = {270, 130, 46, 28, CLR_BTN_ACTIVE, "OPEN", _selectedIndex >= 0};
    Button btnDel   = {270, 162, 46, 28, CLR_BTN_DANGER, "DEL", _selectedIndex >= 0};
    Button btnRefr  = {270, 194, 46, 28, CLR_BTN_WARN, "RFSH"};

    UIManager::drawButton(tft, btnJog);
    UIManager::drawButton(tft, btnUp);
    UIManager::drawButton(tft, btnDown);
    UIManager::drawButton(tft, btnOpen);
    UIManager::drawButton(tft, btnDel);
    UIManager::drawButton(tft, btnRefr);

    drawFileList();
}

void FileBrowserScreen::drawFileList() {
    TFT_eSPI& tft = ui.tft();

    int startIdx = _page * FILES_PER_PAGE;
    int y = 28;

    for (int i = 0; i < FILES_PER_PAGE; i++) {
        int idx = startIdx + i;
        tft.fillRect(0, y, 266, 30, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);

        if (idx < _fileCount) {
            tft.setTextColor(CLR_TEXT, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);
            tft.setTextDatum(ML_DATUM);
            tft.setTextSize(1);

            // Filename (truncated)
            char display[36];
            snprintf(display, sizeof(display), "%.28s", _files[idx].name);
            tft.drawString(display, 4, y + 15);

            // File size
            char sizeStr[16];
            if (_files[idx].size > 1024 * 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%.1fMB", _files[idx].size / 1048576.0f);
            } else if (_files[idx].size > 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%.1fKB", _files[idx].size / 1024.0f);
            } else {
                snprintf(sizeStr, sizeof(sizeStr), "%dB", (int)_files[idx].size);
            }
            tft.setTextDatum(MR_DATUM);
            tft.drawString(sizeStr, 262, y + 15);
        }

        tft.drawLine(0, y + 30, 266, y + 30, CLR_BORDER);
        y += 31;
    }

    // Page indicator
    int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    if (totalPages == 0) totalPages = 1;
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", _page + 1, totalPages);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(pageStr, 133, 216);
}

void FileBrowserScreen::update() {
    if (_needsRedraw) {
        _needsRedraw = false;
        draw();
    }
}

void FileBrowserScreen::onTouch(int16_t x, int16_t y) {
    // File list area (0–266, 28–214)
    if (x < 266 && y >= 28 && y < 214) {
        int row = (y - 28) / 31;
        int idx = _page * FILES_PER_PAGE + row;
        if (idx < _fileCount) {
            _selectedIndex = (_selectedIndex == idx) ? -1 : idx; // toggle
            _needsRedraw = true;
        }
        return;
    }

    // Right-side buttons (x >= 270)
    if (x >= 270) {
        if (y >= 28 && y < 56) {
            // JOG button
            ui.switchScreen(ScreenId::Jog);
        } else if (y >= 60 && y < 88) {
            // UP (previous page)
            if (_page > 0) { _page--; _selectedIndex = -1; _needsRedraw = true; }
        } else if (y >= 92 && y < 120) {
            // DOWN (next page)
            int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
            if (_page < totalPages - 1) { _page++; _selectedIndex = -1; _needsRedraw = true; }
        } else if (y >= 130 && y < 158 && _selectedIndex >= 0) {
            // OPEN — go to preview screen
            snprintf(g_selectedFile, MAX_FILENAME, "/%s", _files[_selectedIndex].name);
            ui.switchScreen(ScreenId::Preview);
        } else if (y >= 162 && y < 190 && _selectedIndex >= 0) {
            // DELETE
            char path[MAX_FILENAME + 2];
            snprintf(path, sizeof(path), "/%s", _files[_selectedIndex].name);
            sdCard.remove(path);
            _selectedIndex = -1;
            refreshFiles();
        } else if (y >= 194 && y < 222) {
            // REFRESH
            refreshFiles();
        }
    }

    // [CAL] button in status bar (x >= 230, y >= 220)
    if (x >= 230 && y >= 220) {
        ui.runCalibration();
    }
}

