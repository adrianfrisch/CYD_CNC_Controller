#pragma once
// =============================================================================
// File Browser Screen — list GCode files, select, delete, navigate to Jog
// =============================================================================

#include "ui_manager.h"
#include "../sd_manager.h"

#define FILES_PER_PAGE 6

class FileBrowserScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

private:
    void drawFileList();
    void refreshFiles();
    bool checkForChanges();  // Returns true if SD contents changed

    FileInfo _files[MAX_FILES];
    int _fileCount = 0;
    int _page = 0;
    int _selectedIndex = -1;
    bool _needsRedraw = false;
    bool _confirmDelete = false;

    // Auto-refresh: check SD every 2 seconds
    unsigned long _lastCheck = 0;
    int _fingerprint = 0;  // simple hash of file count + sizes to detect changes
};

