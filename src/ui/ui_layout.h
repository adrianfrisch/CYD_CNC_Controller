#pragma once
// =============================================================================
// UI Layout Constants — Resolution-independent layout derived from build flags
//
// All UI positioning, sizing, and spacing is computed from SCREEN_W and SCREEN_H
// (defined in config.h, derived from TFT_WIDTH/TFT_HEIGHT build flags swapped
// to landscape). To support a different display, only the build flags need to
// change — all layout adapts automatically.
// =============================================================================

#include "config.h"

// =============================================================================
// Base metrics — derived from screen resolution
// =============================================================================

// Header bar
static constexpr int UI_HEADER_H        = SCREEN_H / 10;          // 24 @ 240
static constexpr int UI_HEADER_TEXT_X    = SCREEN_W / 80;          // 4 @ 320
static constexpr int UI_HEADER_TEXT_Y    = UI_HEADER_H / 2;        // 12 @ 24

// Margins and padding
static constexpr int UI_MARGIN          = SCREEN_W / 80;           // 4 @ 320
static constexpr int UI_PADDING         = SCREEN_W / 40;           // 8 @ 320

// Font size (TFT_eSPI text size multiplier)
static constexpr int UI_FONT_SM         = 1;                       // Small text
static constexpr int UI_FONT_MD         = (SCREEN_W >= 480) ? 2 : 1;  // Medium (scales up for large displays)
static constexpr int UI_FONT_LG         = (SCREEN_W >= 480) ? 3 : 2;  // Large

// Text line height (pixels per text line at size 1)
static constexpr int UI_LINE_H          = SCREEN_H / 15;           // 16 @ 240
static constexpr int UI_LINE_H_SMALL    = SCREEN_H / 17;           // 14 @ 240

// =============================================================================
// Status bar (bottom of screen)
// =============================================================================
static constexpr int UI_STATUS_H        = SCREEN_H / 12;           // 20 @ 240
static constexpr int UI_STATUS_Y        = SCREEN_H - UI_STATUS_H;  // 220 @ 240
static constexpr int UI_STATUS_TEXT_Y   = UI_STATUS_Y + UI_STATUS_H / 2; // 230 @ 240

// =============================================================================
// Bottom button row (above status bar or at screen bottom)
// =============================================================================
static constexpr int UI_BTN_ROW_Y       = SCREEN_H - (SCREEN_H * 30 / 240); // 210 @ 240
static constexpr int UI_BTN_ROW_H       = SCREEN_H * 26 / 240;    // 26 @ 240

// =============================================================================
// File Browser Screen layout
// =============================================================================
static constexpr int FB_BTN_X           = SCREEN_W * 220 / 320;    // 220 @ 320
static constexpr int FB_BTN_W           = SCREEN_W - FB_BTN_X - UI_MARGIN; // 96 @ 320
static constexpr int FB_BTN_H           = SCREEN_H * 30 / 240;     // 30 @ 240
static constexpr int FB_LIST_W          = FB_BTN_X - UI_MARGIN;    // 216 @ 320
static constexpr int FB_LIST_TOP        = UI_HEADER_H + UI_MARGIN; // 28 @ 240
static constexpr int FB_ROW_H           = SCREEN_H * 31 / 240;     // 31 @ 240
static constexpr int FB_FILES_PER_PAGE  = (UI_STATUS_Y - FB_LIST_TOP) / FB_ROW_H; // 6 @ 240
static constexpr int FB_LIST_BOTTOM     = FB_LIST_TOP + FB_FILES_PER_PAGE * FB_ROW_H; // ~214

// File browser button Y positions (evenly spaced in the button column)
static constexpr int FB_BTN_GAP         = (UI_STATUS_Y - FB_LIST_TOP - 5 * FB_BTN_H) / 4;
static constexpr int FB_BTN_Y0          = FB_LIST_TOP;                              // JOG
static constexpr int FB_BTN_Y1          = FB_BTN_Y0 + FB_BTN_H + FB_BTN_GAP;       // UP
static constexpr int FB_BTN_Y2          = FB_BTN_Y1 + FB_BTN_H + FB_BTN_GAP;       // DOWN
static constexpr int FB_BTN_Y3          = FB_BTN_Y2 + FB_BTN_H + FB_BTN_GAP;       // OPEN
static constexpr int FB_BTN_Y4          = FB_BTN_Y3 + FB_BTN_H + FB_BTN_GAP;       // DEL

// Page indicator
static constexpr int FB_PAGE_IND_Y      = FB_LIST_BOTTOM + (UI_STATUS_Y - FB_LIST_BOTTOM) / 2;

// =============================================================================
// Job Screen layout
// =============================================================================
static constexpr int JOB_FILE_Y         = UI_HEADER_H + UI_LINE_H_SMALL / 2 + UI_MARGIN; // 34 @ 240
static constexpr int JOB_PROGRESS_Y     = SCREEN_H * 48 / 240;     // 48 @ 240
static constexpr int JOB_PROGRESS_W     = SCREEN_W - 2 * UI_MARGIN; // 312 @ 320
static constexpr int JOB_PROGRESS_H     = UI_LINE_H;               // 16 @ 240
static constexpr int JOB_PROGRESS_CX    = SCREEN_W / 2;            // 160 @ 320
static constexpr int JOB_PROGRESS_CY    = JOB_PROGRESS_Y + JOB_PROGRESS_H / 2; // 56

// Data rows
static constexpr int JOB_ROW_START      = JOB_PROGRESS_Y + JOB_PROGRESS_H + UI_PADDING + 4; // 76 @ 240
static constexpr int JOB_ROW_SPACING    = SCREEN_H * 20 / 240;     // 20 @ 240
static constexpr int JOB_ROW_POS        = JOB_ROW_START;                           // 76
static constexpr int JOB_ROW_STATE      = JOB_ROW_START + JOB_ROW_SPACING;        // 96
static constexpr int JOB_ROW_FEED       = JOB_ROW_START + 2 * JOB_ROW_SPACING;   // 116
static constexpr int JOB_ROW_LINE       = JOB_ROW_START + 3 * JOB_ROW_SPACING;   // 136
static constexpr int JOB_ROW_TIME       = JOB_ROW_START + 4 * JOB_ROW_SPACING;   // 156

// GCode display bar
static constexpr int JOB_GCODE_Y        = SCREEN_H * 176 / 240;    // 176 @ 240
static constexpr int JOB_GCODE_H        = SCREEN_H * 20 / 240;     // 20 @ 240
static constexpr int JOB_GCODE_TEXT_Y   = JOB_GCODE_Y + JOB_GCODE_H / 2; // 186

// Bottom control buttons
static constexpr int JOB_BTN_W          = SCREEN_W * 70 / 320;     // 70 @ 320
static constexpr int JOB_BTN_X0         = UI_MARGIN;               // 4  (PAUSE)
static constexpr int JOB_BTN_X1         = SCREEN_W * 80 / 320;     // 80  (RESUME)
static constexpr int JOB_BTN_X2         = SCREEN_W * 156 / 320;    // 156 (STOP)
static constexpr int JOB_BTN_X3         = SCREEN_W * 246 / 320;    // 246 (FILES)

// =============================================================================
// Jog Screen layout
// =============================================================================

// XY jog pad (starts at top — no header on jog screen)
static constexpr int JOG_PAD_X          = SCREEN_W * 20 / 320;     // 20 @ 320
static constexpr int JOG_PAD_Y          = SCREEN_H * 8 / 240;      // 8 @ 240 (was 40 — moved up, no header)
static constexpr int JOG_BTN_W          = SCREEN_W * 55 / 320;     // 55 @ 320
static constexpr int JOG_BTN_H          = SCREEN_H * 36 / 240;     // 36 @ 240
static constexpr int JOG_GAP            = SCREEN_W * 4 / 320;      // 4 @ 320

// Z buttons
static constexpr int JOG_Z_X            = SCREEN_W * 210 / 320;    // 210 @ 320
static constexpr int JOG_Z_W            = SCREEN_W * 50 / 320;     // 50 @ 320

// Z microstep buttons
static constexpr int JOG_ZM_X           = SCREEN_W * 266 / 320;    // 266 @ 320
static constexpr int JOG_ZM_W           = SCREEN_W * 50 / 320;     // 50 @ 320

// Step/Feed rows
static constexpr int JOG_STEP_Y         = SCREEN_H * 156 / 240;    // 156 @ 240
static constexpr int JOG_FEED_Y         = SCREEN_H * 180 / 240;    // 180 @ 240
static constexpr int JOG_CTRL_ROW_H     = UI_HEADER_H;             // 24 @ 240
static constexpr int JOG_CTRL_BTN_W     = SCREEN_W * 36 / 320;     // 36 @ 320
static constexpr int JOG_CTRL_PLUS_X    = SCREEN_W * 164 / 320;    // 164 @ 320
static constexpr int JOG_CTRL_CENTER_X  = SCREEN_W * 100 / 320;    // 100 @ 320
static constexpr int JOG_FEED_LABEL_X   = SCREEN_W * 204 / 320;    // 204 @ 320

// Position display
static constexpr int JOG_POS_Y          = SCREEN_H * 196 / 240;    // 196 @ 240
static constexpr int JOG_POS_H          = UI_LINE_H_SMALL;         // 14 @ 240

// Bottom buttons
static constexpr int JOG_BBTN_W         = SCREEN_W * 56 / 320;     // 56 @ 320
static constexpr int JOG_BBTN_X0        = UI_MARGIN;               // 4   (SET 0)
static constexpr int JOG_BBTN_X1        = SCREEN_W * 64 / 320;     // 64  (SET Z0)
static constexpr int JOG_BBTN_X2        = SCREEN_W * 124 / 320;    // 124 (GO 0)
static constexpr int JOG_BBTN_X3        = SCREEN_W * 184 / 320;    // 184 (HOME)
static constexpr int JOG_BBTN_X4        = SCREEN_W * 260 / 320;    // 260 (FILES)

// =============================================================================
// Preview Screen layout
// =============================================================================
static constexpr int PRV_INFO_Y         = UI_HEADER_H + UI_LINE_H_SMALL / 2 + UI_MARGIN; // 34 @ 240
static constexpr int PRV_LINES_Y        = SCREEN_H * 50 / 240;     // 50 @ 240
static constexpr int PRV_LINE_SPACING   = UI_LINE_H;               // 16 @ 240
static constexpr int PRV_BTN_BACK_X     = UI_MARGIN;               // 4 @ 320
static constexpr int PRV_BTN_BACK_W     = SCREEN_W * 70 / 320;     // 70 @ 320
static constexpr int PRV_BTN_START_X    = SCREEN_W * 240 / 320;    // 240 @ 320
static constexpr int PRV_BTN_START_W    = SCREEN_W * 76 / 320;     // 76 @ 320

// =============================================================================
// Calibration Screen layout
// =============================================================================
static constexpr int CAL_TGT_INSET      = SCREEN_W * 30 / 320;     // 30 @ 320
static constexpr int CAL_TGT1_X         = CAL_TGT_INSET;
static constexpr int CAL_TGT1_Y         = CAL_TGT_INSET * SCREEN_H / SCREEN_W; // proportional
static constexpr int CAL_TGT2_X         = SCREEN_W - CAL_TGT_INSET;
static constexpr int CAL_TGT2_Y         = SCREEN_H - (CAL_TGT_INSET * SCREEN_H / SCREEN_W);

// =============================================================================
// Touch hit area helpers
// =============================================================================

// Expand a button hit area by a padding amount (for fat-finger tolerance)
static constexpr int UI_TOUCH_SLOP      = UI_MARGIN;               // 4px extra on each side


