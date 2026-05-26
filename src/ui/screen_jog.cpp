// =============================================================================
// JOG Screen — Implementation
// =============================================================================

#include "screen_jog.h"
#include "../grbl_comm.h"

constexpr float JogScreen::STEPS[];
constexpr float JogScreen::FEEDS[];

void JogScreen::enter() {
    _lastUpdate = 0;
}

void JogScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "JOG Control");

    tft.setTextSize(1);

    // --- XY Jog Pad (left side) ---
    // Layout:     [  Y+  ]
    //     [ X- ] [      ] [ X+ ]
    //             [  Y-  ]

    int padX = 20, padY = 40;
    int bw = 55, bh = 36, gap = 4;

    // Y+
    Button btnYP = {padX + bw + gap, padY, bw, bh, CLR_BTN, "Y+"};
    // X-
    Button btnXM = {padX, padY + bh + gap, bw, bh, CLR_BTN, "X-"};
    // X+
    Button btnXP = {padX + 2 * (bw + gap), padY + bh + gap, bw, bh, CLR_BTN, "X+"};
    // Y-
    Button btnYM = {padX + bw + gap, padY + 2 * (bh + gap), bw, bh, CLR_BTN, "Y-"};

    UIManager::drawButton(tft, btnYP);
    UIManager::drawButton(tft, btnXM);
    UIManager::drawButton(tft, btnXP);
    UIManager::drawButton(tft, btnYM);

    // --- Z Jog (right side) ---
    int zx = 210;
    Button btnZP = {zx, padY, 50, bh, CLR_BTN, "Z+"};
    Button btnZM = {zx, padY + bh + gap + bh + gap, 50, bh, CLR_BTN, "Z-"};

    UIManager::drawButton(tft, btnZP);
    UIManager::drawButton(tft, btnZM);

    // Z label
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Z", zx + 25, padY + bh + gap + bh / 2);

    // --- Z Microstep buttons (far right, for touch-off zeroing) ---
    int zmx = 266;
    Button btnZuP = {zmx, padY,                       50, bh, CLR_BTN_ACTIVE, "Z+.01"};
    Button btnZuM = {zmx, padY + bh + gap + bh + gap, 50, bh, CLR_BTN_ACTIVE, "Z-.01"};

    UIManager::drawButton(tft, btnZuP);
    UIManager::drawButton(tft, btnZuM);

    // Microstep label
    tft.setTextColor(CLR_BORDER, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("probe", zmx + 25, padY + bh + gap + bh / 2);

    // --- Step size & Feed rate ---
    drawStepAndFeed();

    // --- Bottom buttons ---
    Button btnSetZero  = {4,   210, 56, 26, CLR_BTN_ACTIVE, "SET 0"};
    Button btnSetZ0    = {64,  210, 56, 26, CLR_BTN_ACTIVE, "SET Z0"};
    Button btnGoZero   = {124, 210, 56, 26, CLR_BTN,        "GO 0"};
    Button btnHome     = {184, 210, 56, 26, CLR_BTN_WARN,   "HOME"};
    Button btnFiles    = {260, 210, 56, 26, CLR_BTN,         "FILES"};

    UIManager::drawButton(tft, btnSetZero);
    UIManager::drawButton(tft, btnSetZ0);
    UIManager::drawButton(tft, btnGoZero);
    UIManager::drawButton(tft, btnHome);
    UIManager::drawButton(tft, btnFiles);

    drawPosition();
}

void JogScreen::drawStepAndFeed() {
    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(1);

    // --- Step size row (y=156) ---
    int stepY = 156;
    tft.fillRect(0, stepY - 12, SCREEN_W, 24, CLR_BG);

    char stepStr[20];
    snprintf(stepStr, sizeof(stepStr), "Step: %.1fmm", _jogStep);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(stepStr, 100, stepY);

    Button btnStepDn = {4,   stepY - 12, 36, 24, CLR_BTN_WARN, "-"};
    Button btnStepUp = {164, stepY - 12, 36, 24, CLR_BTN_WARN, "+"};
    UIManager::drawButton(tft, btnStepDn);
    UIManager::drawButton(tft, btnStepUp);

    // --- Feed rate row (y=180) ---
    int feedY = 180;
    tft.fillRect(0, feedY - 12, SCREEN_W, 24, CLR_BG);

    char feedStr[28];
    snprintf(feedStr, sizeof(feedStr), "Feed:%5.0f Z:%5.0f", _jogFeed, zFeed());
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(feedStr, 100, feedY);

    Button btnFeedDn = {4,   feedY - 12, 36, 24, CLR_BTN_WARN, "-"};
    Button btnFeedUp = {164, feedY - 12, 36, 24, CLR_BTN_WARN, "+"};
    UIManager::drawButton(tft, btnFeedDn);
    UIManager::drawButton(tft, btnFeedUp);

    // Label
    tft.setTextColor(CLR_BORDER, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("mm/min", 204, feedY);
}

void JogScreen::drawPosition() {
    TFT_eSPI& tft = ui.tft();
    const GrblStatus& st = grbl.status();

    tft.setTextSize(1);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[40];
    int posY = 196;

    // GRBL state
    const char* stateNames[] = {"???", "Idle", "Run", "Hold", "Jog", "ALARM", "Door", "Check", "Home", "Sleep"};
    int si = (int)st.state;
    if (si < 0 || si > 9) si = 0;

    snprintf(buf, sizeof(buf), "X:%7.2f Y:%7.2f Z:%7.2f [%s]",
             st.wposX, st.wposY, st.wposZ, stateNames[si]);

    // Clear and redraw position line
    tft.fillRect(0, posY, SCREEN_W, 14, CLR_BG);
    tft.drawString(buf, 4, posY + 7);
}

void JogScreen::update() {
    unsigned long now = millis();
    if (now - _lastUpdate < 300) return;
    _lastUpdate = now;
    drawPosition();
}

void JogScreen::onTouch(int16_t x, int16_t y) {
    int padX = 20, padY = 40;
    int bw = 55, bh = 36, gap = 4;

    // --- XY pad ---
    // Y+
    if (x >= padX + bw + gap && x < padX + 2 * bw + gap && y >= padY && y < padY + bh) {
        grbl.jog(0, _jogStep, 0, _jogFeed);
        return;
    }
    // X-
    if (x >= padX && x < padX + bw && y >= padY + bh + gap && y < padY + 2 * bh + gap) {
        grbl.jog(-_jogStep, 0, 0, _jogFeed);
        return;
    }
    // X+
    if (x >= padX + 2 * (bw + gap) && x < padX + 3 * bw + 2 * gap && y >= padY + bh + gap && y < padY + 2 * bh + gap) {
        grbl.jog(_jogStep, 0, 0, _jogFeed);
        return;
    }
    // Y-
    if (x >= padX + bw + gap && x < padX + 2 * bw + gap && y >= padY + 2 * (bh + gap) && y < padY + 3 * bh + 2 * gap) {
        grbl.jog(0, -_jogStep, 0, _jogFeed);
        return;
    }

    // --- Z buttons ---
    int zx = 210;
    if (x >= zx && x < zx + 50) {
        if (y >= padY && y < padY + bh) {
            grbl.jog(0, 0, _jogStep, zFeed());
            return;
        }
        if (y >= padY + bh + gap + bh + gap && y < padY + 2 * bh + 2 * gap + bh) {
            grbl.jog(0, 0, -_jogStep, zFeed());
            return;
        }
    }

    // --- Z Microstep buttons (0.01mm at slow feed for touch-off) ---
    int zmx = 266;
    if (x >= zmx && x < zmx + 50) {
        if (y >= padY && y < padY + bh) {
            grbl.jog(0, 0, Z_MICRO_STEP, Z_MICRO_FEED);
            return;
        }
        if (y >= padY + bh + gap + bh + gap && y < padY + 2 * bh + 2 * gap + bh) {
            grbl.jog(0, 0, -Z_MICRO_STEP, Z_MICRO_FEED);
            return;
        }
    }

    // --- Step size buttons (y=156 row) ---
    int stepY = 156;
    if (y >= stepY - 12 && y < stepY + 12) {
        if (x < 40) {
            if (_stepIdx > 0) { _stepIdx--; _jogStep = STEPS[_stepIdx]; drawStepAndFeed(); }
            return;
        }
        if (x >= 164 && x < 200) {
            if (_stepIdx < NUM_STEPS - 1) { _stepIdx++; _jogStep = STEPS[_stepIdx]; drawStepAndFeed(); }
            return;
        }
    }

    // --- Feed rate buttons (y=180 row) ---
    int feedY = 180;
    if (y >= feedY - 12 && y < feedY + 12) {
        if (x < 40) {
            if (_feedIdx > 0) { _feedIdx--; _jogFeed = FEEDS[_feedIdx]; drawStepAndFeed(); }
            return;
        }
        if (x >= 164 && x < 200) {
            if (_feedIdx < NUM_FEEDS - 1) { _feedIdx++; _jogFeed = FEEDS[_feedIdx]; drawStepAndFeed(); }
            return;
        }
    }

    // --- Bottom buttons ---
    if (y >= 210) {
        if (x < 60) {
            grbl.setZero(); // SET 0 (all axes)
        } else if (x < 120) {
            grbl.setZeroZ(); // SET Z0 (Z axis only)
        } else if (x < 180) {
            grbl.goToZero(); // GO 0
        } else if (x < 240) {
            grbl.homeMachine(); // HOME
        } else {
            ui.switchScreen(ScreenId::FileBrowser); // FILES
        }
    }
}

