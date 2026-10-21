"""
Sunton ESP32-2432S028R "Cheap Yellow Display" -- 2.8" 320x240 ILI9341 TFT,
XPT2046 resistive touch, ESP32-WROOM-32.  Board data for ../enclosure.py.

Frame: seen from the FRONT (screen towards you, landscape with the micro-USB
on the right edge and the ESP32 antenna on the left, as in the [WM] photo).
X 0..86 left->right, Y 0..50 bottom->top, Z = 0 PCB front face.

SOURCES (download into ../reference/2432S028R/ -- not committed)
  [DIM]  esp32-2432S028R-back.png (rzeldent/platformio-espressif32-sunton)
         and OriginalDocumentation/3-Structure_Diagram/Dimensions.png
         (witnessmenow/ESP32-Cheap-Yellow-Display): vendor drawing, printed
         78.0 x 42.0 hole pitch, 4.0 / 4.0 edge distance, R1.60 holes.  The
         landscape drawing is the front view rotated by 180 degrees (USB on
         the left there).  Scale 7.37 px/mm.
  [HV]   Cheap Yellow Display v16.step by Havenview (witnessmenow repo,
         3dModels/Havenview_CYD_CAD, MIT licence): board model used for that
         case.  Positions marked [HV] are bounding boxes of the board solid
         sliced in Z, converted to this frame (x = x_hv + 43, y = y_hv + 25).
         Its USB / SD / antenna positions agree with [DIM].
  [WM]   witnessmenow README photo (orientation) and PINS.md (connector
         types: P1 serial, P3 + CN1 GPIO, P4 speaker; all Molex PicoBlade
         1.25 mm).
  "EST"  = estimated, not available from any primary drawing -> measure!
"""

TITLE = "ESP32-2432S028R (2.8\" 320x240 CYD, resistive touch)"

# =============================================================================
# PCB
# =============================================================================
PCB_W = 86.0            # [DIM] 78.0 + 2 x 4.0; [HV] 86.0
PCB_H = 50.0            # [DIM] 42.0 + 2 x 4.0; [HV] 50.0
PCB_T = 1.6             # EST ([HV] models 1.5)
PCB_CORNER_R = 3.0      # EST

HOLE_D = 3.2            # [DIM] R1.60
HOLE_INSET = 4.0        # [DIM] printed 4.0 / 4.0

# PCB notch under the ESP32 antenna, left edge (meas. [DIM]: 8.2 deep,
# y 15.8..34.7; [HV] module y 16..34)
ANT_NOTCH = dict(edge="left", t0=15.8, t1=34.7, depth=8.2)

# =============================================================================
# DISPLAY STACK (front side of the PCB)
# =============================================================================
LCD_W = 69.0            # [HV] display body 69.0 x 50.0, centred in X
LCD_H = 50.0            # [HV] (full PCB height; [DIM] LCD outline agrees)
LCD_X0 = (PCB_W - LCD_W) / 2
LCD_Y0 = 0.0
LCD_TAPE = 0.0
LCD_T = 2.6             # EST split of the [HV] stack (3.6 mm PCB face ->
GLASS_T = 1.0           #     top of the touch panel): LCD 2.6 + touch 1.0

# Active area: EST, typical 2.8" ILI9341 panel 57.6 x 43.2 mm; the wider
# (FPC) border is on one short side, so X margins are 3.3 / 8.1.
AA_W = 57.6             # EST
AA_H = 43.2             # EST
AA_MIN_MARGIN_X = 3.3   # EST
AA_MIN_MARGIN_Y = 3.4   # EST (centred: (50 - 43.2) / 2)

GLASS_W = LCD_W         # resistive touch panel = display outline ([HV])
GLASS_H = LCD_H
GLASS_X0 = LCD_X0
GLASS_Y0 = LCD_Y0

# =============================================================================
# BACK-SIDE COMPONENTS
# =============================================================================
BACK_MAX_H = 4.7        # [HV] button actuators
SMD_ENVELOPE_H = 2.5    # EST

PORTS = {
    # micro-USB, right edge: [HV] y 11.8..19.6, 0.6 mm past the edge, height
    # 2.6-3.2 ([DIM] shell legs at the same place).  Cut sized for a micro-USB
    # plug overmould (~10.5 x 7 mm).  Newer CYD revisions add a USB-C socket
    # -- check your board.
    "usb": dict(edge="right", pos=15.7, w=7.8, h=3.0, d=5.5, overhang=0.6,
                cut=True, cut_w=11.5, cut_h=7.5, fillet=1.5, plug_w=10.5,
                plug_h=6.5, plug_len=25.0, label="micro-USB plug body"),
    # P3 and CN1 (4P PicoBlade, side entry) side by side on the bottom edge:
    # [HV] x 10.2..20.2 and 26.2..36.2, 4.3 deep, ~3.5 high.  The firmware
    # uses CN1 (GPIO 27/22); [HV] does not say which one is CN1, so one
    # opening exposes both and the cable pocket accepts either.
    "p3_cn1": dict(edge="bottom", pos=23.2, w=26.0, h=3.5, d=4.3,
                   overhang=0.0, cut=True, cut_w=27.0, cut_h=5.0, fillet=1.0,
                   plug_w=24.0, plug_h=3.4, label="P3 / CN1 JST plugs"),
    # P1 serial (VIN TX RX GND) at the right edge next to the USB ([HV]
    # 4.5 x 10 x 3.5 body; identification EST).  Not opened by default.
    "p1_serial": dict(edge="right", pos=34.0, w=10.0, h=3.5, d=4.5,
                      overhang=-1.0, cut=False, cut_w=11.0, cut_h=5.0,
                      plug_w=8.0, plug_h=3.4),
}

# micro-SD push-push socket on the bottom edge, card inserted towards +Y
SD = dict(edge="bottom", pos=47.9,      # [HV] socket x 40.4..55.4; [DIM]
          overhang=0.0,                 # [HV] flush with the edge
          w=15.0, d=15.0, h=1.8,        # [HV]
          cut_w=14.0, cut_h=3.0, finger_r=8.0,
          card_w=11.0, card_t=1.0)

# RESET / BOOT tact switches pressed from the back ([HV] bodies 3.5 x 6,
# actuators 1.5 x 3; which one is BOOT is not marked in [HV])
BUTTONS = {
    "BTN_A": (9.35, 45.2),
    "BTN_B": (13.35, 45.2),
}
BTN_W, BTN_L = 6.0, 3.5
BTN_HOLE_D = 3.4        # holes are only 4 mm apart

# ESP32-WROOM-32 module: [HV] can x 7.0..25.4, antenna part x 0..7, y 16..34
ESP = dict(x0=0.0, x1=25.4, y0=16.0, y1=34.0, t=3.2)

OBSTACLES = {
    "tall_part_top_edge": (54.4, 45.6, 62.0, 48.8, 4.5),   # [HV], unlabelled
    "ic_5x5": (28.0, 8.5, 33.0, 13.5, 1.0),                # [HV]
}

# =============================================================================
# BOARD-SPECIFIC ENCLOSURE SETTINGS
# =============================================================================
VENT_FIELDS = [         # (x0, x1, y0, y1) slot fields, slots run along Y
    (28.0, 62.0, 18.0, 40.0),           # centre (slots near keyholes skipped)
    (9.0, 24.0, 19.0, 31.0),            # over the ESP32 module
]
KEYHOLE_XS = (34.0, 70.0)               # 36 mm apart, clear of the ESP32
KEYHOLE_Y = 20.0
KEYHOLE_TRAVEL = 8.0

TIEBAR = None

# P3/CN1 face the bottom wall: the cable goes straight down through a pocket
# into the clamp on a tongue below the case.
CABLE = dict(port="p3_cn1", route="straight")
