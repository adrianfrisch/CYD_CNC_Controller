// =============================================================================
// File Browser Screen — Implementation
// =============================================================================

#include "screen_filebrowser.h"
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

// Button column starts at x=220 (wider buttons, easier to hit)
static constexpr int BTN_X = 220;
static constexpr int BTN_W = 96;  // fills to 316
static constexpr int BTN_H = 30;
static constexpr int LIST_W = 216; // file list width (0..BTN_X-4)

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
    tft.drawString("[CAL]", SCREEN_W - 4, 230);

    // Right-side buttons — no more RFSH, DEL takes its spot
    Button btnJog   = {BTN_X, 28,  BTN_W, BTN_H, CLR_BTN,        "JOG"};
    Button btnUp    = {BTN_X, 62,  BTN_W, BTN_H, CLR_BTN,        "UP"};
    Button btnDown  = {BTN_X, 96,  BTN_W, BTN_H, CLR_BTN,        "DOWN"};
    Button btnOpen  = {BTN_X, 130, BTN_W, BTN_H, CLR_BTN_ACTIVE, "OPEN",  _selectedIndex >= 0};
    Button btnDel   = {BTN_X, 168, BTN_W, BTN_H, CLR_BTN_DANGER,
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
    TFT_eSPI& tft = ui.tft();

    int startIdx = _page * FILES_PER_PAGE;
    int y = 28;

    for (int i = 0; i < FILES_PER_PAGE; i++) {
        int idx = startIdx + i;
        tft.fillRect(0, y, LIST_W, 30, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);

        if (idx < _fileCount) {
            tft.setTextColor(CLR_TEXT, (idx == _selectedIndex) ? CLR_BTN : CLR_BG);
            tft.setTextDatum(ML_DATUM);
            tft.setTextSize(1);

            // Filename (truncated)
            char display[30];
            snprintf(display, sizeof(display), "%.24s", _files[idx].name);
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
            tft.drawString(sizeStr, LIST_W - 4, y + 15);
        }

        tft.drawLine(0, y + 30, LIST_W, y + 30, CLR_BORDER);
        y += 31;
    }

    // Page indicator
    int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    if (totalPages == 0) totalPages = 1;
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", _page + 1, totalPages);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(pageStr, LIST_W / 2, 216);
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
    if (x < LIST_W && y >= 28 && y < 214) {
        int row = (y - 28) / 31;
        int idx = _page * FILES_PER_PAGE + row;
        if (idx < _fileCount) {
            _selectedIndex = (_selectedIndex == idx) ? -1 : idx; // toggle
            _confirmDelete = false;
            Serial.printf("[FB] Selected file %d: %s\n", idx, _files[idx].name);
            _needsRedraw = true;
        }
        return;
    }

    // Right-side buttons (generous hit area: x >= BTN_X - 4)
    if (x >= BTN_X - 4) {
        if (y >= 28 && y < 58) {
            // JOG button
            ui.switchScreen(ScreenId::Jog);
        } else if (y >= 58 && y < 92) {
            // UP (previous page)
            if (_page > 0) { _page--; _selectedIndex = -1; _confirmDelete = false; _needsRedraw = true; }
        } else if (y >= 92 && y < 126) {
            // DOWN (next page)
            int totalPages = (_fileCount + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
            if (_page < totalPages - 1) { _page++; _selectedIndex = -1; _confirmDelete = false; _needsRedraw = true; }
        } else if (y >= 126 && y < 164 && _selectedIndex >= 0) {
            // OPEN — go to preview screen
            Serial.printf("[FB] Opening: %s\n", _files[_selectedIndex].name);
            snprintf(g_selectedFile, MAX_FILENAME, "/%s", _files[_selectedIndex].name);
            _confirmDelete = false;
            ui.switchScreen(ScreenId::Preview);
        } else if (y >= 164 && y < 200 && _selectedIndex >= 0) {
            // DELETE — requires confirmation
            if (_confirmDelete) {
                char path[MAX_FILENAME + 2];
                snprintf(path, sizeof(path), "/%s", _files[_selectedIndex].name);
                Serial.printf("[FB] Deleting: %s\n", path);
                sdCard.remove(path);
                _selectedIndex = -1;
                _confirmDelete = false;
                refreshFiles();
            } else {
                Serial.printf("[FB] Delete requested — tap again to confirm\n");
                _confirmDelete = true;
                _needsRedraw = true;
            }
        }
    }

    // [CAL] button in status bar
    if (x >= SCREEN_W - 60 && y >= 220) {
        ui.runCalibration();
    }
}
