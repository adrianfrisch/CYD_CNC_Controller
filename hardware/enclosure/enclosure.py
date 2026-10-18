#!/usr/bin/env python3
"""
Parametric two-part enclosure for the Sunton / Jingcai ESP32-8048S070C
(7" 800x480 RGB TFT, GT911 capacitive touch, ESP32-S3-WROOM-1).

Parts
  * front bezel  - frames the display, 2 mm lip over the glass edge,
                   M3 heat-set inserts in four corner bosses
  * back shell   - houses the board on four corner standoffs, connector
                   cut-outs, vents, button holes, two keyhole slots

The same four M3 screws (inserted from the back) pass through the shell
standoffs, through the PCB mounting holes, into the inserts in the bezel:
they clamp the PCB between the standoffs and the bezel bosses and hold the
two halves together.

Run (CadQuery 2.x):
    python enclosure.py            # build, export STEP/STL, check, render PNGs
    python enclosure.py --no-png   # skip the matplotlib previews

COORDINATE SYSTEM (all values in mm)
  Seen from the FRONT (looking at the screen, landscape, touch-FPC notch at
  the bottom edge, USB-C on the left edge):
    X = 0 .. 181   PCB left edge -> right edge
    Y = 0 .. 108   PCB bottom edge -> top edge
    Z = 0          PCB front face (the face the LCD is stuck to)
    +Z             towards the viewer;  PCB back face is at Z = -PCB_T
  The vendor's "back" pictures are mirrored left/right with respect to this
  frame (x_front = 181 - x_back).

SOURCES (download into ./reference/ — not committed, see README.md)
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

import math
import os
import sys

import cadquery as cq

HERE = os.path.dirname(os.path.abspath(__file__))

# =============================================================================
# BOARD DIMENSIONS
# =============================================================================
PCB_W = 181.0           # [DIM] printed 181.0; [SPEC p4] 181.0; [TT] 181
PCB_H = 108.0           # [DIM] printed 108.0; [SPEC p4] 108.0; [TT] 108
PCB_T = 1.6             # EST: standard FR4 thickness, not documented
PCB_CORNER_R = 3.0      # EST/meas. [DIM]: corner arc ends ~3.2 mm from edge

HOLE_D = 3.2            # [DIM] printed R1.6; [TT] "mounting holes 3.2 mm"
HOLE_INSET = 3.5        # [DIM] printed 3.5 from left & bottom edge;
                        # meas. [DIM] all four centres 3.5-3.7 from edges
HOLES = [(HOLE_INSET, HOLE_INSET), (PCB_W - HOLE_INSET, HOLE_INSET),
         (HOLE_INSET, PCB_H - HOLE_INSET), (PCB_W - HOLE_INSET, PCB_H - HOLE_INSET)]

# PCB notch at the right edge (front view) for the ESP32-S3 antenna
ANT_NOTCH_X0 = 174.2    # meas. [DIM] (notch depth 6.8 mm)
ANT_NOTCH_Y0 = 44.6     # meas. [DIM] (y_top 44.6..63.4 -> symmetric)
ANT_NOTCH_Y1 = 63.4     # meas. [DIM]
# Shallow notch in the bottom edge where the touch-panel FPC wraps around
TOUCH_FPC_X0 = 138.5    # meas. [DIM]
TOUCH_FPC_X1 = 166.5    # meas. [DIM]

# ---- LCD panel (front side of the PCB) --------------------------------------
LCD_W = 164.9           # [LCD] p.3/p.4 "164.9 +/-0.2"; [DIM] printed 165.0
LCD_H = 100.0           # [LCD] p.3/p.4; [DIM] printed 100.0
LCD_T = 3.5             # [LCD] p.3/p.4 "3.5 +/-0.2"
LCD_X0 = (PCB_W - LCD_W) / 2  # meas. [DIM] 7.9 left / 8.4 right -> centred
LCD_Y0 = (PCB_H - LCD_H) / 2  # meas. [DIM] 3.9 bottom / 4.1 top -> centred
LCD_TAPE = 0.2          # EST: adhesive between PCB and LCD back

# Active area (inside the LCD outline).  [LCD] p.4: AA 154.08 x 85.92;
# AA left margin 80.94-154.08/2 = 3.90, right 6.87; top 47.73-85.92/2 = 4.77,
# bottom (FPC side) 9.31.  The drawing could be a back view, so the
# horizontal margins may be swapped -- the window below is valid either way.
AA_W = 154.08           # [LCD] p.3/p.4 ([SPEC p4] says 153.84 x 85.63)
AA_H = 85.92            # [LCD]
AA_MIN_MARGIN_X = 3.90  # [LCD] derived (smaller of 3.90 / 6.87..6.92)
AA_MIN_MARGIN_Y = 4.77  # [LCD] derived (smaller of 4.77 / 9.31)
AA_X0 = LCD_X0 + 3.90   # [LCD] assuming the drawing is a front view
AA_Y0 = LCD_Y0 + 9.31   # [LCD] FPC (wide border) side at the bottom

# ---- capacitive touch cover glass --------------------------------------------
GLASS_W = LCD_W         # EST: photos [TT]/[PH] show glass ~= LCD outline
GLASS_H = LCD_H         # EST
GLASS_T = 1.8           # EST: typical 7" GT911 CTP (1.1 glass + sensor/OCA)
GLASS_X0 = LCD_X0
GLASS_Y0 = LCD_Y0
Z_LCD0 = LCD_TAPE
Z_GLASS0 = Z_LCD0 + LCD_T
Z_GLASS_TOP = Z_GLASS0 + GLASS_T          # = 5.5 with the defaults

# ---- back-side components -----------------------------------------------------
BACK_MAX_H = 4.3        # [SCH] tallest part = SW_3X6X4.3 tact switches
SMD_ENVELOPE_H = 2.5    # EST: everything else (ICs, caps, FPC conns, inductors)

# USB-C receptacle, left edge (front view), opening faces -X
USB_YC = PCB_H - 54.1   # meas. [DIM] shell-leg pads at y_top 49.7/58.5;
                        # agrees with meas. [SPEC p5] (54.1)
USB_W, USB_H, USB_D = 8.94, 3.26, 7.35   # EST: generic 16P top-mount USB-C
USB_OVERHANG = 0.5      # EST: receptacle lip beyond PCB edge
USB_PLUG_W, USB_PLUG_H = 12.0, 6.5       # EST: typical USB-C plug overmould

# P1 UART/power JST 1.25 mm 4-pin (HC-1.25-4PWT, horizontal), left edge
P1_YC = PCB_H - 38.8    # meas. [SPEC p5] (+/-1 mm)
P1_W, P1_H, P1_D = 8.3, 3.4, 5.5        # EST: 1.25 mm 4P right-angle header
P1_PLUG_W, P1_PLUG_H = 6.8, 3.4          # EST: 4P 1.25 mm plug housing

# micro-SD push-push socket on the top edge, card inserted towards -Y
SD_XC = 129.3           # meas. [DIM] socket mouth x 123.8..134.7
SD_OVERHANG = 1.6       # meas. [DIM] socket protrudes 1.6 beyond top edge
SD_W, SD_D, SD_H = 14.0, 15.0, 1.85      # EST: generic push-push socket
SD_CARD_W, SD_CARD_T = 11.0, 1.0         # micro-SD spec 11 x 15 x 1.0 max

# Other top-edge connectors (front-view x of the connector centre)
TOP_CONNECTORS = {      # meas. [SPEC p5], +/-1 mm
    "P2_SPI_JST1.25": 103.9,
    "P3_UART1_JST1.25": 83.3,
    "P4_IO_JST1.25": 63.2,
    "P5_SH1.0": 45.4,
    "P6_SPEAKER_JST1.25_2P": 30.4,
}
TOP_CONN_W, TOP_CONN_H, TOP_CONN_D = 7.0, 3.4, 5.5   # EST

# RESET / BOOT tact switches (3 x 6 x 4.3, push towards the PCB)
BUTTONS = {             # meas. [SPEC p5] back x 10.2 / 17.3, y_top 14.9
    "RESET": (PCB_W - 10.2, PCB_H - 14.9),
    "BOOT": (PCB_W - 17.3, PCB_H - 14.9),
}
BTN_W, BTN_L = 3.0, 6.0  # [SCH] SW_3X6X4.3 (long side along X, EST)

# ESP32-S3-WROOM-1 module (18 x 25.5 x 3.1 per Espressif datasheet)
ESP_X0, ESP_X1 = 155.2, 180.6   # meas. [SPEC p5]
ESP_Y0, ESP_Y1 = 44.6, 63.4     # meas. [SPEC p5] / [DIM] notch
ESP_T = 3.1                     # Espressif ESP32-S3-WROOM-1 datasheet

# =============================================================================
# ENCLOSURE PARAMETERS (design defaults)
# =============================================================================
WALL = 2.0              # side wall / floor / plate thickness
CLR = 0.3               # clearance PCB/glass outline -> inner wall
BEZEL_T = 2.0           # front plate thickness
GLASS_GAP = 0.2         # air gap between glass and bezel lip (no pressure)
LIP = 2.0               # bezel overlap onto the glass edge
WINDOW_R = 1.0          # window corner radius
BACK_CLEAR = 7.0        # PCB back face -> inner floor (BACK_MAX_H 4.3 + margin
                        # + room for the keyhole pockets)
SEAM_LIP_H = 1.5        # shell alignment lip height above the seam (Z=0)
SEAM_LIP_T = 1.0        # shell alignment lip thickness
SEAM_LIP_CLR = 0.15     # clearance of the matching rabbet in the bezel

# Fasteners: M3 heat-set inserts in the bezel, screws from the back.
INSERT_HOLE_D = 4.0     # M3 x 4 mm short heat-set insert (OD ~4.6) -> 4.0 hole
INSERT_HOLE_DEPTH = 6.0
BEZEL_BOSS_D = 7.0      # must stay clear of the glass corner (x0 = 8.05)
SHELL_BOSS_D = 7.0
SCREW_CLEAR_D = 3.4     # M3 clearance
SCREW_HEAD_D = 6.2      # counterbore for M3 ISO 4762 / 7380 head (5.5-5.7)
SCREW_HEAD_DEPTH = 3.0

# Cut-out sizes
USB_CUT_W, USB_CUT_H = 13.0, 7.5        # plug-body clearance (12 x 6.5 plug)
P1_CUT_W, P1_CUT_H = 9.0, 5.0           # lets the 4P plug pass
SD_CUT_W, SD_CUT_H = 14.0, 3.0
SD_FINGER_R = 8.0                       # finger scallop for push-push card
BTN_HOLE_D = 4.0
TOUCH_FPC_RELIEF = 1.2                  # wall relief where touch FPC wraps
EXPOSE_TOP_CONNECTORS = []              # e.g. ["P6_SPEAKER_JST1.25_2P"]
TOP_CONN_CUT_W, TOP_CONN_CUT_H = 9.0, 5.0

# Ventilation (slots through the back)
VENT_W = 2.0
VENT_PITCH = 5.0
VENT_FIELDS = [         # (x0, x1, y0, y1) slot fields, slots run along Y
    (30.0, 150.0, 22.0, 56.0),          # centre of the board
    (152.0, 172.0, 66.0, 84.0),         # near the ESP32-S3 module
]

# Keyhole slots for wall/machine mounting (screw head <= 7.5 mm, shank <= 3.8)
KEYHOLES_ENABLE = True
KEYHOLE_XS = (PCB_W / 2 - 50.0, PCB_W / 2 + 50.0)   # 100 mm apart
KEYHOLE_Y = 72.0        # centre of the head-entry circle
KEYHOLE_HEAD_D = 8.0
KEYHOLE_SLOT_W = 4.0
KEYHOLE_TRAVEL = 10.0   # slot length (screw hangs at Y = KEYHOLE_Y + travel)
KEYHOLE_POCKET_H = 2.5  # room for the screw head inside the pad
KEYHOLE_ROOF = 1.5

# P1 cable strain relief: zip-tie bar on the floor next to P1
TIEBAR_X0 = 14.0
TIEBAR_W = 3.0
TIEBAR_LEN = 13.0
TIEBAR_TUNNEL_H = 2.0
TIEBAR_TOP_T = 1.5

# =============================================================================
# DERIVED Z LEVELS
# =============================================================================
Z_PCB_BACK = -PCB_T
Z_FLOOR_IN = Z_PCB_BACK - BACK_CLEAR
Z_FLOOR_OUT = Z_FLOOR_IN - WALL
Z_BEZEL_IN = Z_GLASS_TOP + GLASS_GAP
Z_BEZEL_OUT = Z_BEZEL_IN + BEZEL_T
Z_SEAM = 0.0
CONN_Z0 = Z_PCB_BACK    # top-mount connectors hang from the PCB back face


# =============================================================================
# HELPERS
# =============================================================================
def box(x0, y0, z0, x1, y1, z1):
    return (cq.Workplane("XY")
            .box(x1 - x0, y1 - y0, z1 - z0, centered=False)
            .translate((x0, y0, z0)))


def rrect(x0, y0, x1, y1, r, z0, z1):
    """Rounded rectangle (vertical edges filleted) extruded z0..z1."""
    s = box(x0, y0, z0, x1, y1, z1)
    if r > 0.01:
        s = s.edges("|Z").fillet(r)
    return s


def outline(offset, z0, z1):
    """PCB outline offset outwards by `offset`, extruded z0..z1."""
    return rrect(-offset, -offset, PCB_W + offset, PCB_H + offset,
                 PCB_CORNER_R + offset, z0, z1)


def cyl(x, y, d, z0, z1):
    return (cq.Workplane("XY").circle(d / 2).extrude(z1 - z0)
            .translate((x, y, z0)))


def slot_y(x, y0, y1, w, z0, z1):
    """Stadium slot along Y from centre y0 to centre y1."""
    return (cq.Workplane("XY")
            .center(x, (y0 + y1) / 2)
            .slot2D(y1 - y0 + w, w, angle=90)
            .extrude(z1 - z0).translate((0, 0, z0)))


def ybox_x(xc, w, y0, y1, z0, z1):
    return box(xc - w / 2, y0, z0, xc + w / 2, y1, z1)


def xbox_y(yc, w, x0, x1, z0, z1):
    return box(x0, yc - w / 2, z0, x1, yc + w / 2, z1)


# =============================================================================
# BOARD PLACEHOLDER (fit-check body)
# =============================================================================
def build_board():
    parts = {}
    pcb = outline(0.0, Z_PCB_BACK, 0.0)
    pcb = pcb.cut(box(ANT_NOTCH_X0, ANT_NOTCH_Y0, Z_PCB_BACK - 1,
                      PCB_W + 1, ANT_NOTCH_Y1, 1))
    pcb = pcb.cut(box(TOUCH_FPC_X0, -1, Z_PCB_BACK - 1, TOUCH_FPC_X1, 0.9, 1))
    for x, y in HOLES:
        pcb = pcb.cut(cyl(x, y, HOLE_D, Z_PCB_BACK - 1, 1))
    parts["pcb"] = pcb

    parts["lcd"] = box(LCD_X0, LCD_Y0, Z_LCD0, LCD_X0 + LCD_W, LCD_Y0 + LCD_H,
                       Z_LCD0 + LCD_T)
    parts["glass"] = box(GLASS_X0, GLASS_Y0, Z_GLASS0, GLASS_X0 + GLASS_W,
                         GLASS_Y0 + GLASS_H, Z_GLASS_TOP)

    zb = CONN_Z0
    parts["usb_c"] = xbox_y(USB_YC, USB_W, -USB_OVERHANG, USB_D - USB_OVERHANG,
                            zb - USB_H, zb)
    parts["p1_uart"] = xbox_y(P1_YC, P1_W, 0.0, P1_D, zb - P1_H, zb)
    parts["sd_socket"] = ybox_x(SD_XC, SD_W, PCB_H + SD_OVERHANG - SD_D,
                                PCB_H + SD_OVERHANG, zb - SD_H, zb)
    for name, xc in TOP_CONNECTORS.items():
        parts[name] = ybox_x(xc, TOP_CONN_W, PCB_H - TOP_CONN_D, PCB_H,
                             zb - TOP_CONN_H, zb)
    for name, (x, y) in BUTTONS.items():
        parts["sw_" + name] = box(x - BTN_L / 2, y - BTN_W / 2, zb - BACK_MAX_H,
                                  x + BTN_L / 2, y + BTN_W / 2, zb)
    parts["esp32s3"] = box(ESP_X0, ESP_Y0, zb - ESP_T, ESP_X1, ESP_Y1, zb)

    # generic envelope for all small SMD parts, kept out of the screw bosses
    env = box(1.0, 1.0, zb - SMD_ENVELOPE_H, PCB_W - 1.0, PCB_H - 1.0, zb)
    for x, y in HOLES:
        env = env.cut(cyl(x, y, SHELL_BOSS_D + 1.0, zb - 10, zb + 1))
    env = env.cut(box(ANT_NOTCH_X0, ANT_NOTCH_Y0, zb - 10, PCB_W + 1,
                      ANT_NOTCH_Y1, zb + 1))
    parts["smd_envelope"] = env
    return parts


def union_all(shapes):
    it = iter(shapes)
    acc = next(it)
    for s in it:
        acc = acc.union(s)
    return acc


# =============================================================================
# CUT-OUTS shared by both halves (anything crossing the seam cuts both)
# =============================================================================
def side_cutouts():
    cuts = []
    out = CLR + WALL + 2.0
    # USB-C: left wall, plug body clearance, centred on the receptacle
    usb_zc = CONN_Z0 - USB_H / 2
    cuts.append(rrect(-out, USB_YC - USB_CUT_W / 2, 0.5, USB_YC + USB_CUT_W / 2,
                      0.0, usb_zc - USB_CUT_H / 2, usb_zc + USB_CUT_H / 2)
                .edges("|X").fillet(1.5))
    # P1 cable exit: left wall
    p1_zc = CONN_Z0 - P1_H / 2
    cuts.append(box(-out, P1_YC - P1_CUT_W / 2, p1_zc - P1_CUT_H / 2,
                    0.5, P1_YC + P1_CUT_W / 2, p1_zc + P1_CUT_H / 2)
                .edges("|X").fillet(1.0))
    # micro-SD slot: top wall + finger scallop on the outside
    sd_zc = CONN_Z0 - SD_H / 2
    cuts.append(box(SD_XC - SD_CUT_W / 2, PCB_H - 0.5, sd_zc - SD_CUT_H / 2,
                    SD_XC + SD_CUT_W / 2, PCB_H + out, sd_zc + SD_CUT_H / 2))
    y_out = PCB_H + CLR + WALL
    cuts.append(cyl(SD_XC, y_out + SD_FINGER_R - 1.2, 2 * SD_FINGER_R,
                    sd_zc - 3.0, min(sd_zc + 3.0, Z_SEAM - 0.01)))
    # optional top-edge connectors
    for name in EXPOSE_TOP_CONNECTORS:
        xc = TOP_CONNECTORS[name]
        zc = CONN_Z0 - TOP_CONN_H / 2
        cuts.append(box(xc - TOP_CONN_CUT_W / 2, PCB_H - 0.5,
                        zc - TOP_CONN_CUT_H / 2, xc + TOP_CONN_CUT_W / 2,
                        PCB_H + out, zc + TOP_CONN_CUT_H / 2))
    # touch FPC wraps around the bottom edge: relief in the inner wall face
    cuts.append(box(TOUCH_FPC_X0 - 1.0, -CLR - TOUCH_FPC_RELIEF, Z_PCB_BACK - 1.5,
                    TOUCH_FPC_X1 + 1.0, 0.5, Z_GLASS_TOP))
    return cuts


# =============================================================================
# FRONT BEZEL
# =============================================================================
def build_bezel():
    plate = outline(CLR + WALL, Z_BEZEL_IN, Z_BEZEL_OUT)
    walls = outline(CLR + WALL, Z_SEAM, Z_BEZEL_IN).cut(
        outline(CLR, Z_SEAM - 1, Z_BEZEL_IN + 1))
    # rabbet that receives the shell's alignment lip
    walls = walls.cut(outline(CLR + SEAM_LIP_T + SEAM_LIP_CLR, Z_SEAM - 1,
                              Z_SEAM + SEAM_LIP_H + SEAM_LIP_CLR))
    bezel = plate.union(walls)

    # display window = glass minus LIP on every side
    wx0, wy0 = GLASS_X0 + LIP, GLASS_Y0 + LIP
    wx1, wy1 = GLASS_X0 + GLASS_W - LIP, GLASS_Y0 + GLASS_H - LIP
    bezel = bezel.cut(rrect(wx0, wy0, wx1, wy1, WINDOW_R,
                            Z_BEZEL_IN - 1, Z_BEZEL_OUT + 1))

    # corner bosses (rest on the PCB front face) with insert holes
    zb0 = Z_SEAM
    for x, y in HOLES:
        boss = cyl(x, y, BEZEL_BOSS_D, zb0, Z_BEZEL_IN + 0.01)
        # web joining the boss to the corner walls (above the rabbet)
        cx = -CLR - 0.5 if x < PCB_W / 2 else PCB_W + CLR + 0.5
        cy = -CLR - 0.5 if y < PCB_H / 2 else PCB_H + CLR + 0.5
        web = box(min(x, cx), min(y, cy), Z_SEAM + SEAM_LIP_H + SEAM_LIP_CLR,
                  max(x, cx), max(y, cy), Z_BEZEL_IN + 0.01)
        bezel = bezel.union(boss).union(web)
        bezel = bezel.cut(cyl(x, y, INSERT_HOLE_D, zb0 - 1,
                              zb0 + INSERT_HOLE_DEPTH))
    for c in side_cutouts():
        bezel = bezel.cut(c)
    return bezel


# =============================================================================
# BACK SHELL
# =============================================================================
def keyhole_pad(x):
    y_top = KEYHOLE_Y + KEYHOLE_TRAVEL
    pad_h = KEYHOLE_POCKET_H + KEYHOLE_ROOF
    pad = rrect(x - 7.0, KEYHOLE_Y - 6.0, x + 7.0, y_top + 6.0, 2.0,
                Z_FLOOR_IN - 0.01, Z_FLOOR_IN + pad_h)
    return pad


def keyhole_cuts(x):
    y_top = KEYHOLE_Y + KEYHOLE_TRAVEL
    cuts = [
        # through the floor: head-entry circle + shank slot
        cyl(x, KEYHOLE_Y, KEYHOLE_HEAD_D, Z_FLOOR_OUT - 1, Z_FLOOR_IN + 0.01),
        slot_y(x, KEYHOLE_Y, y_top, KEYHOLE_SLOT_W, Z_FLOOR_OUT - 1,
               Z_FLOOR_IN + 0.01),
        # pocket for the screw head inside the pad
        slot_y(x, KEYHOLE_Y, y_top, KEYHOLE_HEAD_D, Z_FLOOR_IN - 0.01,
               Z_FLOOR_IN + KEYHOLE_POCKET_H),
    ]
    return cuts


def build_shell():
    floor = outline(CLR + WALL, Z_FLOOR_OUT, Z_FLOOR_IN)
    walls = outline(CLR + WALL, Z_FLOOR_IN - 0.01, Z_SEAM).cut(
        outline(CLR, Z_FLOOR_IN - 1, Z_SEAM + 1))
    lip = outline(CLR + SEAM_LIP_T, Z_SEAM - 0.01, Z_SEAM + SEAM_LIP_H).cut(
        outline(CLR, Z_SEAM - 1, Z_SEAM + SEAM_LIP_H + 1))
    shell = floor.union(walls).union(lip)

    # standoffs at the PCB mounting holes
    for x, y in HOLES:
        boss = cyl(x, y, SHELL_BOSS_D, Z_FLOOR_IN - 0.01, Z_PCB_BACK)
        cx = -CLR - 0.5 if x < PCB_W / 2 else PCB_W + CLR + 0.5
        cy = -CLR - 0.5 if y < PCB_H / 2 else PCB_H + CLR + 0.5
        web = box(min(x, cx), min(y, cy), Z_FLOOR_IN - 0.01,
                  max(x, cx), max(y, cy), Z_PCB_BACK)
        shell = shell.union(boss).union(web)

    # keyhole pads (before vents/holes are cut)
    if KEYHOLES_ENABLE:
        for x in KEYHOLE_XS:
            shell = shell.union(keyhole_pad(x))

    # zip-tie bar for the P1 cable (two legs + bridge)
    leg_h = TIEBAR_TUNNEL_H
    y0, y1 = P1_YC - TIEBAR_LEN / 2, P1_YC + TIEBAR_LEN / 2
    x0, x1 = TIEBAR_X0, TIEBAR_X0 + TIEBAR_W
    tie = box(x0, y0, Z_FLOOR_IN - 0.01, x1, y1,
              Z_FLOOR_IN + leg_h + TIEBAR_TOP_T)
    tie = tie.cut(box(x0 - 1, y0 + 2.0, Z_FLOOR_IN - 1, x1 + 1, y1 - 2.0,
                      Z_FLOOR_IN + leg_h))
    shell = shell.union(tie)

    # screw holes + counterbores from the outside
    for x, y in HOLES:
        shell = shell.cut(cyl(x, y, SCREW_CLEAR_D, Z_FLOOR_OUT - 1, Z_PCB_BACK + 1))
        shell = shell.cut(cyl(x, y, SCREW_HEAD_D, Z_FLOOR_OUT - 1,
                              Z_FLOOR_OUT + SCREW_HEAD_DEPTH))

    # button access holes
    for x, y in BUTTONS.values():
        shell = shell.cut(cyl(x, y, BTN_HOLE_D, Z_FLOOR_OUT - 1, Z_FLOOR_IN + 1))

    # ventilation slots
    for vx0, vx1, vy0, vy1 in VENT_FIELDS:
        n = int((vx1 - vx0) // VENT_PITCH) + 1
        for i in range(n):
            x = vx0 + i * VENT_PITCH
            if KEYHOLES_ENABLE and any(abs(x - kx) < 9 for kx in KEYHOLE_XS) \
                    and vy1 > KEYHOLE_Y - 8:
                continue
            shell = shell.cut(slot_y(x, vy0 + VENT_W / 2, vy1 - VENT_W / 2,
                                     VENT_W, Z_FLOOR_OUT - 1, Z_FLOOR_IN + 1))

    if KEYHOLES_ENABLE:
        for x in KEYHOLE_XS:
            for c in keyhole_cuts(x):
                shell = shell.cut(c)

    for c in side_cutouts():
        shell = shell.cut(c)
    return shell


# =============================================================================
# CHECKS
# =============================================================================
def vol(wp):
    return wp.val().Volume() if hasattr(wp, "val") else wp.Volume()


def inter_vol(a, b):
    try:
        return vol(a.intersect(b))
    except Exception as e:          # never report a failed boolean as "0"
        print("    boolean failed:", e)
        return float("inf")


def probes():
    """Bodies that must pass freely through the walls (plug/card/finger)."""
    zb = CONN_Z0
    far = CLR + WALL + 6.0
    p = {}
    usb_zc = zb - USB_H / 2
    p["USB-C plug body"] = xbox_y(USB_YC, USB_PLUG_W, -far, -USB_OVERHANG,
                                  usb_zc - USB_PLUG_H / 2, usb_zc + USB_PLUG_H / 2)
    p1_zc = zb - P1_H / 2
    p["P1 JST plug"] = xbox_y(P1_YC, P1_PLUG_W, -far, 0.0,
                              p1_zc - P1_PLUG_H / 2, p1_zc + P1_PLUG_H / 2)
    sd_zc = zb - SD_H / 2
    p["micro-SD card"] = ybox_x(SD_XC, SD_CARD_W, PCB_H + SD_OVERHANG,
                                PCB_H + far, sd_zc - SD_CARD_T / 2,
                                sd_zc + SD_CARD_T / 2)
    for name, (x, y) in BUTTONS.items():
        p[name + " button access (3 mm pin)"] = cyl(x, y, 3.0, Z_FLOOR_OUT - 5,
                                                    zb - BACK_MAX_H)
    for name in EXPOSE_TOP_CONNECTORS:
        xc = TOP_CONNECTORS[name]
        p[name] = ybox_x(xc, TOP_CONN_W - 1, PCB_H, PCB_H + far,
                         zb - TOP_CONN_H + 0.5, zb - 0.5)
    return p


def bbox_str(shape):
    bb = shape.BoundingBox()
    return ("x[%.2f, %.2f] y[%.2f, %.2f] z[%.2f, %.2f]  size %.2f x %.2f x %.2f"
            % (bb.xmin, bb.xmax, bb.ymin, bb.ymax, bb.zmin, bb.zmax,
               bb.xlen, bb.ylen, bb.zlen))


def run_checks(bezel, shell, board):
    ok = True
    print("\n=== Re-import of exported STEP files ===")
    for fn in ("bezel.step", "back_shell.step", "assembly.step"):
        path = os.path.join(HERE, fn)
        imp = cq.importers.importStep(path)
        solids = imp.solids().vals()
        v = sum(s.Volume() for s in solids)
        print("%-16s solids=%2d  volume=%10.1f mm^3  %s"
              % (fn, len(solids), v, bbox_str(imp.val() if len(solids) == 1
                                               else cq.Compound.makeCompound(solids))))

    print("\n=== Interference: board placeholder vs shells (should be ~0) ===")
    for pname, part in (("bezel", bezel), ("back_shell", shell)):
        for bname, b in board.items():
            v = inter_vol(part, b)
            flag = "OK" if v < 1e-3 else "COLLISION"
            if v >= 1e-3:
                ok = False
            if v >= 1e-3 or bname in ("pcb", "glass", "lcd", "usb_c", "p1_uart",
                                      "sd_socket"):
                print("  %-11s x %-22s %10.4f mm^3  %s" % (pname, bname, v, flag))
    v = inter_vol(bezel, shell)
    print("  bezel       x back_shell             %10.4f mm^3  %s"
          % (v, "OK" if v < 1e-3 else "COLLISION"))
    ok &= v < 1e-3

    print("\n=== Cut-out alignment: mating bodies through the walls (should be ~0) ===")
    for name, pr in probes().items():
        v1, v2 = inter_vol(pr, bezel), inter_vol(pr, shell)
        good = v1 < 1e-3 and v2 < 1e-3
        ok &= good
        print("  %-34s bezel %8.4f  shell %8.4f  %s"
              % (name, v1, v2, "OK" if good else "BLOCKED"))

    print("\n=== Window vs active area ===")
    wx0, wx1 = GLASS_X0 + LIP, GLASS_X0 + GLASS_W - LIP
    wy0, wy1 = GLASS_Y0 + LIP, GLASS_Y0 + GLASS_H - LIP
    print("  window x[%.2f, %.2f] y[%.2f, %.2f]  (%.2f x %.2f)"
          % (wx0, wx1, wy0, wy1, wx1 - wx0, wy1 - wy0))
    # AA edges in board coords, worst case of either drawing orientation
    mx = min(LCD_X0 + AA_MIN_MARGIN_X - wx0,
             wx1 - (LCD_X0 + LCD_W - AA_MIN_MARGIN_X))
    my = min(LCD_Y0 + AA_MIN_MARGIN_Y - wy0,
             wy1 - (LCD_Y0 + LCD_H - AA_MIN_MARGIN_Y))
    print("  worst-case AA margin inside window: x %.2f mm, y %.2f mm  %s"
          % (mx, my, "OK" if mx > 0 and my > 0 else "LIP COVERS ACTIVE AREA"))
    ok &= mx > 0 and my > 0

    screw_len = (0 - (Z_FLOOR_OUT + SCREW_HEAD_DEPTH)) + 4.0
    print("\n=== Fasteners ===")
    print("  head seat z=%.2f, insert z=0..%.1f -> needs M3 x %.1f (use M3x12, tip at z=%.1f)"
          % (Z_FLOOR_OUT + SCREW_HEAD_DEPTH, INSERT_HOLE_DEPTH, screw_len,
             Z_FLOOR_OUT + SCREW_HEAD_DEPTH + 12))
    print("\n=== Overall enclosure size ===")
    print("  %.1f x %.1f x %.1f mm (z %.2f .. %.2f)"
          % (PCB_W + 2 * (CLR + WALL), PCB_H + 2 * (CLR + WALL),
             Z_BEZEL_OUT - Z_FLOOR_OUT, Z_FLOOR_OUT, Z_BEZEL_OUT))
    print("\nALL CHECKS PASSED" if ok else "\nSOME CHECKS FAILED")
    return ok


# =============================================================================
# PREVIEW RENDERING (matplotlib, no display needed)
# =============================================================================
def render_view(shape, path, direction, width=1100, height=750):
    """Hidden-line projection via CadQuery SVG export, rasterised with cairosvg."""
    import cairosvg
    shp = shape.val() if hasattr(shape, "val") else shape
    svg = cq.occ_impl.exporters.svg.getSVG(shp, {
        "width": width, "height": height, "marginLeft": 30, "marginTop": 30,
        "projectionDir": direction, "showAxes": False, "showHidden": False,
        "strokeWidth": -1.0, "strokeColor": (40, 40, 40)})
    cairosvg.svg2png(bytestring=svg.encode(), write_to=path,
                     background_color="white")


def render_section(named, path, axis, at, title):
    """2D cross-section of several bodies (thin slab intersect + tessellate).
    axis 'y': section plane Y=at, plotted in (X, Z); axis 'x': plane X=at, (Y, Z)."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.collections import PolyCollection
    big = 1000.0
    t = 0.02
    if axis == "y":
        slab = box(-big, at - t, -big, big, at + t, big)
        i0, i1, lab = 0, 2, ("X [mm]", "Z [mm]")
    else:
        slab = box(at - t, -big, -big, at + t, big, big)
        i0, i1, lab = 1, 2, ("Y [mm]", "Z [mm]")
    fig, ax = plt.subplots(figsize=(13, 4.2), dpi=110)
    handles = []
    for name, shape, color in named:
        try:
            sec = shape.intersect(slab)
            solids = sec.solids().vals()
        except Exception:
            continue
        polys = []
        for so in solids:
            verts, tris = so.tessellate(0.05, 0.2)
            pts = [(v.toTuple()[i0], v.toTuple()[i1]) for v in verts]
            polys += [[pts[a], pts[b], pts[c]] for a, b, c in tris]
        if polys:
            ax.add_collection(PolyCollection(polys, facecolors=color,
                                             edgecolors=color, linewidths=0.3))
            handles.append(plt.Rectangle((0, 0), 1, 1, color=color, label=name))
    ax.autoscale()
    ax.set_aspect("equal")
    ax.set_xlabel(lab[0])
    ax.set_ylabel(lab[1])
    ax.set_title(title)
    ax.legend(handles=handles, loc="upper left", bbox_to_anchor=(1.0, 1.0),
              fontsize=8)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(path)
    plt.close(fig)


def render_all(bezel, shell, board):
    out = lambda n: os.path.join(HERE, n)
    shells = [bezel.val(), shell.val()]
    front = cq.Compound.makeCompound(
        shells + [board[k].val() for k in ("glass", "pcb")])
    both = cq.Compound.makeCompound(shells)
    render_view(front, out("preview_assembly_front.png"), (-1.75, 1.1, 5))
    render_view(both, out("preview_assembly_back.png"), (1.75, 1.1, -5))
    render_view(both, out("preview_assembly_back_left.png"), (-1, 1, -1))
    render_view(bezel, out("preview_bezel_inside.png"), (1.75, 1.1, -5))
    render_view(shell, out("preview_back_shell.png"), (-1.75, 1.1, 5))
    grey = {"bezel": "#2f3640", "back_shell": "#7f8c9a"}
    named = [("bezel", bezel, grey["bezel"]), ("back_shell", shell, grey["back_shell"])]
    pal = {"pcb": "#d9a520", "lcd": "#9aa5b1", "glass": "#5dade2",
           "usb_c": "#c0392b", "p1_uart": "#27ae60", "sd_socket": "#8e44ad",
           "smd_envelope": "#f5cba7", "esp32s3": "#16a085"}
    for n, b in board.items():
        named.append(("board: " + n, b, pal.get(n, "#e67e22")))
    render_section(named, out("preview_section_usb.png"), "y", USB_YC,
                   "Section Y=%.1f (through USB-C)" % USB_YC)
    render_section(named, out("preview_section_p1.png"), "y", P1_YC,
                   "Section Y=%.1f (through P1 UART connector)" % P1_YC)
    render_section(named, out("preview_section_sd.png"), "x", SD_XC,
                   "Section X=%.1f (through micro-SD slot)" % SD_XC)


# =============================================================================
# MAIN
# =============================================================================
def main():
    do_png = "--no-png" not in sys.argv
    print("Building board placeholder ...")
    board = build_board()
    print("Building bezel ...")
    bezel = build_bezel()
    print("Building back shell ...")
    shell = build_shell()

    # ---- STEP (assembly coordinates) ----
    cq.exporters.export(bezel, os.path.join(HERE, "bezel.step"))
    cq.exporters.export(shell, os.path.join(HERE, "back_shell.step"))
    assy = cq.Assembly(name="ESP32-8048S070C_enclosure")
    assy.add(shell, name="back_shell", color=cq.Color(0.25, 0.25, 0.28))
    assy.add(bezel, name="bezel", color=cq.Color(0.15, 0.15, 0.15))
    colors = {"pcb": (0.85, 0.65, 0.1), "lcd": (0.7, 0.7, 0.7),
              "glass": (0.1, 0.1, 0.2)}
    for name, b in board.items():
        c = colors.get(name, (0.9, 0.9, 0.9))
        assy.add(b, name="board_" + name, color=cq.Color(*c))
    assy.save(os.path.join(HERE, "assembly.step"), exportType="STEP")

    # ---- STL in print orientation ----
    bezel_print = bezel.rotate((0, 0, 0), (1, 0, 0), 180).translate(
        (0, 0, Z_BEZEL_OUT))           # front face down on the bed
    shell_print = shell.translate((0, 0, -Z_FLOOR_OUT))   # floor on the bed
    cq.exporters.export(bezel_print, os.path.join(HERE, "bezel.stl"),
                        tolerance=0.02, angularTolerance=0.1)
    cq.exporters.export(shell_print, os.path.join(HERE, "back_shell.stl"),
                        tolerance=0.02, angularTolerance=0.1)

    ok = run_checks(bezel, shell, board)

    if do_png:
        print("\nRendering previews ...")
        render_all(bezel, shell, board)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
