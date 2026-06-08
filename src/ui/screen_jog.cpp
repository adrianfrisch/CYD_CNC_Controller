// =============================================================================
// JOG Screen — Implementation
// =============================================================================

#include "screen_jog.h"
#include "ui_layout.h"
#include "../grbl_comm.h"
#include "../machine_config.h"

constexpr float JogScreen::STEPS[];
constexpr float JogScreen::FEEDS[];

void JogScreen::enter() {
    _lastUpdate = 0;
    _prevX = _prevY = _prevZ = -9999;
    _prevState = -1;
}

void JogScreen::draw() {
    TFT_eSPI& tft = ui.tft();
    UIManager::drawHeader(tft, "JOG Control");

    tft.setTextSize(UI_FONT_SM);

    // --- XY Jog Pad (left side) ---
    int padX = JOG_PAD_X, padY = JOG_PAD_Y;
    int bw = JOG_BTN_W, bh = JOG_BTN_H, gap = JOG_GAP;

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
    int zx = JOG_Z_X;
    Button btnZP = {zx, padY, JOG_Z_W, bh, CLR_BTN, "Z+"};
    Button btnZM = {zx, padY + bh + gap + bh + gap, JOG_Z_W, bh, CLR_BTN, "Z-"};

    UIManager::drawButton(tft, btnZP);
    UIManager::drawButton(tft, btnZM);

    // Z label
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Z", zx + JOG_Z_W / 2, padY + bh + gap + bh / 2);

    // --- Z Microstep buttons (far right, for touch-off zeroing) ---
    int zmx = JOG_ZM_X;
    Button btnZuP = {zmx, padY,                       JOG_ZM_W, bh, CLR_BTN_ACTIVE, "Z+.05"};
    Button btnZuM = {zmx, padY + bh + gap + bh + gap, JOG_ZM_W, bh, CLR_BTN_ACTIVE, "Z-.05"};

    UIManager::drawButton(tft, btnZuP);
    UIManager::drawButton(tft, btnZuM);

    // Microstep label
    tft.setTextColor(CLR_BORDER, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("probe", zmx + JOG_ZM_W / 2, padY + bh + gap + bh / 2);

    // --- Step size & Feed rate ---
    drawStepAndFeed();

    // --- Bottom buttons ---
    Button btnSetZero  = {JOG_BBTN_X0, UI_BTN_ROW_Y, JOG_BBTN_W, UI_BTN_ROW_H, CLR_BTN_ACTIVE, "SET 0"};
    Button btnSetZ0    = {JOG_BBTN_X1, UI_BTN_ROW_Y, JOG_BBTN_W, UI_BTN_ROW_H, CLR_BTN_ACTIVE, "SET Z0"};
    Button btnGoZero   = {JOG_BBTN_X2, UI_BTN_ROW_Y, JOG_BBTN_W, UI_BTN_ROW_H, CLR_BTN,        "GO 0"};
    Button btnHome     = {JOG_BBTN_X3, UI_BTN_ROW_Y, JOG_BBTN_W, UI_BTN_ROW_H, CLR_BTN_WARN,   "HOME", machineConfig.homingEnabled()};
    Button btnFiles    = {JOG_BBTN_X4, UI_BTN_ROW_Y, JOG_BBTN_W, UI_BTN_ROW_H, CLR_BTN,         "FILES"};

    UIManager::drawButton(tft, btnSetZero);
    UIManager::drawButton(tft, btnSetZ0);
    UIManager::drawButton(tft, btnGoZero);
    UIManager::drawButton(tft, btnHome);
    UIManager::drawButton(tft, btnFiles);

    drawPosition();
}

void JogScreen::drawStepAndFeed() {
    TFT_eSPI& tft = ui.tft();
    tft.setTextSize(UI_FONT_SM);

    // --- Step size row ---
    int stepY = JOG_STEP_Y;
    tft.fillRect(0, stepY - JOG_CTRL_ROW_H / 2, SCREEN_W, JOG_CTRL_ROW_H, CLR_BG);

    char stepStr[20];
    snprintf(stepStr, sizeof(stepStr), "Step: %.1fmm", _jogStep);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(stepStr, JOG_CTRL_CENTER_X, stepY);

    Button btnStepDn = {UI_MARGIN,        stepY - JOG_CTRL_ROW_H / 2, JOG_CTRL_BTN_W, JOG_CTRL_ROW_H, CLR_BTN_WARN, "-"};
    Button btnStepUp = {JOG_CTRL_PLUS_X,  stepY - JOG_CTRL_ROW_H / 2, JOG_CTRL_BTN_W, JOG_CTRL_ROW_H, CLR_BTN_WARN, "+"};
    UIManager::drawButton(tft, btnStepDn);
    UIManager::drawButton(tft, btnStepUp);

    // --- Feed rate row ---
    int feedY = JOG_FEED_Y;
    tft.fillRect(0, feedY - JOG_CTRL_ROW_H / 2, SCREEN_W, JOG_CTRL_ROW_H, CLR_BG);

    char feedStr[28];
    snprintf(feedStr, sizeof(feedStr), "Feed:%5.0f Z:%5.0f", _jogFeed, zFeed());
    tft.setTextColor(CLR_ACCENT, CLR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(feedStr, JOG_CTRL_CENTER_X, feedY);

    Button btnFeedDn = {UI_MARGIN,        feedY - JOG_CTRL_ROW_H / 2, JOG_CTRL_BTN_W, JOG_CTRL_ROW_H, CLR_BTN_WARN, "-"};
    Button btnFeedUp = {JOG_CTRL_PLUS_X,  feedY - JOG_CTRL_ROW_H / 2, JOG_CTRL_BTN_W, JOG_CTRL_ROW_H, CLR_BTN_WARN, "+"};
    UIManager::drawButton(tft, btnFeedDn);
    UIManager::drawButton(tft, btnFeedUp);

    // Label
    tft.setTextColor(CLR_BORDER, CLR_BG);
    tft.setTextDatum(ML_DATUM);
    tft.drawString("mm/min", JOG_FEED_LABEL_X, feedY);
}

void JogScreen::drawPosition() {
    const GrblStatus& st = grbl.status();
    int si = (int)st.state;

    // Skip redraw if nothing changed
    if (fabsf(st.wposX - _prevX) < 0.005f &&
        fabsf(st.wposY - _prevY) < 0.005f &&
        fabsf(st.wposZ - _prevZ) < 0.005f &&
        si == _prevState) return;

    _prevX = st.wposX;
    _prevY = st.wposY;
    _prevZ = st.wposZ;
    _prevState = si;

    TFT_eSPI& tft = ui.tft();

    tft.setTextSize(UI_FONT_SM);
    tft.setTextColor(CLR_TEXT, CLR_BG);
    tft.setTextDatum(ML_DATUM);

    char buf[48];
    int posY = JOG_POS_Y;

    const char* stateNames[] = {"???", "Idle", "Run", "Hold", "Jog", "ALARM", "Door", "Check", "Home", "Sleep"};
    if (si < 0 || si > 9) si = 0;

    snprintf(buf, sizeof(buf), "X:%7.2f Y:%7.2f Z:%7.2f [%s]  ",
             st.wposX, st.wposY, st.wposZ, stateNames[si]);

    tft.fillRect(0, posY, SCREEN_W, JOG_POS_H, CLR_BG);
    tft.drawString(buf, UI_MARGIN, posY + JOG_POS_H / 2);
}

void JogScreen::update() {
    unsigned long now = millis();
    if (now - _lastUpdate < 300) return;
    _lastUpdate = now;
    drawPosition();
}

void JogScreen::onTouch(int16_t x, int16_t y) {
    int padX = JOG_PAD_X, padY = JOG_PAD_Y;
    int bw = JOG_BTN_W, bh = JOG_BTN_H, gap = JOG_GAP;

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
    int zx = JOG_Z_X;
    if (x >= zx && x < zx + JOG_Z_W) {
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
    int zmx = JOG_ZM_X;
    if (x >= zmx && x < zmx + JOG_ZM_W) {
        if (y >= padY && y < padY + bh) {
            grbl.jog(0, 0, Z_MICRO_STEP, Z_MICRO_FEED);
            return;
        }
        if (y >= padY + bh + gap + bh + gap && y < padY + 2 * bh + 2 * gap + bh) {
            grbl.jog(0, 0, -Z_MICRO_STEP, Z_MICRO_FEED);
            return;
        }
    }

    // --- Step size buttons ---
    int stepY = JOG_STEP_Y;
    if (y >= stepY - JOG_CTRL_ROW_H / 2 && y < stepY + JOG_CTRL_ROW_H / 2) {
        if (x < UI_MARGIN + JOG_CTRL_BTN_W) {
            if (_stepIdx > 0) { _stepIdx--; _jogStep = STEPS[_stepIdx]; drawStepAndFeed(); }
            return;
        }
        if (x >= JOG_CTRL_PLUS_X && x < JOG_CTRL_PLUS_X + JOG_CTRL_BTN_W) {
            if (_stepIdx < NUM_STEPS - 1) { _stepIdx++; _jogStep = STEPS[_stepIdx]; drawStepAndFeed(); }
            return;
        }
    }

    // --- Feed rate buttons ---
    int feedY = JOG_FEED_Y;
    if (y >= feedY - JOG_CTRL_ROW_H / 2 && y < feedY + JOG_CTRL_ROW_H / 2) {
        if (x < UI_MARGIN + JOG_CTRL_BTN_W) {
            if (_feedIdx > 0) { _feedIdx--; _jogFeed = FEEDS[_feedIdx]; drawStepAndFeed(); }
            return;
        }
        if (x >= JOG_CTRL_PLUS_X && x < JOG_CTRL_PLUS_X + JOG_CTRL_BTN_W) {
            if (_feedIdx < NUM_FEEDS - 1) { _feedIdx++; _jogFeed = FEEDS[_feedIdx]; drawStepAndFeed(); }
            return;
        }
    }

    // --- Bottom buttons ---
    if (y >= UI_BTN_ROW_Y) {
        if (x < JOG_BBTN_X1 - UI_TOUCH_SLOP) {
            grbl.jogCancel();
            grbl.setZero(); // SET 0 (all axes)
            grbl.requestStatus();
            _prevX = _prevY = _prevZ = -9999;
            _lastUpdate = 0;
        } else if (x < JOG_BBTN_X2 - UI_TOUCH_SLOP) {
            grbl.jogCancel();
            grbl.setZeroZ(); // SET Z0 (Z axis only)
            grbl.requestStatus();
            _prevZ = -9999;
            _lastUpdate = 0;
        } else if (x < JOG_BBTN_X3 - UI_TOUCH_SLOP) {
            grbl.jogCancel();
            grbl.goToZero(); // GO 0
        } else if (x < JOG_BBTN_X4 - UI_TOUCH_SLOP) {
            if (machineConfig.homingEnabled()) {
                grbl.jogCancel();
                grbl.homeMachine(); // HOME
            }
        } else {
            ui.switchScreen(ScreenId::FileBrowser); // FILES
        }
    }
}

