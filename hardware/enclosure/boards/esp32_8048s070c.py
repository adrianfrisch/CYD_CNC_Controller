"""
Sunton / Jingcai ESP32-8048S070C -- 7" 800x480 RGB TFT, GT911 capacitive
touch, ESP32-S3-WROOM-1.  Board data for ../enclosure.py.

Frame: seen from the FRONT (screen towards you, landscape, touch-FPC notch at
the bottom edge, USB-C on the left edge).  X 0..181 left->right, Y 0..108
bottom->top, Z = 0 PCB front face.  The vendor's "back" pictures are mirrored
left/right with respect to this frame (x_front = 181 - x_back).

SOURCES (download into ../reference/ -- not committed, see ../README.md)
  [DIM]  esp32-8048S070-back.png (identical to the vendor package file
         3-Structure_Diagram/Dimensions.png)
         Vendor dimensioned PCB drawing (front side, LCD outline drawn).
         Printed dims: 181.0, 108.0, 165.0, 100.0, 3.5, 3.5, R1.6.
         Scale derived from the printed 181.0/108.0: 6.80 px/mm (both axes
         agree to 0.2 %).  Values marked "meas. [DIM]" are measured on that
         drawing at that scale (about +/-0.3 mm).
  [SPEC] vendor-ESP32-8048S070-Specifications-EN.pdf (Jingcai), p.4 table
         (module 181.0 x 108.0, active area 153.84 x 85.63) and p.5
         "Interface Description" back-side render
         (= vendor-spec-p5-interface-back.png).  Positions marked
         "meas. [SPEC p5]" are measured on that 800x800 render, scale from
         the board outline = 181 mm (3.04 px/mm, about +/-1 mm).
  [LCD]  JC8048B070N.pdf (LCD panel approval sheet), p.3 table and p.4
         mechanical drawing: 164.9 x 100 x 3.5 module, AA 154.08 x 85.92,
         bezel open 156.55 x 89.5, offsets 6.87 / 5.5 / 80.94 / 47.73.
  [SCH]  ESP32-8048S070-LCM-V1.1.png / -MCU-V1.1.png (vendor schematics):
         RST/BOOT are "SW_3X6X4.3" tact switches, P1 = HC-1.25-4PWT,
         USB1 = Type-C, TF1 = push-push micro-SD, P6 = JST 1.25 2P speaker.
  [TT]   tinytronics-8048S070C-*.jpg (retailer listing): 181 x 108 mm,
         mounting holes 3.2 mm, USB-C.
  [PH]   dkm1978-IMG_*.jpg real photos of a board (front and back).
  "EST"  = estimated, not available from any primary drawing -> measure!
"""

TITLE = "ESP32-8048S070C (7\" 800x480, capacitive touch)"

# =============================================================================
# PCB
# =============================================================================
PCB_W = 181.0           # [DIM] printed 181.0; [SPEC p4] 181.0; [TT] 181
PCB_H = 108.0           # [DIM] printed 108.0; [SPEC p4] 108.0; [TT] 108
PCB_T = 1.6             # EST: standard FR4 thickness, not documented
PCB_CORNER_R = 3.0      # EST/meas. [DIM]: corner arc ends ~3.2 mm from edge

HOLE_D = 3.2            # [DIM] printed R1.6; [TT] "mounting holes 3.2 mm"
HOLE_INSET = 3.5        # [DIM] printed 3.5 from left & bottom edge;
                        # meas. [DIM] all four centres 3.5-3.7 from edges

# PCB notch at the right edge (front view) for the ESP32-S3 antenna
ANT_NOTCH = dict(edge="right", t0=44.6, t1=63.4, depth=6.8)   # meas. [DIM]
# Shallow notch in the bottom edge where the touch-panel FPC wraps around
TOUCH_FPC = dict(edge="bottom", t0=138.5, t1=166.5, notch=0.9,
                 relief=1.2)                                  # meas. [DIM]

# =============================================================================
# DISPLAY STACK (front side of the PCB)
# =============================================================================
LCD_W = 164.9           # [LCD] p.3/p.4 "164.9 +/-0.2"; [DIM] printed 165.0
LCD_H = 100.0           # [LCD] p.3/p.4; [DIM] printed 100.0
LCD_T = 3.5             # [LCD] p.3/p.4 "3.5 +/-0.2"
LCD_X0 = (PCB_W - LCD_W) / 2  # meas. [DIM] 7.9 left / 8.4 right -> centred
LCD_Y0 = (PCB_H - LCD_H) / 2  # meas. [DIM] 3.9 bottom / 4.1 top -> centred
LCD_TAPE = 0.2          # EST: adhesive between PCB and LCD back

# Active area (inside the LCD outline).  [LCD] p.4: AA 154.08 x 85.92;
# AA left margin 80.94-154.08/2 = 3.90, right 6.87; top 47.73-85.92/2 = 4.77,
# bottom (FPC side) 9.31.  The drawing could be a back view, so the
# horizontal margins may be swapped -- the window is valid either way.
AA_W = 154.08           # [LCD] p.3/p.4 ([SPEC p4] says 153.84 x 85.63)
AA_H = 85.92            # [LCD]
AA_MIN_MARGIN_X = 3.90  # [LCD] derived (smaller of 3.90 / 6.87..6.92)
AA_MIN_MARGIN_Y = 4.77  # [LCD] derived (smaller of 4.77 / 9.31)

# Capacitive touch cover glass
GLASS_W = LCD_W         # EST: photos [TT]/[PH] show glass ~= LCD outline
GLASS_H = LCD_H         # EST
GLASS_T = 1.8           # EST: typical 7" GT911 CTP (1.1 glass + sensor/OCA)
GLASS_X0 = LCD_X0
GLASS_Y0 = LCD_Y0

# =============================================================================
# BACK-SIDE COMPONENTS
# =============================================================================
BACK_MAX_H = 4.3        # [SCH] tallest part = SW_3X6X4.3 tact switches
SMD_ENVELOPE_H = 2.5    # EST: everything else (ICs, caps, FPC conns, inductors)

# Edge connectors.  edge = which PCB edge (front view), pos = centre along that
# edge, w/h/d = body width along the edge / height below the PCB back face /
# depth into the board, overhang = how far the body sticks out past the edge.
# cut = opening in the enclosure wall (False: placeholder only).
PORTS = {
    # USB-C receptacle: meas. [DIM] shell-leg pads at y_top 49.7/58.5;
    # agrees with meas. [SPEC p5] (54.1).  Body: EST generic 16P top-mount.
    "usb_c": dict(edge="left", pos=PCB_H - 54.1, w=8.94, h=3.26, d=7.35,
                  overhang=0.5, cut=True, cut_w=13.0, cut_h=7.5, fillet=1.5,
                  plug_w=12.0, plug_h=6.5, plug_len=30.0,
                  label="USB-C plug body"),
    # P1 UART/power JST 1.25 mm 4-pin (HC-1.25-4PWT, horizontal).
    # pos: meas. [SPEC p5] (+/-1 mm); body EST.
    "p1_uart": dict(edge="left", pos=PCB_H - 38.8, w=8.3, h=3.4, d=5.5,
                    overhang=0.0, cut=True, cut_w=9.0, cut_h=5.0, fillet=1.0,
                    plug_w=6.8, plug_h=3.4, label="P1 JST plug"),
    # Other top-edge connectors (meas. [SPEC p5], +/-1 mm; body EST).
    # Set cut=True to open them in the wall.
    "P2_SPI_JST1.25": dict(edge="top", pos=103.9, w=7.0, h=3.4, d=5.5,
                           cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.0,
                           plug_h=2.4),
    "P3_UART1_JST1.25": dict(edge="top", pos=83.3, w=7.0, h=3.4, d=5.5,
                             cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.0,
                             plug_h=2.4),
    "P4_IO_JST1.25": dict(edge="top", pos=63.2, w=7.0, h=3.4, d=5.5,
                          cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.0,
                          plug_h=2.4),
    "P5_SH1.0": dict(edge="top", pos=45.4, w=7.0, h=3.4, d=5.5, cut=False,
                     cut_w=9.0, cut_h=5.0, plug_w=6.0, plug_h=2.4),
    "P6_SPEAKER_JST1.25_2P": dict(edge="top", pos=30.4, w=7.0, h=3.4, d=5.5,
                                  cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.0,
                                  plug_h=2.4),
}

# micro-SD push-push socket on the top edge, card inserted towards -Y
SD = dict(edge="top",
          pos=129.3,            # meas. [DIM] socket mouth x 123.8..134.7
          overhang=1.6,         # meas. [DIM] protrudes 1.6 beyond top edge
          w=14.0, d=15.0, h=1.85,   # EST: generic push-push socket
          cut_w=14.0, cut_h=3.0, finger_r=8.0,
          card_w=11.0, card_t=1.0)  # micro-SD spec 11 x 15 x 1.0 max

# RESET / BOOT tact switches (3 x 6 x 4.3, push towards the PCB)
BUTTONS = {             # meas. [SPEC p5] back x 10.2 / 17.3, y_top 14.9
    "RESET": (PCB_W - 10.2, PCB_H - 14.9),
    "BOOT": (PCB_W - 17.3, PCB_H - 14.9),
}
BTN_W, BTN_L = 3.0, 6.0  # [SCH] SW_3X6X4.3 (long side along X, EST)

# ESP32-S3-WROOM-1 module (18 x 25.5 x 3.1 per Espressif datasheet)
ESP = dict(x0=155.2, x1=180.6,  # meas. [SPEC p5]
           y0=44.6, y1=63.4,    # meas. [SPEC p5] / [DIM] notch
           t=3.1)               # Espressif ESP32-S3-WROOM-1 datasheet

OBSTACLES = {}          # other tall parts: name -> (x0, y0, x1, y1, height)

# =============================================================================
# BOARD-SPECIFIC ENCLOSURE SETTINGS
# =============================================================================
VENT_FIELDS = [         # (x0, x1, y0, y1) slot fields, slots run along Y
    (30.0, 150.0, 22.0, 56.0),          # centre of the board
    (152.0, 172.0, 66.0, 84.0),         # near the ESP32-S3 module
]
KEYHOLE_XS = (PCB_W / 2 - 50.0, PCB_W / 2 + 50.0)   # 100 mm apart
KEYHOLE_Y = 72.0        # centre of the head-entry circle

# Zip-tie bar on the inside floor next to P1 (optional pigtail retention)
TIEBAR = dict(x0=14.0, yc=PORTS["p1_uart"]["pos"])

# P1 cable: exits the left wall, runs along it towards the top edge (away from
# USB-C), clamp on a tongue past the top-left corner.
CABLE = dict(port="p1_uart", route="side", run="top")
