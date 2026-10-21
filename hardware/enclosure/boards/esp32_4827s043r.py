"""
Sunton / Jingcai ESP32-4827S043R -- 4.3" 480x272 RGB TFT, XPT2046 resistive
touch, ESP32-S3-WROOM-1.  Board data for ../enclosure.py.

Frame: seen from the FRONT (screen towards you, landscape, USB-C and P1 on
the left edge, micro-SD on the bottom edge).  X 0..123 left->right, Y 0..74
bottom->top, Z = 0 PCB front face.  The vendor's component-side render is
mirrored left/right with respect to this frame (x_front = 123 - x_comp).

SOURCES (download into ../reference/4827S043R/ -- not committed)
  [DIM]  esp32-4827S043-back.png (rzeldent/platformio-espressif32-sunton,
         assets/images): vendor drawing of the display side with the LCD
         outline.  Printed dims: 123.0, 74.0, 105.5, 67.2, 3.5, 3.5, D3.2.
         Scale from the printed 123.0: 6.42 px/mm.
  [CMP]  esp32-4827S043-front.png (same repo): vendor render of the
         component side with the same printed dims.  Positions marked
         "meas. [CMP]" are measured on it, scale from the PCB outline
         (1032 px = 123 mm -> 8.38 px/mm, about +/-0.3 mm); converted to
         the front frame.
  [SCH]  ESP32-4827S043-MCU-V1.0.jpg (same repo, schematics).
  "EST"  = estimated, not available from any primary drawing -> measure!
"""

TITLE = "ESP32-4827S043R (4.3\" 480x272, resistive touch)"

# =============================================================================
# PCB
# =============================================================================
PCB_W = 123.0           # [DIM]/[CMP] printed 123.0
PCB_H = 74.0            # [DIM]/[CMP] printed 74.0
PCB_T = 1.6             # EST: standard FR4 thickness, not documented
PCB_CORNER_R = 3.0      # EST: corner arcs look like the 7" board

HOLE_D = 3.2            # [DIM]/[CMP] printed D3.2
HOLE_INSET = 3.5        # [DIM]/[CMP] printed 3.5 / 3.5; meas. [CMP] 3.3-3.7

# Notch in the bottom edge where the touch FPC wraps around to FPC1
TOUCH_FPC = dict(edge="bottom", t0=92.2, t1=108.0, notch=2.0,
                 relief=1.2)    # meas. [CMP] notch, [DIM] agrees (92.2..107.8)

# =============================================================================
# DISPLAY STACK (front side of the PCB)
# =============================================================================
LCD_W = 105.5           # [DIM]/[CMP] printed 105.5
LCD_H = 67.2            # [DIM]/[CMP] printed 67.2
LCD_T = 3.0             # EST: typical 4.3" 480x272 TFT module 2.9-3.1 mm
LCD_X0 = (PCB_W - LCD_W) / 2  # meas. [DIM] 8.7 left / 8.9 right -> centred
LCD_Y0 = (PCB_H - LCD_H) / 2  # meas. [DIM] 3.4 top / 3.7 bottom -> centred
LCD_TAPE = 0.2          # EST: adhesive between PCB and LCD back

# Active area: EST, typical 4.3" 480x272 panel 95.04 x 53.86 mm.  Margins
# inside the LCD outline are not documented; the minimums below are
# conservative (centred would be 5.2 / 6.7).
AA_W = 95.04            # EST
AA_H = 53.86            # EST
AA_MIN_MARGIN_X = 4.0   # EST
AA_MIN_MARGIN_Y = 3.0   # EST

# Resistive touch panel on top of the LCD
GLASS_W = LCD_W         # EST: touch panel = LCD outline
GLASS_H = LCD_H         # EST
GLASS_T = 1.3           # EST: typical 4-wire resistive panel 1.1-1.5 mm
GLASS_X0 = LCD_X0
GLASS_Y0 = LCD_Y0

# =============================================================================
# BACK-SIDE COMPONENTS
# =============================================================================
BACK_MAX_H = 4.3        # EST: tact switches (same family as the 7" board)
SMD_ENVELOPE_H = 2.5    # EST: ICs, regulators (SOT-223), FPC connectors

PORTS = {
    # USB-C receptacle, left edge.  meas. [CMP] y_top 29.0..37.9 -> centre
    # 40.5 from the bottom, sticks out ~0.5 mm; body EST generic 16P.
    "usb_c": dict(edge="left", pos=40.5, w=8.94, h=3.26, d=7.35,
                  overhang=0.5, cut=True, cut_w=13.0, cut_h=7.5, fillet=1.5,
                  plug_w=12.0, plug_h=6.5, plug_len=30.0,
                  label="USB-C plug body"),
    # P1 UART/power JST 1.25 mm 4-pin (GND RXD TXD 5V), left edge, opening
    # faces the edge.  meas. [CMP] centre 23.5 from the bottom; body EST.
    "p1_uart": dict(edge="left", pos=23.5, w=8.3, h=3.4, d=5.5,
                    overhang=0.0, cut=True, cut_w=9.0, cut_h=5.0, fillet=1.0,
                    plug_w=6.8, plug_h=3.4, label="P1 JST plug"),
    # Bottom-edge connectors (meas. [CMP] centres; body EST).
    # Set cut=True to open them in the wall.
    "P2_SPI_JST1.25": dict(edge="bottom", pos=49.1, w=8.3, h=3.4, d=4.8,
                           cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.8,
                           plug_h=3.4),
    "P3_UART1_USB_JST1.25": dict(edge="bottom", pos=29.8, w=8.3, h=3.4, d=4.8,
                                 cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.8,
                                 plug_h=3.4),
    "P4_IO_JST1.25": dict(edge="bottom", pos=15.1, w=7.5, h=3.4, d=4.8,
                          cut=False, cut_w=9.0, cut_h=5.0, plug_w=6.0,
                          plug_h=3.4),
}

# micro-SD push-push socket on the bottom edge, card inserted towards +Y.
# meas. [CMP]: socket x 72.5 +/- 7.4, card centre 73.4, socket flush with
# the edge, an inserted card sticks out ~1.9 mm.
SD = dict(edge="bottom", pos=73.4, overhang=0.3,
          w=14.8, d=15.0, h=1.85,       # [CMP] footprint; height EST
          cut_w=14.0, cut_h=3.0, finger_r=8.0,
          card_w=11.0, card_t=1.0)

# RESET / BOOT tact switches, pressed from the back (meas. [CMP])
BUTTONS = {
    "RESET": (117.0, 31.0),
    "BOOT": (117.0, 20.9),
}
BTN_W, BTN_L = 6.0, 3.0  # long side along Y here (meas. [CMP])

# ESP32-S3-WROOM-1 module (18 x 25.5 x 3.1 per Espressif datasheet),
# antenna at the right edge.  meas. [CMP]
ESP = dict(x0=97.1, x1=122.6, y0=39.7, y1=58.2, t=3.1)

OBSTACLES = {
    # touch FPC connector FPC1 and LCD FPC connector LCD1 (meas. [CMP])
    "fpc1_touch": (97.6, 22.1, 110.9, 28.7, 2.5),
    "lcd1_fpc": (26.8, 41.8, 52.5, 47.2, 2.5),
}

# =============================================================================
# BOARD-SPECIFIC ENCLOSURE SETTINGS
# =============================================================================
VENT_FIELDS = [         # (x0, x1, y0, y1) slot fields, slots run along Y
    (40.0, 82.0, 10.0, 34.0),           # centre of the board
    (100.0, 120.0, 42.0, 56.0),         # over the ESP32-S3 module
]
KEYHOLE_XS = (PCB_W / 2 - 27.0, PCB_W / 2 + 27.0)   # 54 mm apart, clear of
KEYHOLE_Y = 38.0                                    # the ESP32 module

TIEBAR = dict(x0=13.0, yc=PORTS["p1_uart"]["pos"])

# P1 cable: exits the left wall, runs along it towards the BOTTOM edge (P1 is
# below USB-C on this board), clamp on a tongue past the bottom-left corner.
CABLE = dict(port="p1_uart", route="side", run="bottom")
