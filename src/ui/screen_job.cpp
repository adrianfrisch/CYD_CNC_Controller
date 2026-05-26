// =============================================================================
// Job Execution Screen — Implementation
// =============================================================================

#include "screen_job.h"
#include "../grbl_comm.h"
#include "../job_streamer.h"

extern char g_selectedFile[];

void JobScreen::enter() {
    _lastPercent = -1;
    _lastUpdate = 0;
}

void JobScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "Job Running");

    tft.setTextSize(1);

    // File name
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(g_selectedFile, 4, 34);

    // Progress bar outline
    UIManager::drawProgressBar(tft, 4, 48, 312, 16, 0, CLR_PROGRESS);

    // Control buttons at bottom
    Button btnPause  = {4,   210, 70, 26, CLR_BTN_WARN,   "PAUSE"};
    Button btnResume = {80,  210, 70, 26, CLR_BTN_ACTIVE,  "RESUME"};
    Button btnStop   = {156, 210, 70, 26, CLR_BTN_DANGER,  "STOP"};
    Button btnBack   = {246, 210, 70, 26, CLR_BTN,         "FILES"};

    UIManager::drawButton(tft, btnPause);
    UIManager::drawButton(tft, btnResume);
    UIManager::drawButton(tft, btnStop);
    UIManager::drawButton(tft, btnBack);

    drawStatus();
}

void JobScreen::drawStatus() {
    TFT_eSPI& tft = ui.tft();
    const GrblStatus& st = grbl.status();

    tft.setTextSize(1);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    // Progress
    int pct = job.percentComplete();
    UIManager::drawProgressBar(tft, 4, 48, 312, 16, pct, CLR_PROGRESS);

    char buf[64];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    tft.setTextColor(CLR_TEXT, pct > 50 ? CLR_PROGRESS : CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, 160, 56);

    // Position
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    snprintf(buf, sizeof(buf), "X: %7.2f  Y: %7.2f  Z: %7.2f", st.wposX, st.wposY, st.wposZ);
    tft.drawString(buf, 4, 76);

    // Status line
    const char* stateNames[] = {"???", "Idle", "Run", "Hold", "Jog", "ALARM", "Door", "Check", "Home", "Sleep"};
    int si = (int)st.state;
    if (si < 0 || si > 9) si = 0;

    const char* jobStates[] = {"Idle", "Running", "Paused", "Done", "Error"};
    int ji = (int)job.state();
    if (ji < 0 || ji > 4) ji = 0;

    snprintf(buf, sizeof(buf), "GRBL: %-6s  Job: %-8s", stateNames[si], jobStates[ji]);
    tft.drawString(buf, 4, 96);

    // Feed & spindle
    snprintf(buf, sizeof(buf), "Feed: %d mm/min  Spindle: %d RPM", st.feedRate, st.spindleSpeed);
    tft.drawString(buf, 4, 116);

    // Line progress
    snprintf(buf, sizeof(buf), "Line: %d / %d", job.currentLine(), job.totalLines());
    tft.drawString(buf, 4, 136);

    // Time
    unsigned long elapsed = job.elapsedMs() / 1000;
    unsigned long remaining = job.estimatedRemainingMs() / 1000;
    snprintf(buf, sizeof(buf), "Elapsed: %02lu:%02lu  ETA: %02lu:%02lu",
             elapsed / 60, elapsed % 60, remaining / 60, remaining % 60);
    tft.drawString(buf, 4, 156);

    drawGCodeLine();
}

void JobScreen::drawGCodeLine() {
    TFT_eSPI& tft = ui.tft();
    // Show current GCode line being sent
    tft.fillRect(0, 176, SCREEN_W, 20, CLR_HEADER);
    tft.setTextColor(CLR_ACCENT, CLR_HEADER);
    tft.setTextDatum(ML_DATUM);

    char buf[54];
    snprintf(buf, sizeof(buf), "> %.50s", job.currentGCodeLine());
    tft.drawString(buf, 4, 186);
}

void JobScreen::update() {
    // Refresh display periodically
    unsigned long now = millis();
    if (now - _lastUpdate < 500) return;
    _lastUpdate = now;

    drawStatus();
}

void JobScreen::onTouch(int16_t x, int16_t y) {
    if (y < 210) return; // Only buttons at bottom

    if (x < 74) {
        // PAUSE
        job.pause();
    } else if (x < 150) {
        // RESUME
        job.resume();
    } else if (x < 226) {
        // STOP
        job.stop();
    } else {
        // FILES — back to file browser
        if (job.state() != JobState::Running) {
            ui.switchScreen(ScreenId::FileBrowser);
        }
    }
}

