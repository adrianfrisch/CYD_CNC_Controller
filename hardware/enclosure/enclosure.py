#!/usr/bin/env python3
"""
Parametric two-part enclosure for the ESP32 display boards supported by the
CYD CNC Controller firmware.  Board dimensions live in boards/<board>.py
(each value with its source or "EST"); this file holds the design and the
builder shared by all boards.

Boards (output folder = PlatformIO environment name)
  esp32-8048S070C   7"   800x480, GT911 capacitive touch
  esp32-4827S043R   4.3" 480x272, XPT2046 resistive touch
  esp32-2432S028R   2.8" 320x240 "Cheap Yellow Display", resistive touch

Parts
  * front bezel  - frames the display, lip over the glass edge,
                   M3 heat-set inserts in four corner bosses
  * back shell   - houses the board on four corner standoffs, connector
                   cut-outs, vents, button holes, two keyhole slots, and the
                   GRBL cable channel + clamp tongue
  * cable clamp  - screw-down bar that pinches the CAT5 cable on the tongue

The same four M3 screws (inserted from the back) pass through the shell
standoffs, through the PCB mounting holes, into the inserts in the bezel:
they clamp the PCB between the standoffs and the bezel bosses and hold the
two halves together.

Run (CadQuery 2.x):
    python enclosure.py                    # all boards
    python enclosure.py esp32-2432S028R    # one board
    python enclosure.py --no-png ...       # skip the matplotlib previews

COORDINATE SYSTEM (all values in mm), per board
  Seen from the FRONT (looking at the screen, landscape as the firmware
  draws it):
    X = 0 .. PCB_W   PCB left edge -> right edge
    Y = 0 .. PCB_H   PCB bottom edge -> top edge
    Z = 0            PCB front face (the face the LCD is stuck to)
    +Z               towards the viewer;  PCB back face is at Z = -PCB_T
  Edge connectors are given per edge ("left", "right", "top", "bottom") with
  their centre position along that edge.

CABLE ROUTES (CABLE in the board file)
  "side"     the connector faces the left/right wall; the cable turns into a
             groove along the outside of that wall and runs towards the
             top/bottom edge ("run"), where a tongue past the corner holds
             the clamp.
  "straight" the connector faces the top/bottom wall; the cable goes straight
             out into a wide pocket (takes the pigtail from any connector
             behind the opening) and then through the clamp on a tongue.
"""

import importlib
import math
import os
import sys

import cadquery as cq

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = HERE              # set per board in main()

BOARDS = {
    "esp32-8048S070C": "esp32_8048s070c",
    "esp32-4827S043R": "esp32_4827s043r",
    "esp32-2432S028R": "esp32_2432s028r",
}

# =============================================================================
# ENCLOSURE PARAMETERS (design defaults -- a board file may override any)
# =============================================================================
WALL = 2.0              # side wall / floor / plate thickness
CLR = 0.3               # clearance PCB/glass outline -> inner wall
BEZEL_T = 2.0           # front plate thickness
GLASS_GAP = 0.2         # air gap between glass and bezel lip (no pressure)
LIP = 2.0               # bezel overlap onto the glass edge
WINDOW_R = 1.0          # window corner radius
BACK_CLEAR = 7.0        # PCB back face -> inner floor (tallest part + margin
                        # + room for the keyhole pockets)
SEAM_LIP_H = 1.5        # shell alignment lip height above the seam (Z=0)
SEAM_LIP_T = 1.0        # shell alignment lip thickness
SEAM_LIP_CLR = 0.15     # clearance of the matching rabbet in the bezel

# Fasteners: M3 heat-set inserts in the bezel, screws from the back.
INSERT_HOLE_D = 4.0     # M3 x 4 mm short heat-set insert (OD ~4.6) -> 4.0 hole
INSERT_HOLE_DEPTH = 6.0
BEZEL_BOSS_D = 7.0      # must stay clear of the glass corner
SHELL_BOSS_D = 7.0
SCREW_CLEAR_D = 3.4     # M3 clearance
SCREW_HEAD_D = 6.2      # counterbore for M3 ISO 4762 / 7380 head (5.5-5.7)
SCREW_HEAD_DEPTH = 3.0

BTN_HOLE_D = 4.0

# Ventilation (slots through the back)
VENT_W = 2.0
VENT_PITCH = 5.0
VENT_FIELDS = []

# Keyhole slots for wall/machine mounting (screw head <= 7.5 mm, shank <= 3.8)
KEYHOLES_ENABLE = True
KEYHOLE_XS = ()
KEYHOLE_Y = 0.0         # centre of the head-entry circle
KEYHOLE_HEAD_D = 8.0
KEYHOLE_SLOT_W = 4.0
KEYHOLE_TRAVEL = 10.0   # slot length (screw hangs at Y = KEYHOLE_Y + travel)
KEYHOLE_POCKET_H = 2.5  # room for the screw head inside the pad
KEYHOLE_ROOF = 1.5

# Optional zip-tie bar on the inside floor (board: TIEBAR = dict(x0, yc))
TIEBAR = None
TIEBAR_W = 3.0
TIEBAR_LEN = 13.0
TIEBAR_TUNNEL_H = 2.0
TIEBAR_TOP_T = 1.5

# GRBL cable channel + exit clamp (standard CAT5/5e cable to the CNC
# controller).  The CAT5 conductors are spliced to a JST 1.25 pigtail inside
# the channel (24 AWG solid does not fit 1.25 mm crimps); the clamp takes all
# pull off the connector.
CHANNEL_ENABLE = True
CABLE = None            # board: dict(port=, route="side"|"straight", run=)
CABLE_D = 6.0           # EST: CAT5/5e UTP jacket 5.0-6.5 mm -- MEASURE yours
CH_CLR = 0.5            # groove width = CABLE_D + CH_CLR
CH_FLOOR_T = 2.0        # groove floor (flush with the case back)
CH_OUTER_WALL = 2.0
CH_LIP_T = 1.0          # snap lips along the groove opening (z thickness)
CH_LIP_OVERLAP = 0.4    # per side -> opening CABLE_D + CH_CLR - 0.8
CH_EXIT_R = 2.0         # rounded floor edge where the cable leaves the clamp
CH_FUNNEL_L = 8.0       # "straight" route: pocket length before the groove
CLAMP_TONGUE_L = 16.0   # "side" route: tongue length beyond the case edge
CLAMP_W = 10.0          # clamp bar width along the cable
CLAMP_T = 3.0           # clamp bar thickness above the tongue
CLAMP_SQUEEZE = 0.8     # jacket compression when the clamp is screwed down
CLAMP_BOSS_R = 3.5      # material around the inserts
CLAMP_TOOTH_PITCH = 3.0 # grip teeth across the ridge underside
CLAMP_TOOTH_DEPTH = 0.6

# Board data defaults (optional features)
ANT_NOTCH = None
TOUCH_FPC = None
OBSTACLES = {}
LCD_TAPE = 0.0

DEFAULTS = {k: v for k, v in dict(globals()).items()
            if k.isupper() and k not in ("HERE", "OUT", "BOARDS")}


def load_board(env):
    """Reset the design defaults, then apply boards/<module>.py."""
    globals().update(DEFAULTS)
    sys.path.insert(0, HERE)
    try:
        mod = importlib.import_module("boards." + BOARDS[env])
        mod = importlib.reload(mod)
    finally:
        sys.path.pop(0)
    globals().update({k: v for k, v in vars(mod).items() if k.isupper()})
    derive()


def derive():
    g = globals()
    g["HOLES"] = [(HOLE_INSET, HOLE_INSET), (PCB_W - HOLE_INSET, HOLE_INSET),
                  (HOLE_INSET, PCB_H - HOLE_INSET),
                  (PCB_W - HOLE_INSET, PCB_H - HOLE_INSET)]
    g["Z_LCD0"] = LCD_TAPE
    g["Z_GLASS0"] = g["Z_LCD0"] + LCD_T
    g["Z_GLASS_TOP"] = g["Z_GLASS0"] + GLASS_T
    g["Z_PCB_BACK"] = -PCB_T
    g["Z_FLOOR_IN"] = g["Z_PCB_BACK"] - BACK_CLEAR
    g["Z_FLOOR_OUT"] = g["Z_FLOOR_IN"] - WALL
    g["Z_BEZEL_IN"] = g["Z_GLASS_TOP"] + GLASS_GAP
    g["Z_BEZEL_OUT"] = g["Z_BEZEL_IN"] + BEZEL_T
    g["Z_SEAM"] = 0.0
    g["CONN_Z0"] = g["Z_PCB_BACK"]   # top-mount parts hang from the PCB back
    g["CLAMP_SCREW_DX"] = (CABLE_D + CH_CLR) / 2 + 3.5   # from cable axis


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


def edge_xy(edge, n, t):
    """Point at outward distance n from a PCB edge, position t along it."""
    return {"left": (-n, t), "right": (PCB_W + n, t),
            "bottom": (t, -n), "top": (t, PCB_H + n)}[edge]


def edge_box(edge, n0, n1, t0, t1, z0, z1):
    """Box spanning n0..n1 outward from an edge and t0..t1 along it."""
    xa, ya = edge_xy(edge, n0, t0)
    xb, yb = edge_xy(edge, n1, t1)
    return box(min(xa, xb), min(ya, yb), z0, max(xa, xb), max(ya, yb), z1)


def normal_axis(edge):
    return "|X" if edge in ("left", "right") else "|Y"


def ybox(x0, x1, ya, yb, z0, z1):
    return box(x0, min(ya, yb), z0, x1, max(ya, yb), z1)


def union_all(shapes):
    it = iter(shapes)
    acc = next(it)
    for s in it:
        acc = acc.union(s)
    return acc


# =============================================================================
# BOARD PLACEHOLDER (fit-check body)
# =============================================================================
def build_board():
    parts = {}
    pcb = outline(0.0, Z_PCB_BACK, 0.0)
    if ANT_NOTCH:
        a = ANT_NOTCH
        pcb = pcb.cut(edge_box(a["edge"], -a["depth"], 1, a["t0"], a["t1"],
                               Z_PCB_BACK - 1, 1))
    if TOUCH_FPC:
        f = TOUCH_FPC
        pcb = pcb.cut(edge_box(f["edge"], -f["notch"], 1, f["t0"], f["t1"],
                               Z_PCB_BACK - 1, 1))
    for x, y in HOLES:
        pcb = pcb.cut(cyl(x, y, HOLE_D, Z_PCB_BACK - 1, 1))
    parts["pcb"] = pcb

    parts["lcd"] = box(LCD_X0, LCD_Y0, Z_LCD0, LCD_X0 + LCD_W, LCD_Y0 + LCD_H,
                       Z_LCD0 + LCD_T)
    parts["glass"] = box(GLASS_X0, GLASS_Y0, Z_GLASS0, GLASS_X0 + GLASS_W,
                         GLASS_Y0 + GLASS_H, Z_GLASS_TOP)

    zb = CONN_Z0
    for name, p in PORTS.items():
        ov = p.get("overhang", 0.0)
        parts[name] = edge_box(p["edge"], ov - p["d"], ov,
                               p["pos"] - p["w"] / 2, p["pos"] + p["w"] / 2,
                               zb - p["h"], zb)
    s = SD
    parts["sd_socket"] = edge_box(s["edge"], s["overhang"] - s["d"],
                                  s["overhang"], s["pos"] - s["w"] / 2,
                                  s["pos"] + s["w"] / 2, zb - s["h"], zb)
    for name, (x, y) in BUTTONS.items():
        parts["sw_" + name] = box(x - BTN_L / 2, y - BTN_W / 2, zb - BACK_MAX_H,
                                  x + BTN_L / 2, y + BTN_W / 2, zb)
    parts["esp32"] = box(ESP["x0"], ESP["y0"], zb - ESP["t"], ESP["x1"],
                         ESP["y1"], zb)
    for name, (x0, y0, x1, y1, h) in OBSTACLES.items():
        parts[name] = box(x0, y0, zb - h, x1, y1, zb)

    # generic envelope for all small SMD parts, kept out of the screw bosses
    # (keep-out circle also covers the envelope corner when holes sit further in)
    env = box(1.0, 1.0, zb - SMD_ENVELOPE_H, PCB_W - 1.0, PCB_H - 1.0, zb)
    keep_d = max(SHELL_BOSS_D + 1.0, 2 * math.sqrt(2) * (HOLE_INSET - 1.0) + 0.5)
    for x, y in HOLES:
        env = env.cut(cyl(x, y, keep_d, zb - 10, zb + 1))
    if ANT_NOTCH:
        a = ANT_NOTCH
        env = env.cut(edge_box(a["edge"], -a["depth"], 1, a["t0"], a["t1"],
                               zb - 10, zb + 1))
    parts["smd_envelope"] = env
    return parts


# =============================================================================
# CUT-OUTS shared by both halves (anything crossing the seam cuts both)
# =============================================================================
def side_cutouts():
    cuts = []
    out = CLR + WALL + 2.0
    for p in PORTS.values():
        if not p.get("cut"):
            continue
        zc = CONN_Z0 - p["h"] / 2
        c = edge_box(p["edge"], -0.5, out, p["pos"] - p["cut_w"] / 2,
                     p["pos"] + p["cut_w"] / 2, zc - p["cut_h"] / 2,
                     zc + p["cut_h"] / 2)
        if p.get("fillet"):
            c = c.edges(normal_axis(p["edge"])).fillet(p["fillet"])
        cuts.append(c)
    # micro-SD slot + finger scallop on the outside
    s = SD
    sd_zc = CONN_Z0 - s["h"] / 2
    cuts.append(edge_box(s["edge"], -0.5, out, s["pos"] - s["cut_w"] / 2,
                         s["pos"] + s["cut_w"] / 2, sd_zc - s["cut_h"] / 2,
                         sd_zc + s["cut_h"] / 2))
    fx, fy = edge_xy(s["edge"], CLR + WALL + s["finger_r"] - 1.2, s["pos"])
    cuts.append(cyl(fx, fy, 2 * s["finger_r"], sd_zc - 3.0,
                    min(sd_zc + 3.0, Z_SEAM - 0.01)))
    # touch FPC wraps around an edge: relief in the inner wall face
    if TOUCH_FPC:
        f = TOUCH_FPC
        cuts.append(edge_box(f["edge"], -0.5, CLR + f["relief"], f["t0"] - 1.0,
                             f["t1"] + 1.0, Z_PCB_BACK - 1.5, Z_GLASS_TOP))
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

    # optional zip-tie bar (two legs + bridge)
    if TIEBAR:
        leg_h = TIEBAR_TUNNEL_H
        y0, y1 = TIEBAR["yc"] - TIEBAR_LEN / 2, TIEBAR["yc"] + TIEBAR_LEN / 2
        x0, x1 = TIEBAR["x0"], TIEBAR["x0"] + TIEBAR_W
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

    if CHANNEL_ENABLE and CABLE:
        shell = add_channel(shell)

    for c in side_cutouts():
        shell = shell.cut(c)
    return shell


# =============================================================================
# GRBL CABLE CHANNEL + CLAMP
# Built in a canonical frame, then mirrored into place:
#   "side"     canonical = exit through the LEFT wall, run towards +Y (top)
#   "straight" canonical = exit through the BOTTOM wall, run towards -Y
# =============================================================================
def cable_port():
    return PORTS[CABLE["port"]]


def channel_frame():
    """(route, mirror_x, mirror_y, canonical port position)."""
    p = cable_port()
    if CABLE["route"] == "side":
        assert p["edge"] in ("left", "right"), "side route needs a left/right port"
        mx = p["edge"] == "right"
        my = CABLE["run"] == "bottom"
        return "side", mx, my, (PCB_H - p["pos"]) if my else p["pos"]
    assert p["edge"] in ("top", "bottom"), "straight route needs a top/bottom port"
    return "straight", False, p["edge"] == "top", p["pos"]


def to_actual(wp):
    _, mx, my, _ = channel_frame()
    if mx:
        wp = wp.mirror("YZ", (PCB_W / 2, 0, 0))
    if my:
        wp = wp.mirror("XZ", (0, PCB_H / 2, 0))
    return wp


def act_x(x):
    return PCB_W - x if channel_frame()[1] else x


def act_y(y):
    return PCB_H - y if channel_frame()[2] else y


def channel_geom():
    """All channel/clamp coordinates in the canonical frame."""
    route, _, _, pc = channel_frame()
    p = cable_port()
    g = {"route": route}
    g["ch_w"] = CABLE_D + CH_CLR
    g["zg0"] = Z_FLOOR_OUT + CH_FLOOR_T          # groove floor
    g["zg1"] = g["zg0"] + g["ch_w"] + CH_LIP_T   # duct / tongue top
    g["cable_top"] = g["zg0"] + CABLE_D          # cable resting on the floor
    yw = -(CLR + WALL)                           # outer wall face (bottom/left)
    if route == "side":
        d = +1
        g["gx1"] = yw                            # groove inner face = wall face
        g["gx0"] = g["gx1"] - g["ch_w"]
        g["dx0"] = g["gx0"] - CH_OUTER_WALL      # duct outer face
        g["xc"] = (g["gx0"] + g["gx1"]) / 2
        # y: a = behind the port, b = case edge, c = tongue end
        g["y_p1"] = pc - d * p["cut_w"] / 2      # groove starts at the window
        g["ya"] = g["y_p1"] - d * CH_OUTER_WALL
        g["yb"] = PCB_H + CLR + WALL
        g["yc"] = g["yb"] + d * CLAMP_TONGUE_L
        g["y_clamp"] = g["yb"] + d * (CLAMP_TONGUE_L / 2 + 0.5)
        g["tongue_y0"] = g["yb"] - d * 1.0
        g["tongue_len"] = CLAMP_TONGUE_L
    else:
        d = -1
        g["xc"] = pc
        g["fw"] = max(p["cut_w"], g["ch_w"])     # pocket width
        g["yw"] = yw
        g["y_f1"] = yw - CH_FUNNEL_L             # pocket end
        g["y_clamp"] = g["y_f1"] - 1.0 - CLAMP_W / 2
        g["yc"] = g["y_clamp"] - CLAMP_W / 2 - 3.0
        g["tongue_y0"] = yw + 1.0
        g["tongue_len"] = yw - g["yc"]
    g["d"] = d
    g["tx0"] = g["xc"] - CLAMP_SCREW_DX - CLAMP_BOSS_R
    g["tx1"] = g["xc"] + CLAMP_SCREW_DX + CLAMP_BOSS_R
    if route == "straight":
        g["tx0"] = min(g["tx0"], pc - g["fw"] / 2 - CH_OUTER_WALL)
        g["tx1"] = max(g["tx1"], pc + g["fw"] / 2 + CH_OUTER_WALL)
    assert g["zg1"] < Z_SEAM, "channel taller than the back shell"
    zc = CONN_Z0 - p["h"] / 2
    assert zc - p["cut_h"] / 2 < g["zg1"] and zc + p["cut_h"] / 2 > g["zg0"], \
        "cable port opening does not reach the groove"
    return g


def exit_round(g, xa, xb):
    """Rounded floor edge at the groove exit (bend protection)."""
    r, d = CH_EXIT_R, g["d"]
    corner = ybox(xa, xb, g["yc"] - d * r, g["yc"] + d * 0.01,
                  g["zg0"] - r, g["zg0"] + 0.01)
    keep = (cq.Workplane("YZ").circle(r).extrude(xb - xa + 0.02)
            .translate((xa - 0.01, g["yc"] - d * r, g["zg0"] - r)))
    return corner.cut(keep)


def channel_ops():
    """[(op, solid)] in the canonical frame; op = 'union' | 'cut'."""
    g = channel_geom()
    d = g["d"]
    ops = []
    tongue = ybox(g["tx0"], g["tx1"], g["tongue_y0"], g["yc"], Z_FLOOR_OUT,
                  g["zg1"]).edges("|Z").fillet(2.0)
    if g["route"] == "side":
        # duct along the wall (overlaps the wall by 0.5 for a solid union)
        ops.append(("union", ybox(g["dx0"], g["gx1"] + 0.5, g["ya"], g["yb"],
                                  Z_FLOOR_OUT, g["zg1"])))
        ops.append(("union", tongue))
        # groove (open to the front), through the tongue end
        ops.append(("cut", ybox(g["gx0"], g["gx1"], g["y_p1"],
                                g["yc"] + d * 1.0, g["zg0"], g["zg1"] + 1.0)))
        # snap lips along the duct part only (the clamp ridge needs a clear groove)
        lz0 = g["zg1"] - CH_LIP_T
        for lx0, lx1 in ((g["gx0"] - 0.01, g["gx0"] + CH_LIP_OVERLAP),
                         (g["gx1"] - CH_LIP_OVERLAP, g["gx1"] + 0.01)):
            ops.append(("union", ybox(lx0, lx1, g["y_p1"], g["yb"], lz0,
                                      g["zg1"])))
        ops.append(("cut", exit_round(g, g["gx0"], g["gx1"])))
    else:
        ops.append(("union", tongue))
        half = g["ch_w"] / 2
        # pocket behind the opening (pigtail from any connector in it)
        ops.append(("cut", ybox(g["xc"] - g["fw"] / 2, g["xc"] + g["fw"] / 2,
                                g["yw"], g["y_f1"], g["zg0"], g["zg1"] + 1.0)))
        # groove through the clamp to the tongue end
        ops.append(("cut", ybox(g["xc"] - half, g["xc"] + half, g["y_f1"] + 0.01,
                                g["yc"] + d * 1.0, g["zg0"], g["zg1"] + 1.0)))
        ops.append(("cut", exit_round(g, g["xc"] - half, g["xc"] + half)))
    # heat-set inserts for the clamp screws (from the front face)
    for sx in (-1, 1):
        ops.append(("cut", cyl(g["xc"] + sx * CLAMP_SCREW_DX, g["y_clamp"],
                               INSERT_HOLE_D, g["zg1"] - INSERT_HOLE_DEPTH,
                               g["zg1"] + 1)))
    return ops


def add_channel(shell):
    for op, solid in channel_ops():
        solid = to_actual(solid)
        shell = shell.union(solid) if op == "union" else shell.cut(solid)
    return shell


def build_clamp():
    """Screw-down clamp bar (assembly position = tightened on the cable)."""
    g = channel_geom()
    yc0, yc1 = g["y_clamp"] - CLAMP_W / 2, g["y_clamp"] + CLAMP_W / 2
    bar = rrect(g["tx0"], yc0, g["tx1"], yc1, 2.0, g["zg1"], g["zg1"] + CLAMP_T)
    # ridge that reaches into the groove and squeezes the jacket
    half = g["ch_w"] / 2 - 0.3
    rz0 = g["cable_top"] - CLAMP_SQUEEZE
    ridge = box(g["xc"] - half, yc0, rz0, g["xc"] + half, yc1, g["zg1"] + 0.01)
    # transverse grip teeth: shallow grooves across the ridge underside
    n = int(CLAMP_W // CLAMP_TOOTH_PITCH)
    y = yc0 + (CLAMP_W - (n - 1) * CLAMP_TOOTH_PITCH) / 2
    for i in range(n - 1):
        yy = y + (i + 0.5) * CLAMP_TOOTH_PITCH
        ridge = ridge.cut(box(g["xc"] - half - 1, yy - 0.6, rz0 - 1,
                              g["xc"] + half + 1, yy + 0.6, rz0 + CLAMP_TOOTH_DEPTH))
    clamp = bar.union(ridge)
    for sx in (-1, 1):
        clamp = clamp.cut(cyl(g["xc"] + sx * CLAMP_SCREW_DX, g["y_clamp"],
                              SCREW_CLEAR_D, rz0 - 1, g["zg1"] + CLAMP_T + 1))
    return to_actual(clamp)


def cable_probe():
    """CAT5 jacket lying on the groove floor, from the opening to past the end."""
    g = channel_geom()
    d = g["d"]
    y0 = g["y_p1"] + 0.5 if g["route"] == "side" else g["yw"] - 0.01
    y1 = g["yc"] + d * 8.0
    probe = (cq.Workplane("XZ").circle(CABLE_D / 2).extrude(-(abs(y1 - y0)))
             .translate((g["xc"], min(y0, y1), g["zg0"] + CABLE_D / 2)))
    return to_actual(probe)


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
    pr = {}
    for name, p in PORTS.items():
        if not p.get("cut"):
            continue
        ov = p.get("overhang", 0.0)
        zc = zb - p["h"] / 2
        t0, t1 = p["pos"] - p["plug_w"] / 2, p["pos"] + p["plug_w"] / 2
        z0, z1 = zc - p["plug_h"] / 2, zc + p["plug_h"] / 2
        label = p.get("label", name)
        pr[label] = edge_box(p["edge"], ov, far, t0, t1, z0, z1)
        if p.get("plug_len"):
            pr[label + " + %.0f mm overmould" % p["plug_len"]] = edge_box(
                p["edge"], ov, p["plug_len"], t0, t1, z0, z1)
    s = SD
    sd_zc = zb - s["h"] / 2
    pr["micro-SD card"] = edge_box(s["edge"], s["overhang"], far,
                                   s["pos"] - s["card_w"] / 2,
                                   s["pos"] + s["card_w"] / 2,
                                   sd_zc - s["card_t"] / 2,
                                   sd_zc + s["card_t"] / 2)
    for name, (x, y) in BUTTONS.items():
        pr[name + " button access (3 mm pin)"] = cyl(x, y, 3.0, Z_FLOOR_OUT - 5,
                                                     zb - BACK_MAX_H)
    return pr


def bbox_str(shape):
    bb = shape.BoundingBox()
    return ("x[%.2f, %.2f] y[%.2f, %.2f] z[%.2f, %.2f]  size %.2f x %.2f x %.2f"
            % (bb.xmin, bb.xmax, bb.ymin, bb.ymax, bb.zmin, bb.zmax,
               bb.xlen, bb.ylen, bb.zlen))


def run_checks(bezel, shell, board, clamp=None):
    ok = True
    print("\n=== Re-import of exported STEP files ===")
    files = ["bezel.step", "back_shell.step", "assembly.step"]
    if clamp is not None:
        files.insert(2, "clamp.step")
    for fn in files:
        path = os.path.join(OUT, fn)
        imp = cq.importers.importStep(path)
        solids = imp.solids().vals()
        v = sum(s.Volume() for s in solids)
        print("%-16s solids=%2d  volume=%10.1f mm^3  %s"
              % (fn, len(solids), v, bbox_str(imp.val() if len(solids) == 1
                                               else cq.Compound.makeCompound(solids))))

    print("\n=== Interference: board placeholder vs shells (should be ~0) ===")
    shown = {"pcb", "glass", "lcd", "sd_socket", "esp32"} | \
        {n for n, p in PORTS.items() if p.get("cut")}
    for pname, part in (("bezel", bezel), ("back_shell", shell)):
        for bname, b in board.items():
            v = inter_vol(part, b)
            flag = "OK" if v < 1e-3 else "COLLISION"
            if v >= 1e-3:
                ok = False
            if v >= 1e-3 or bname in shown:
                print("  %-11s x %-22s %10.4f mm^3  %s" % (pname, bname, v, flag))
    v = inter_vol(bezel, shell)
    print("  bezel       x back_shell             %10.4f mm^3  %s"
          % (v, "OK" if v < 1e-3 else "COLLISION"))
    ok &= v < 1e-3

    print("\n=== Cut-out alignment: mating bodies through the walls (should be ~0) ===")
    for name, prb in probes().items():
        v1, v2 = inter_vol(prb, bezel), inter_vol(prb, shell)
        good = v1 < 1e-3 and v2 < 1e-3
        ok &= good
        print("  %-38s bezel %8.4f  shell %8.4f  %s"
              % (name, v1, v2, "OK" if good else "BLOCKED"))

    if clamp is not None:
        print("\n=== GRBL cable channel + clamp (%s route) ===" % CABLE["route"])
        cab = cable_probe()
        for name, part in (("back_shell", shell), ("bezel", bezel)):
            v = inter_vol(cab, part)
            ok &= v < 1e-3
            print("  CAT5 cable (d=%.1f) x %-11s %8.4f mm^3  %s"
                  % (CABLE_D, name, v, "OK" if v < 1e-3 else "BLOCKED"))
        for name, part in [("back_shell", shell), ("bezel", bezel)] + list(board.items()):
            v = inter_vol(clamp, part)
            ok &= v < 1e-3
            if v >= 1e-3 or name in ("back_shell", "bezel"):
                print("  clamp x %-22s %8.4f mm^3  %s"
                      % (name, v, "OK" if v < 1e-3 else "COLLISION"))
        v = inter_vol(clamp, cab)
        ok &= v > 1.0
        print("  clamp squeezes cable: %.1f mm^3 overlap (%.1f mm)  %s"
              % (v, CLAMP_SQUEEZE, "OK" if v > 1.0 else "NO GRIP"))
        g = channel_geom()
        if g["route"] == "side":
            print("  channel protrudes %.1f mm beside the wall, tongue %.1f mm "
                  "past the edge" % (g["gx1"] - g["dx0"], g["tongue_len"]))
        else:
            print("  pocket %.1f mm wide, tongue %.1f mm past the edge"
                  % (g["fw"], g["tongue_len"]))
        print("  clamp screws M3 x %.0f" % (CLAMP_T + INSERT_HOLE_DEPTH - 1.0))

    print("\n=== Window vs active area ===")
    wx0, wx1 = GLASS_X0 + LIP, GLASS_X0 + GLASS_W - LIP
    wy0, wy1 = GLASS_Y0 + LIP, GLASS_Y0 + GLASS_H - LIP
    print("  window x[%.2f, %.2f] y[%.2f, %.2f]  (%.2f x %.2f)"
          % (wx0, wx1, wy0, wy1, wx1 - wx0, wy1 - wy0))
    # AA edges in board coords, worst case of either orientation
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
    print("\n=== Overall enclosure size (without channel/tongue) ===")
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




def port_section(named, path, edge, pos, title):
    if edge in ("left", "right"):
        render_section(named, path, "y", pos, "Section Y=%.1f (%s)" % (pos, title))
    else:
        render_section(named, path, "x", pos, "Section X=%.1f (%s)" % (pos, title))


def render_all(bezel, shell, board, clamp=None):
    out = lambda n: os.path.join(OUT, n)
    shells = [bezel.val(), shell.val()] + ([clamp.val()] if clamp is not None else [])
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
    if clamp is not None:
        named.append(("clamp", clamp, "#e74c3c"))
        named.append(("CAT5 cable", cable_probe(), "#3498db"))
    pal = {"pcb": "#d9a520", "lcd": "#9aa5b1", "glass": "#5dade2",
           "sd_socket": "#8e44ad", "smd_envelope": "#f5cba7", "esp32": "#16a085"}
    for n, b in board.items():
        named.append(("board: " + n, b, pal.get(n, "#e67e22")))
    for name, p in PORTS.items():
        if p.get("cut"):
            port_section(named, out("preview_section_%s.png" % name), p["edge"],
                         p["pos"], "through %s" % name)
    port_section(named, out("preview_section_sd.png"), SD["edge"], SD["pos"],
                 "through micro-SD slot")
    if clamp is not None:
        g = channel_geom()
        render_section(named, out("preview_section_clamp.png"), "y",
                       act_y(g["y_clamp"]),
                       "Section Y=%.1f (through the cable clamp)" % act_y(g["y_clamp"]))
        render_section(named, out("preview_section_channel.png"), "x",
                       act_x(g["xc"]),
                       "Section X=%.1f (along the cable channel)" % act_x(g["xc"]))
        sx = -1.2 if act_x(g["xc"]) < PCB_W / 2 else 1.2
        sy = 1.6 if act_y(g["y_clamp"]) > PCB_H / 2 else -1.6
        render_view(cq.Compound.makeCompound(shells), out("preview_channel.png"),
                    (sx, sy, 1.0))


# =============================================================================
# MAIN
# =============================================================================
def build_and_export(env, do_png):
    global OUT
    load_board(env)
    OUT = os.path.join(HERE, env)
    os.makedirs(OUT, exist_ok=True)
    print("\n" + "=" * 78 + "\n%s -> %s\n" % (TITLE, OUT) + "=" * 78)
    print("Building board placeholder ...")
    board = build_board()
    print("Building bezel ...")
    bezel = build_bezel()
    print("Building back shell ...")
    shell = build_shell()
    clamp = build_clamp() if CHANNEL_ENABLE and CABLE else None

    # ---- STEP (assembly coordinates) ----
    cq.exporters.export(bezel, os.path.join(OUT, "bezel.step"))
    cq.exporters.export(shell, os.path.join(OUT, "back_shell.step"))
    if clamp is not None:
        cq.exporters.export(clamp, os.path.join(OUT, "clamp.step"))
    assy = cq.Assembly(name=env + "_enclosure")
    assy.add(shell, name="back_shell", color=cq.Color(0.25, 0.25, 0.28))
    assy.add(bezel, name="bezel", color=cq.Color(0.15, 0.15, 0.15))
    if clamp is not None:
        assy.add(clamp, name="cable_clamp", color=cq.Color(0.8, 0.2, 0.2))
    colors = {"pcb": (0.85, 0.65, 0.1), "lcd": (0.7, 0.7, 0.7),
              "glass": (0.1, 0.1, 0.2)}
    for name, b in board.items():
        c = colors.get(name, (0.9, 0.9, 0.9))
        assy.add(b, name="board_" + name, color=cq.Color(*c))
    assy.save(os.path.join(OUT, "assembly.step"), exportType="STEP")

    # ---- STL in print orientation ----
    bezel_print = bezel.rotate((0, 0, 0), (1, 0, 0), 180).translate(
        (0, 0, Z_BEZEL_OUT))           # front face down on the bed
    shell_print = shell.translate((0, 0, -Z_FLOOR_OUT))   # floor on the bed
    cq.exporters.export(bezel_print, os.path.join(OUT, "bezel.stl"),
                        tolerance=0.02, angularTolerance=0.1)
    cq.exporters.export(shell_print, os.path.join(OUT, "back_shell.stl"),
                        tolerance=0.02, angularTolerance=0.1)
    if clamp is not None:              # flat top on the bed, ridge up
        top = channel_geom()["zg1"] + CLAMP_T
        clamp_print = clamp.rotate((0, 0, 0), (1, 0, 0), 180).translate((0, 0, top))
        cq.exporters.export(clamp_print, os.path.join(OUT, "clamp.stl"),
                            tolerance=0.02, angularTolerance=0.1)

    ok = run_checks(bezel, shell, board, clamp)

    if do_png:
        print("\nRendering previews ...")
        render_all(bezel, shell, board, clamp)
    return ok


def main():
    do_png = "--no-png" not in sys.argv
    envs = [a for a in sys.argv[1:] if not a.startswith("--")] or list(BOARDS)
    unknown = [e for e in envs if e not in BOARDS]
    if unknown:
        print("unknown board(s): %s (known: %s)" % (", ".join(unknown),
                                                     ", ".join(BOARDS)))
        return 2
    results = {env: build_and_export(env, do_png) for env in envs}
    print("\n" + "\n".join("%-18s %s" % (e, "OK" if r else "CHECKS FAILED")
                           for e, r in results.items()))
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
