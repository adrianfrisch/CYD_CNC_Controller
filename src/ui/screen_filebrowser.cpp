// =============================================================================
// File Browser Screen — Implementation
// =============================================================================

#include "screen_filebrowser.h"
#include "ui_layout.h"
#include "../grbl_comm.h"
#include "../web_server.h"

// Shared selected file path (used by Preview & Job screens)
char g_selectedFile[MAX_FILENAME] = {};

// Compute a simple fingerprint from file count and sizes
static int computeFingerprint(const FileInfo* files, int count) {
    int fp = count * 31;
    for (int i = 0; i < count; i++) {
        fp = fp * 37 + (int)(files[i].size & 0x7FFFFFFF);
        // Mix in first char of filename for extra uniqueness
        fp = fp * 17 + files[i].name[0];
    }
    return fp;
}

void FileBrowserScreen::enter() {
    _page = 0;
    _selectedIndex = -1;
    _confirmDelete = false;
    _lastCheck = 0;
    _fingerprint = 0;
    refreshFiles();
}

void FileBrowserScreen::refreshFiles() {
    _fileCount = sdCard.listGCodeFiles("/", _files, MAX_FILES);
    _fingerprint = computeFingerprint(_files, _fileCount);
    _needsRedraw = true;
}

bool FileBrowserScreen::checkForChanges() {
    FileInfo tempFiles[MAX_FILES];
    int count = sdCard.listGCodeFiles("/", tempFiles, MAX_FILES);
    int fp = computeFingerprint(tempFiles, count);
    if (fp != _fingerprint) {
        // Copy new data
        memcpy(_files, tempFiles, sizeof(tempFiles));
        _fileCount = count;
        _fingerprint = fp;
        return true;
    }
    return false;
}

// Button layout uses constants from ui_layout.h
static constexpr int BTN_X = FB_BTN_X;
static constexpr int BTN_W = FB_BTN_W;
static constexpr int BTN_H = FB_BTN_H;
static constexpr int LIST_W = FB_LIST_W;

void FileBrowserScreen::draw() {
    DisplayDriver& tft = ui.tft();
    UIManager::drawHeader(tft, "CYD CNC - File Browser");

    // Status bar at bottom
    tft.fillRect(0, UI_STATUS_Y, SCREEN_W, UI_STATUS_H, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(UI_FONT_SM);

    const char* stateStr = grbl.isConnected() ? "GRBL OK" : "NO GRBL";
    tft.drawString(stateStr, UI_MARGIN, UI_STATUS_TEXT_Y);

    // Show WiFi IP or SD status
    if (webServer.isConnected()) {
        String ip = webServer.getIP();
        char buf[32];
        snprintf(buf, sizeof(buf), "http://%s", ip.c_str());
        tft.setTextColor(CLR_ACCENT, CLR_HEADER);
        tft.drawString(buf, SCREEN_W * 60 / 320, UI_STATUS_TEXT_Y);
    } else if (sdCard.isReady()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "SD:%d files", _fileCount);
        tft.drawString(buf, SCREEN_W * 60 / 320, UI_STATUS_TEXT_Y);
    } else {
        tft.drawString("NO SD CARD", SCREEN_W * 60 / 320, UI_STATUS_TEXT_Y);
    }

    // Calibrate touch button in status bar
    tft.setTextColor(CLR_BORDER, CLR_HEADER);
    tft.setTextDatum(MR_DATUM);
    tft.drawString("[CAL]", SCREEN_W - UI_MARGIN, UI_STATUS_TEXT_Y);

    // Right-side buttons
    Button btnJog   = {BTN_X, FB_BTN_Y0, BTN_W, BTN_H, CLR_BTN,        "JOG"};
    Button btnUp    = {BTN_X, FB_BTN_Y1, BTN_W, BTN_H, CLR_BTN,        "UP"};
    Button btnDown  = {BTN_X, FB_BTN_Y2, BTN_W, BTN_H, CLR_BTN,        "DOWN"};
    Button btnOpen  = {BTN_X, FB_BTN_Y3, BTN_W, BTN_H, CLR_BTN_ACTIVE, "OPEN",  _selectedIndex >= 0};
    Button btnDel   = {BTN_X, FB_BTN_Y4, BTN_W, BTN_H, CLR_BTN_DANGER,
                       _confirmDelete ? "CONFIRM?" : "DEL",
                       _selectedIndex >= 0};

    UIManager::drawButton(tft, btnJog);
    UIManager::drawButton(tft, btnUp);
    UIManager::drawButton(tft, btnDown);
    UIManager::drawButton(tft, btnOpen);
    UIManager::drawButton(tft, btnDel);

    drawFileList();
}

void FileBrowserScreen::drawFileList() {
    DisplayDriver& tft = ui.tft();

    int startIdx = _page * FILES_PER_PAGE;
    int y = FB_LIST_TOP;

    for (int i = 0; i < FILES_PER_PAGE; i++) {
        int idx = startIdx + i;
        tft.fillRect(0, y, LIST_W, FB_ROW_H - 1, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);

        if (idx < _fileCount) {
            tft.setTextColor(CLR_TEXT, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);
            tft.setTextDatum(ML_DATUM);
            tft.setTextSize(UI_FONT_SM);

            // Filename (truncated)
            char display[30];
            snprintf(display, sizeof(display), "%.24s", _files[idx].name);
            tft.drawString(display, UI_MARGIN, y + FB_ROW_H / 2);

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
            tft.drawString(sizeStr, LIST_W - UI_MARGIN, y + FB_ROW_H / 2);
        }

        tft.drawLine(0, y + FB_ROW_H - 1, LIST_W, y + FB_ROW_H - 1, CLR_BORDER);
        y += FB_ROW_H;
    }

    // Page indicator
    int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    if (totalPages == 0) totalPages = 1;
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", _page + 1, totalPages);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(pageStr, LIST_W / 2, FB_PAGE_IND_Y);
}

void FileBrowserScreen::update() {
    if (_needsRedraw) {
        _needsRedraw = false;
        draw();
    }

    // Auto-refresh: check SD card every 2 seconds
    unsigned long now = millis();
    if (now - _lastCheck >= 2000) {
        _lastCheck = now;
        if (sdCard.isReady() && checkForChanges()) {
            DBG("File browser: SD contents changed, refreshing");
            _selectedIndex = -1;
            _confirmDelete = false;
            _needsRedraw = true;
        }
    }
}

void FileBrowserScreen::onTouch(int16_t x, int16_t y) {
    // File list area (left column)
    if (x < LIST_W && y >= FB_LIST_TOP && y < FB_LIST_BOTTOM) {
        int row = (y - FB_LIST_TOP) / FB_ROW_H;
        int idx = _page * FILES_PER_PAGE + row;
        if (idx < _fileCount) {
            _selectedIndex = (_selectedIndex == idx) ? -1 : idx; // toggle
            _confirmDelete = false;
            DebugSerial.printf("[FB] Selected file %d: %s\n", idx, _files[idx].name);
            _needsRedraw = true;
        }
        return;
    }

    // Right-side buttons (generous hit area: x >= BTN_X - UI_TOUCH_SLOP)
    if (x >= BTN_X - UI_TOUCH_SLOP) {
        if (y >= FB_BTN_Y0 && y < FB_BTN_Y0 + BTN_H) {
            // JOG button
            ui.switchScreen(ScreenId::Jog);
        } else if (y >= FB_BTN_Y1 && y < FB_BTN_Y1 + BTN_H) {
            // UP (previous page)
            if (_page > 0) { _page--; _selectedIndex = -1; _confirmDelete = false; _needsRedraw = true; }
        } else if (y >= FB_BTN_Y2 && y < FB_BTN_Y2 + BTN_H) {
            // DOWN (next page)
            int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
            if (_page < totalPages - 1) { _page++; _selectedIndex = -1; _confirmDelete = false; _needsRedraw = true; }
        } else if (y >= FB_BTN_Y3 && y < FB_BTN_Y3 + BTN_H && _selectedIndex >= 0) {
            // OPEN — go to preview screen
            DebugSerial.printf("[FB] Opening: %s\n", _files[_selectedIndex].name);
            snprintf(g_selectedFile, MAX_FILENAME, "/%s", _files[_selectedIndex].name);
            _confirmDelete = false;
            ui.switchScreen(ScreenId::Preview);
        } else if (y >= FB_BTN_Y4 && y < FB_BTN_Y4 + BTN_H && _selectedIndex >= 0) {
            // DELETE — requires confirmation
            if (_confirmDelete) {
                char path[MAX_FILENAME + 2];
                snprintf(path, sizeof(path), "/%s", _files[_selectedIndex].name);
                DebugSerial.printf("[FB] Deleting: %s\n", path);
                sdCard.remove(path);
                _selectedIndex = -1;
                _confirmDelete = false;
                refreshFiles();
            } else {
                DebugSerial.printf("[FB] Delete requested — tap again to confirm\n");
                _confirmDelete = true;
                _needsRedraw = true;
            }
        }
    }

    // [CAL] button in status bar
    if (x >= SCREEN_W - SCREEN_W * 60 / 320 && y >= UI_STATUS_Y) {
        ui.runCalibration();
    }
}
