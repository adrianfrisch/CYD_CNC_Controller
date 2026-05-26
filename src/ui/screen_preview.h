#pragma once
// =============================================================================
// GCode Preview Screen — shows file content before starting job
// =============================================================================

#include "ui_manager.h"

#define PREVIEW_LINES 10

class PreviewScreen : public Screen {
public:
    void enter() override;
    void draw() override;
    void update() override;
    void onTouch(int16_t x, int16_t y) override;

private:
    void loadPreview();

    char _lines[PREVIEW_LINES][80];
    int _lineCount = 0;
    bool _hasMore = false;
    size_t _fileSize = 0;
};

// Shared selected file
extern char g_selectedFile[64];

