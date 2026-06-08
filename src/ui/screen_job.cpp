// =============================================================================
// Job Execution Screen — Implementation
// Only redraws UI elements whose underlying data actually changed.
// =============================================================================

#include "screen_job.h"
#include "ui_layout.h"
#include "../grbl_comm.h"
#include "../job_streamer.h"

extern char g_selectedFile[];

void JobScreen::enter() {
    _lastUpdate = 0;
    // Invalidate all cached values to force full draw
    _prevPct = -1;
    _prevX = _prevY = _prevZ = -9999;
    _prevGrblState = _prevJobState = -1;
    _prevFeed = _prevSpindle = -1;
    _prevLine = -1;
    _prevElapsed = _prevRemaining = 0;
    _prevGCode[0] = '\0';
}

void JobScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "Job Running");

    tft.setTextSize(UI_FONT_SM);

    // File name (static — only drawn once)
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(g_selectedFile, UI_MARGIN, JOB_FILE_Y);

    // Progress bar outline
    tft.drawRect(UI_MARGIN, JOB_PROGRESS_Y, JOB_PROGRESS_W, JOB_PROGRESS_H, CLR_BORDER);

    // Control buttons at bottom (static)
    Button btnPause  = {JOB_BTN_X0, UI_BTN_ROW_Y, JOB_BTN_W, UI_BTN_ROW_H, CLR_BTN_WARN,   "PAUSE"};
    Button btnResume = {JOB_BTN_X1, UI_BTN_ROW_Y, JOB_BTN_W, UI_BTN_ROW_H, CLR_BTN_ACTIVE,  "RESUME"};
    Button btnStop   = {JOB_BTN_X2, UI_BTN_ROW_Y, JOB_BTN_W, UI_BTN_ROW_H, CLR_BTN_DANGER,  "STOP"};
    Button btnBack   = {JOB_BTN_X3, UI_BTN_ROW_Y, JOB_BTN_W, UI_BTN_ROW_H, CLR_BTN,         "FILES"};

    UIManager::drawButton(tft, btnPause);
    UIManager::drawButton(tft, btnResume);
    UIManager::drawButton(tft, btnStop);
    UIManager::drawButton(tft, btnBack);

    // Force-draw all dynamic fields
    drawProgressBar();
    drawPosition();
    drawState();
    drawFeedSpindle();
    drawLine();
    drawTime();
    drawGCodeLine();
}

// --- Individual field draw methods (only redraw if value changed) ---

void JobScreen::drawProgressBar() {
    int pct = job.percentComplete();
    if (pct == _prevPct) return;
    _prevPct = pct;

    TFT_eSPI& tft = ui.tft();
    UIManager::drawProgressBar(tft, UI_MARGIN, JOB_PROGRESS_Y, JOB_PROGRESS_W, JOB_PROGRESS_H, pct, CLR_PROGRESS);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, pct > 50 ? CLR_PROGRESS : CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, JOB_PROGRESS_CX, JOB_PROGRESS_CY);
}

void JobScreen::drawPosition() {
    const GrblStatus& st = grbl.status();
    if (fabsf(st.wposX - _prevX) < 0.005f &&
        fabsf(st.wposY - _prevY) < 0.005f &&
        fabsf(st.wposZ - _prevZ) < 0.005f) return;

    _prevX = st.wposX;
    _prevY = st.wposY;
    _prevZ = st.wposZ;

    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[48];
    snprintf(buf, sizeof(buf), "X:%7.2f  Y:%7.2f  Z:%7.2f  ", st.wposX, st.wposY, st.wposZ);
    tft.drawString(buf, UI_MARGIN, JOB_ROW_POS);
}

void JobScreen::drawState() {
    int si = (int)grbl.status().state;
    int ji = (int)job.state();
    if (si == _prevGrblState && ji == _prevJobState) return;
    _prevGrblState = si;
    _prevJobState = ji;

    const char* stateNames[] = {"???", "Idle", "Run", "Hold", "Jog", "ALARM", "Door", "Check", "Home", "Sleep"};
    const char* jobStates[]  = {"Idle", "Running", "Paused", "Done", "Error"};
    if (si < 0 || si > 9) si = 0;
    if (ji < 0 || ji > 4) ji = 0;

    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[48];
    snprintf(buf, sizeof(buf), "GRBL: %-6s  Job: %-8s  ", stateNames[si], jobStates[ji]);
    tft.drawString(buf, UI_MARGIN, JOB_ROW_STATE);
}

void JobScreen::drawFeedSpindle() {
    const GrblStatus& st = grbl.status();
    if (st.feedRate == _prevFeed && st.spindleSpeed == _prevSpindle) return;
    _prevFeed = st.feedRate;
    _prevSpindle = st.spindleSpeed;

    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[48];
    snprintf(buf, sizeof(buf), "Feed: %d mm/min  Spindle: %d RPM   ", st.feedRate, st.spindleSpeed);
    tft.drawString(buf, UI_MARGIN, JOB_ROW_FEED);
}

void JobScreen::drawLine() {
    int line = job.currentLine();
    if (line == _prevLine) return;
    _prevLine = line;

    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[32];
    snprintf(buf, sizeof(buf), "Line: %d     ", line);
    tft.drawString(buf, UI_MARGIN, JOB_ROW_LINE);
}

void JobScreen::drawTime() {
    unsigned long elapsed = job.elapsedMs() / 1000;
    unsigned long remaining = job.estimatedRemainingMs() / 1000;
    if (elapsed == _prevElapsed && remaining == _prevRemaining) return;
    _prevElapsed = elapsed;
    _prevRemaining = remaining;

    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[48];
    snprintf(buf, sizeof(buf), "Elapsed: %02lu:%02lu  ETA: %02lu:%02lu   ",
             elapsed / 60, elapsed % 60, remaining / 60, remaining % 60);
    tft.drawString(buf, UI_MARGIN, JOB_ROW_TIME);
}

void JobScreen::drawGCodeLine() {
    char buf[54];
    snprintf(buf, sizeof(buf), "> %.50s", job.currentGCodeLine());
    if (strcmp(buf, _prevGCode) == 0) return;
    strncpy(_prevGCode, buf, sizeof(_prevGCode) - 1);

    TFT_eSPI& tft = ui.tft();
    tft.fillRect(0, JOB_GCODE_Y, SCREEN_W, JOB_GCODE_H, CLR_HEADER);
    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_ACCENT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(buf, UI_MARGIN, JOB_GCODE_TEXT_Y);
}

void JobScreen::update() {
    unsigned long now = millis();

    // GCode line — check every loop (changes per command)
    drawGCodeLine();

    // Other fields — check every 250ms
    if (now - _lastUpdate < 250) return;
    _lastUpdate = now;

    drawProgressBar();
    drawPosition();
    drawState();
    drawFeedSpindle();
    drawLine();
    drawTime();
}

void JobScreen::onTouch(int16_t x, int16_t y) {
    if (y < UI_BTN_ROW_Y) return; // Only buttons at bottom

    if (x < JOB_BTN_X1 - UI_TOUCH_SLOP) {
        job.pause();
    } else if (x < JOB_BTN_X2 - UI_TOUCH_SLOP) {
        job.resume();
    } else if (x < JOB_BTN_X3 - UI_TOUCH_SLOP) {
        job.stop();
    } else {
        if (job.state() != JobState::Running) {
            ui.switchScreen(ScreenId::FileBrowser);
        }
    }
}
