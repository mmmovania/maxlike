"""Generates the toolbar icon strips (original placeholder art):

  res/toolbar_icons.bmp  24x24 cells, main toolbar, order of ui::toolbar::Icon
  res/status_icons.bmp   16x16 cells, status-area buttons, order of ui::toolbar::StatusIcon
  res/panel_tab_icons.bmp 22x20 cells, command panel tabs, order of ui::cmdpanel::Tab
  res/panel_category_icons.bmp 16x16 cells, Create panel categories, order of ui::cmdpanel::Category
  res/objects_icons.bmp  24x24 cells, the tab panel's Objects strip, order of ObjectsToolbarSpec

Both in src/ui/ToolbarSpec.h. Magenta (#FF00FF) is the transparent colour.
Drawn pixel-exact with no anti-aliasing, in the spirit of 1999 toolbar art.

    python tools/make_icons.py          (needs Pillow: pip install pillow)
"""
from PIL import Image, ImageDraw
import os

S = 24
KEY = (255, 0, 255)
INK = (0, 0, 0)
DARK = (64, 64, 64)
GREY = (128, 128, 128)
WHITE = (255, 255, 255)
BLUE = (40, 80, 200)
LBLUE = (150, 180, 240)
RED = (200, 30, 30)
YELLOW = (240, 200, 40)
GREEN = (40, 150, 60)
TEAL = (40, 140, 150)


def arrow_cursor(d, x, y):
    pts = [(x, y), (x, y + 13), (x + 3, y + 10), (x + 6, y + 15), (x + 8, y + 14),
           (x + 5, y + 9), (x + 9, y + 9)]
    d.polygon(pts, fill=WHITE, outline=INK)


def box(d, x0, y0, x1, y1, fill=LBLUE):
    d.rectangle([x0, y0, x1, y1], fill=fill, outline=BLUE)


def curved_arrow(d, flip):
    # A thick arc with an arrowhead; flip=True mirrors it horizontally.
    def P(x, y):
        return (23 - x, y) if flip else (x, y)
    for r in (7, 8, 9):
        pts = []
        import math
        for i in range(0, 181, 6):
            a = math.radians(180 + i)
            pts.append(P(12 + r * math.cos(a), 15 + r * math.sin(a)))
        d.line(pts, fill=DARK, width=1)
    d.polygon([P(1, 14), P(7, 14), P(4, 19)], fill=DARK)


def letter(d, ch, x, y):
    # Blocky 2-px-stroke letters drawn with rectangles.
    def R(a, b, c, e):
        d.rectangle([x + a, y + b, x + c, y + e], fill=DARK)
    if ch == "X":
        for i in range(10):
            R(i, i, i + 2, i); R(9 - i, i, 11 - i, i)
    elif ch == "Y":
        for i in range(5):
            R(i, i, i + 2, i); R(9 - i, i, 11 - i, i)
        R(4, 5, 7, 9)
    elif ch == "Z":
        R(0, 0, 11, 1); R(0, 8, 11, 9)
        for i in range(2, 8):
            R(9 - i, i, 11 - i, i)


def small_letter(d, ch, x):
    """8x8 letters for the plane constraints (XY, YZ, ZX), 2-px strokes."""
    y = 8
    if ch == "X":
        for i in range(8):
            d.point([(x + i, y + i), (x + 7 - i, y + i), (x + i + 1, y + i), (x + 8 - i, y + i)], fill=DARK)
    elif ch == "Y":
        for i in range(4):
            d.point([(x + i, y + i), (x + 7 - i, y + i), (x + i + 1, y + i), (x + 8 - i, y + i)], fill=DARK)
        d.rectangle([x + 3, y + 4, x + 5, y + 7], fill=DARK)
    elif ch == "Z":
        d.rectangle([x, y, x + 8, y + 1], fill=DARK); d.rectangle([x, y + 6, x + 8, y + 7], fill=DARK)
        for i in range(2, 6):
            d.point([(x + 7 - i, y + i), (x + 8 - i, y + i)], fill=DARK)


def small_pair(d, a, b):
    small_letter(d, a, 2); small_letter(d, b, 13)


def dashed_points(d, pts):
    for i, p in enumerate(pts):
        if i % 2 == 0:
            d.point([p], fill=INK)


def icon(d, n):
    if n == 0:   # help: cursor + question mark
        arrow_cursor(d, 3, 4)
        d.arc([12, 3, 20, 10], 180, 90, fill=BLUE, width=2)
        d.rectangle([15, 10, 16, 13], fill=BLUE); d.rectangle([15, 16, 16, 17], fill=BLUE)
    elif n == 1: curved_arrow(d, False)                      # undo
    elif n == 2: curved_arrow(d, True)                       # redo
    elif n == 3:   # link: two boxes joined by a chain
        box(d, 2, 2, 8, 8); box(d, 15, 15, 21, 21)
        d.ellipse([7, 7, 12, 12], outline=RED); d.ellipse([11, 11, 16, 16], outline=RED)
    elif n == 4:   # unlink: two boxes, broken chain
        box(d, 2, 2, 8, 8); box(d, 15, 15, 21, 21)
        d.ellipse([7, 7, 11, 11], outline=RED); d.ellipse([13, 13, 17, 17], outline=RED)
        d.point([(12, 9), (9, 13), (14, 11)], fill=RED)
    elif n == 5:   # bind to space warp: wave + box
        for i in range(3):
            d.arc([1, 3 + i * 4, 9, 9 + i * 4], 180, 360, fill=BLUE)
            d.arc([8, 3 + i * 4, 16, 9 + i * 4], 0, 180, fill=BLUE)
        box(d, 13, 13, 21, 21)
    elif n == 6: arrow_cursor(d, 8, 4)                       # select
    elif n == 7:   # rectangular region: dashed rectangle
        for x in range(3, 21, 2):
            d.point([(x, 4), (x, 19)], fill=INK)
        for y in range(4, 20, 2):
            d.point([(3, y), (20, y)], fill=INK)
        d.rectangle([6, 7, 17, 16], fill=LBLUE)
    elif n == 8:   # select by name: list lines + small cursor
        for i, w in enumerate((12, 9, 12, 7, 11, 9)):
            d.line([(2, 3 + i * 3), (2 + w, 3 + i * 3)], fill=INK)
        arrow_cursor(d, 13, 8)
    elif n == 9:   # move: four-way arrow
        d.rectangle([11, 4, 12, 19], fill=DARK); d.rectangle([4, 11, 19, 12], fill=DARK)
        d.polygon([(11, 1), (12, 1), (15, 5), (8, 5)], fill=DARK)
        d.polygon([(11, 22), (12, 22), (15, 18), (8, 18)], fill=DARK)
        d.polygon([(1, 11), (1, 12), (5, 15), (5, 8)], fill=DARK)
        d.polygon([(22, 11), (22, 12), (18, 15), (18, 8)], fill=DARK)
    elif n == 10:  # rotate: circular arrow
        d.arc([3, 3, 20, 20], 300, 240, fill=DARK, width=3)
        d.polygon([(13, 1), (19, 5), (13, 9)], fill=DARK)
    elif n == 11:  # scale: small box inside a large one
        d.rectangle([3, 3, 20, 20], outline=DARK)
        d.rectangle([6, 11, 12, 17], fill=RED, outline=INK)
        d.line([(12, 11), (18, 5)], fill=DARK); d.polygon([(19, 4), (15, 5), (18, 8)], fill=DARK)
    elif n == 12:  # pivot center: target in a box
        d.rectangle([3, 3, 20, 20], outline=GREY)
        d.ellipse([7, 7, 16, 16], outline=RED)
        d.line([(11, 5), (11, 18)], fill=INK); d.line([(5, 11), (18, 11)], fill=INK)
    elif n == 13: letter(d, "X", 6, 7)
    elif n == 14: letter(d, "Y", 6, 7)
    elif n == 15: letter(d, "Z", 6, 7)
    elif n == 16: small_pair(d, "X", "Y")                    # XY
    elif n == 17:  # IK: three joints connected
        pts = [(4, 18), (11, 6), (19, 15)]
        d.line(pts, fill=GREY, width=2)
        for (x, y), c in zip(pts, (BLUE, RED, BLUE)):
            d.ellipse([x - 2, y - 2, x + 2, y + 2], fill=c, outline=INK)
    elif n == 18:  # mirror: two triangles and an axis
        d.polygon([(2, 4), (9, 11), (2, 19)], fill=TEAL, outline=INK)
        d.polygon([(21, 4), (14, 11), (21, 19)], fill=LBLUE, outline=INK)
        d.line([(11, 2), (11, 21)], fill=RED); d.line([(12, 2), (12, 21)], fill=RED)
    elif n == 19:  # array: 3x3 dots
        for i in range(3):
            for j in range(3):
                x, y = 4 + i * 7, 4 + j * 7
                d.ellipse([x - 2, y - 2, x + 2, y + 2], fill=BLUE if (i + j) % 2 else TEAL)
    elif n == 20:  # align: two bars on a common line
        d.rectangle([3, 4, 10, 14], fill=LBLUE, outline=BLUE)
        d.rectangle([13, 8, 20, 14], fill=YELLOW, outline=INK)
        d.line([(1, 16), (22, 16)], fill=RED)
    elif n == 21:  # track view: tracks with keys
        for i in range(4):
            y = 4 + i * 5
            d.line([(2, y), (21, y)], fill=GREY)
            d.rectangle([5 + i * 3, y - 1, 7 + i * 3, y + 1], fill=YELLOW, outline=INK)
    elif n == 22:  # schematic: boxes connected as a tree
        box(d, 8, 2, 15, 7, fill=YELLOW)
        box(d, 2, 15, 9, 20); box(d, 14, 15, 21, 20)
        d.line([(11, 8), (11, 11), (5, 11), (5, 14)], fill=INK)
        d.line([(11, 11), (17, 11), (17, 14)], fill=INK)
    elif n == 23:  # material editor: four spheres
        for (x, y), c in zip(((7, 7), (16, 7), (7, 16), (16, 16)), (GREY, WHITE, TEAL, RED)):
            d.ellipse([x - 4, y - 4, x + 4, y + 4], fill=c, outline=INK)
            d.point([(x - 2, y - 2)], fill=WHITE)
    elif n == 24:  # render scene: window frame with a sphere
        d.rectangle([2, 3, 21, 20], fill=WHITE, outline=INK)
        d.rectangle([3, 4, 20, 6], fill=BLUE)
        d.ellipse([7, 9, 16, 18], fill=TEAL, outline=INK)
    elif n == 25:  # quick render: sphere with a spark
        d.ellipse([3, 7, 15, 19], fill=TEAL, outline=INK)
        d.polygon([(17, 2), (21, 2), (18, 7), (21, 7), (14, 14), (16, 8), (14, 8)], fill=YELLOW, outline=INK)
    elif n == 26:  # render last: sphere with a curved arrow
        d.ellipse([3, 9, 14, 20], fill=TEAL, outline=INK)
        d.arc([8, 2, 21, 15], 200, 340, fill=DARK, width=2)
        d.polygon([(18, 3), (22, 7), (17, 8)], fill=DARK)

    # ---- flyout alternatives (step 1.4c) ----
    elif n == 27:  # circular region
        import math
        dashed_points(d, [(11.5 + 9 * math.cos(a / 40 * 2 * math.pi), 11.5 + 9 * math.sin(a / 40 * 2 * math.pi)) for a in range(40)])
        d.ellipse([6, 6, 17, 17], fill=LBLUE)
    elif n == 28:  # fence region
        poly = [(3, 6), (12, 2), (20, 8), (17, 20), (6, 17)]
        d.polygon(poly, fill=LBLUE)
        for (x0, y0), (x1, y1) in zip(poly, poly[1:] + poly[:1]):
            steps = max(abs(x1 - x0), abs(y1 - y0))
            dashed_points(d, [(x0 + (x1 - x0) * t / steps, y0 + (y1 - y0) * t / steps) for t in range(steps)])
    elif n == 29:  # non-uniform scale: box stretched sideways
        d.rectangle([3, 7, 20, 16], outline=DARK)
        d.rectangle([7, 9, 16, 14], fill=RED, outline=INK)
        d.polygon([(0, 11), (3, 8), (3, 15)], fill=DARK); d.polygon([(23, 11), (20, 8), (20, 15)], fill=DARK)
    elif n == 30:  # squash: flattened box, arrows pressing in
        d.rectangle([3, 9, 20, 15], fill=RED, outline=INK)
        d.polygon([(11, 7), (8, 3), (15, 3)], fill=DARK); d.polygon([(11, 17), (8, 21), (15, 21)], fill=DARK)
    elif n == 31:  # selection center: target between two boxes
        box(d, 2, 2, 7, 7); box(d, 16, 16, 21, 21)
        d.ellipse([8, 8, 15, 15], outline=RED)
        d.line([(11, 6), (11, 17)], fill=INK); d.line([(6, 11), (17, 11)], fill=INK)
    elif n == 32:  # transform coordinate center: axis tripod with a target at the origin
        d.line([(5, 18), (21, 18)], fill=RED); d.line([(5, 18), (5, 2)], fill=GREEN)
        d.ellipse([2, 15, 8, 21], outline=INK)
        box(d, 12, 5, 19, 12)
    elif n == 33: small_pair(d, "Y", "Z")                    # YZ
    elif n == 34: small_pair(d, "Z", "X")                    # ZX
    elif n == 35:  # snapshot: copies along a path
        for i, c in enumerate(((200, 210, 240), (150, 180, 240), LBLUE)):
            box(d, 2 + i * 6, 12 - i * 4, 9 + i * 6, 19 - i * 4, fill=c)
    elif n == 36:  # spacing tool: dots spaced along a curve
        d.arc([2, 4, 30, 40], 190, 270, fill=GREY)
        for x, y in ((3, 18), (7, 12), (12, 8), (18, 6)):
            d.ellipse([x - 2, y - 2, x + 2, y + 2], fill=BLUE)
    elif n == 37:  # normal align: two surfaces with normals meeting
        d.line([(2, 20), (10, 12)], fill=BLUE, width=2); d.line([(13, 12), (21, 20)], fill=TEAL, width=2)
        d.line([(6, 16), (10, 6)], fill=RED); d.line([(17, 16), (13, 6)], fill=RED)
    elif n == 38:  # place highlight: a small sun shining on a surface
        d.ellipse([3, 3, 10, 10], fill=YELLOW, outline=INK)
        d.line([(10, 10), (17, 17)], fill=YELLOW)
        d.line([(12, 20), (22, 14)], fill=BLUE, width=2)
    elif n == 39:  # align camera: camera body with a lens
        d.rectangle([2, 8, 14, 17], fill=GREY, outline=INK)
        d.polygon([(14, 10), (21, 6), (21, 19), (14, 15)], fill=LBLUE, outline=INK)
        d.ellipse([4, 4, 8, 8], outline=INK); d.ellipse([9, 4, 13, 8], outline=INK)
    elif n == 40:  # align to view: box turning to face the viewer
        d.rectangle([3, 3, 20, 20], outline=GREY)
        box(d, 7, 7, 16, 16)
        d.polygon([(19, 1), (22, 4), (19, 7)], fill=DARK)
    elif n == 41:  # quick render (draft): outlined sphere with a spark
        d.ellipse([3, 7, 15, 19], fill=WHITE, outline=TEAL)
        d.polygon([(17, 2), (21, 2), (18, 7), (21, 7), (14, 14), (16, 8), (14, 8)], fill=YELLOW, outline=INK)



# ---------------------------------------------------------------- 16x16 status icons
def tri(d, pts, c=DARK):
    d.polygon(pts, fill=c)


def magnet(d, x, y):
    # A small horseshoe magnet (snap toggles).
    d.arc([x, y, x + 8, y + 8], 90, 360, fill=RED, width=3)
    d.rectangle([x + 6, y + 4, x + 8, y + 8], fill=RED)
    d.rectangle([x + 6, y + 7, x + 8, y + 8], fill=GREY)
    d.rectangle([x, y + 4, x + 2, y + 5], fill=GREY)


def status_icon(d, n):
    if n == 0:    # degradation override: a plain rounded box
        d.rounded_rectangle([1, 4, 14, 12], radius=3, fill=WHITE, outline=GREY)
    elif n == 1:  # window/crossing: dotted box with a sphere crossing it
        for x in range(1, 14, 2):
            d.point([(x, 2), (x, 12)], fill=INK)
        for y in range(2, 13, 2):
            d.point([(1, y), (12, y)], fill=INK)
        d.ellipse([8, 6, 14, 12], fill=GREY, outline=INK)
    elif n == 2:  # adaptive degradation: shaded cube
        d.polygon([(2, 5), (8, 2), (14, 5), (8, 8)], fill=WHITE, outline=INK)
        d.polygon([(2, 5), (8, 8), (8, 14), (2, 11)], fill=GREY, outline=INK)
        d.polygon([(8, 8), (14, 5), (14, 11), (8, 14)], fill=DARK, outline=INK)
    elif n in (3, 4, 5):  # 3D / 2.5D / 2D snap
        magnet(d, 1, 5)
        label = {3: "3", 4: "2.5", 5: "2"}[n]
        if label == "3":
            d.line([(11, 1), (14, 1), (12, 3), (14, 4), (11, 6)], fill=INK)
        elif label == "2":
            d.line([(11, 1), (14, 1), (14, 3), (11, 5), (14, 5)], fill=INK)
        else:
            d.line([(10, 1), (12, 1), (12, 2), (10, 4), (12, 4)], fill=INK)
            d.point([(13, 4)], fill=INK); d.line([(14, 1), (15, 1), (14, 2), (15, 3), (14, 4)], fill=INK)
    elif n == 6:  # angle snap
        magnet(d, 1, 6)
        d.line([(9, 13), (15, 13)], fill=INK); d.line([(9, 13), (14, 4)], fill=INK)
    elif n == 7:  # percent snap
        magnet(d, 1, 6)
        d.point([(10, 1), (14, 5)], fill=INK); d.line([(14, 1), (10, 5)], fill=INK)
    elif n == 8:  # spinner snap
        magnet(d, 1, 6)
        d.rectangle([11, 2, 15, 12], outline=GREY)
        tri(d, [(13, 3), (12, 6), (14, 6)]); tri(d, [(13, 11), (12, 8), (14, 8)])
    elif n == 9:   # go to start
        d.rectangle([2, 4, 3, 11], fill=BLUE); tri(d, [(4, 8), (9, 4), (9, 12)], BLUE); tri(d, [(9, 8), (14, 4), (14, 12)], BLUE)
    elif n == 10:  # previous frame
        d.rectangle([3, 4, 5, 11], fill=BLUE); tri(d, [(7, 8), (12, 3), (12, 13)], BLUE)
    elif n == 11:  # play
        tri(d, [(4, 3), (13, 8), (4, 13)], BLUE)
    elif n == 12:  # play selected
        tri(d, [(4, 3), (13, 8), (4, 13)], LBLUE)
        d.rectangle([1, 1, 14, 14], outline=GREY)
    elif n == 13:  # next frame
        tri(d, [(4, 3), (9, 8), (4, 13)], BLUE); d.rectangle([11, 4, 13, 11], fill=BLUE)
    elif n == 14:  # go to end
        tri(d, [(2, 4), (7, 8), (2, 12)], BLUE); tri(d, [(7, 4), (12, 8), (7, 12)], BLUE); d.rectangle([12, 4, 13, 11], fill=BLUE)
    elif n == 15:  # key mode
        d.ellipse([1, 5, 6, 10], outline=BLUE, width=2)
        d.rectangle([6, 7, 14, 8], fill=BLUE); d.rectangle([11, 9, 12, 10], fill=BLUE); d.rectangle([14, 9, 14, 10], fill=BLUE)
    elif n == 16:  # time configuration: page with a clock
        d.rectangle([1, 1, 11, 12], fill=WHITE, outline=BLUE); d.rectangle([1, 1, 11, 3], fill=BLUE)
        d.ellipse([7, 7, 15, 15], fill=WHITE, outline=RED); d.line([(11, 9), (11, 11), (13, 11)], fill=RED)
    elif n in (17, 18):  # zoom / zoom all
        if n == 18:
            d.rectangle([5, 1, 15, 11], outline=RED); d.line([(10, 1), (10, 11)], fill=RED); d.line([(5, 6), (15, 6)], fill=RED)
        d.ellipse([1, 4, 9, 12] if n == 18 else [2, 1, 11, 10], fill=WHITE, outline=INK)
        if n == 17:
            d.line([(10, 9), (14, 13)], fill=INK, width=2)
        else:
            d.line([(8, 11), (11, 14)], fill=INK, width=2)
    elif n in (19, 20):  # zoom extents / zoom extents selected: box in corner brackets
        c = INK if n == 19 else RED
        d.polygon([(4, 6), (8, 4), (12, 6), (8, 8)], fill=WHITE, outline=INK)
        d.polygon([(4, 6), (8, 8), (8, 12), (4, 10)], fill=LBLUE, outline=INK)
        d.polygon([(8, 8), (12, 6), (12, 10), (8, 12)], fill=GREY, outline=INK)
        for (x, y, dx, dy) in ((1, 1, 1, 1), (14, 1, -1, 1), (1, 14, 1, -1), (14, 14, -1, -1)):
            d.line([(x, y), (x + 3 * dx, y)], fill=c); d.line([(x, y), (x, y + 3 * dy)], fill=c)
    elif n in (21, 22):  # zoom extents all / all selected: two boxes, grid behind
        c = RED if n == 21 else GREY
        d.rectangle([6, 1, 15, 10], outline=c); d.line([(10, 1), (10, 10)], fill=c); d.line([(6, 5), (15, 5)], fill=c)
        d.polygon([(1, 8), (5, 6), (9, 8), (5, 10)], fill=WHITE, outline=INK)
        d.polygon([(1, 8), (5, 10), (5, 14), (1, 12)], fill=LBLUE, outline=INK)
        d.polygon([(5, 10), (9, 8), (9, 12), (5, 14)], fill=GREY, outline=INK)
    elif n == 23:  # region zoom: dashed box with a magnifier
        for x in range(1, 15, 2):
            d.point([(x, 1), (x, 14)], fill=INK)
        for y in range(1, 15, 2):
            d.point([(1, y), (14, y)], fill=INK)
        d.ellipse([4, 3, 10, 9], fill=WHITE, outline=INK); d.line([(9, 9), (12, 12)], fill=INK, width=2)
    elif n == 24:  # pan: an open hand
        for x in (3, 6, 9, 12):
            d.rectangle([x, 2 if x in (6, 9) else 4, x + 1, 9], fill=WHITE, outline=INK)
        d.rounded_rectangle([3, 8, 13, 14], radius=2, fill=WHITE, outline=INK)
    elif n in (25, 26, 27):  # arc rotate / selected / sub-object
        c = {25: GREY, 26: YELLOW, 27: RED}[n]
        d.ellipse([3, 3, 12, 12], outline=BLUE)
        d.ellipse([6, 6, 9, 9], fill=c, outline=INK)
        tri(d, [(12, 5), (15, 8), (12, 8)], BLUE); tri(d, [(3, 10), (0, 7), (3, 7)], BLUE)
    elif n == 28:  # min/max toggle: a small window inside a larger one
        d.rectangle([1, 1, 14, 14], outline=BLUE); d.rectangle([1, 1, 8, 8], fill=WHITE, outline=BLUE)
        d.line([(9, 9), (13, 13)], fill=INK); tri(d, [(13, 13), (10, 13), (13, 10)], INK)


STATUS_COUNT = 29  # must equal ui::toolbar::kStatusIconCount

COUNT = 42  # must equal ui::toolbar::kIconCount

img = Image.new("RGB", (S * COUNT, S), KEY)
for n in range(COUNT):
    cell = Image.new("RGB", (S, S), KEY)
    icon(ImageDraw.Draw(cell), n)
    img.paste(cell, (n * S, 0))

out = os.path.join(os.path.dirname(__file__), "..", "res", "toolbar_icons.bmp")
img.save(out)
print("wrote", os.path.normpath(out), img.size)

T = 16
simg = Image.new("RGB", (T * STATUS_COUNT, T), KEY)
for n in range(STATUS_COUNT):
    cell = Image.new("RGB", (T, T), KEY)
    status_icon(ImageDraw.Draw(cell), n)
    simg.paste(cell, (n * T, 0))
sout = os.path.join(os.path.dirname(__file__), "..", "res", "status_icons.bmp")
simg.save(sout)
print("wrote", os.path.normpath(sout), simg.size)


# ---------------------------------------------------------------- 22x20 command panel tabs
def tab_icon(d, n):
    if n == 0:    # Create: an arrow cursor with a sparkle
        d.line([(9, 3), (9, 17)], fill=INK)
        d.line([(9, 3), (19, 13)], fill=INK)
        d.line([(9, 17), (12, 14)], fill=INK)
        d.line([(12, 14), (15, 19)], fill=INK)
        d.line([(15, 19), (17, 18)], fill=INK)
        d.line([(17, 18), (14, 13)], fill=INK)
        d.line([(14, 13), (19, 13)], fill=INK)
        d.polygon([(10, 5), (10, 15), (12, 13), (15, 18), (16, 17), (13, 12), (17, 12)], fill=WHITE)
        for x, y in ((4, 1), (4, 2), (4, 4), (4, 5), (2, 3), (3, 3), (5, 3), (6, 3)):
            d.point((x, y), fill=YELLOW)
        d.point((4, 3), fill=WHITE)
        d.point((1, 7), fill=YELLOW)
        d.point((7, 0), fill=YELLOW)
    elif n == 1:  # Modify: a thick bent pipe (a shaded quarter ring)
        c, ro, ri = (20, 19), 18, 8
        d.pieslice([c[0] - ro, c[1] - ro, c[0] + ro, c[1] + ro], 180, 270, fill=BLUE)
        d.pieslice([c[0] - 14, c[1] - 14, c[0] + 14, c[1] + 14], 180, 270, fill=LBLUE)
        d.pieslice([c[0] - 11, c[1] - 11, c[0] + 11, c[1] + 11], 180, 270, fill=BLUE)
        d.pieslice([c[0] - ri, c[1] - ri, c[0] + ri, c[1] + ri], 180, 270, fill=KEY)
        d.arc([c[0] - ro, c[1] - ro, c[0] + ro, c[1] + ro], 180, 270, fill=INK)
        d.arc([c[0] - ri, c[1] - ri, c[0] + ri, c[1] + ri], 180, 270, fill=INK)
        d.line([(2, 19), (12, 19)], fill=INK)
        d.line([(20, 1), (20, 11)], fill=INK)
    elif n == 2:  # Hierarchy: a tree of boxes
        d.rectangle([8, 1, 13, 6], outline=BLUE, fill=WHITE)
        d.line([(10, 7), (10, 10)], fill=INK)
        d.line([(3, 10), (18, 10)], fill=INK)
        for x in (3, 10, 18):
            d.line([(x, 10), (x, 13)], fill=INK)
        for x0 in (1, 8, 15):
            d.rectangle([x0, 13, x0 + 5, 18], outline=BLUE, fill=WHITE)
    elif n == 3:  # Motion: a wheel with motion lines
        d.ellipse([6, 2, 21, 17], outline=INK, fill=WHITE)
        d.ellipse([8, 4, 19, 15], outline=GREY)
        for a, b in (((13, 3), (13, 16)), ((7, 9), (20, 9)), ((9, 5), (18, 14)), ((18, 5), (9, 14))):
            d.line([a, b], fill=GREY)
        d.ellipse([11, 7, 15, 11], fill=RED, outline=INK)
        for y in (5, 9, 13):
            d.line([(0, y), (3, y)], fill=DARK)
    elif n == 4:  # Display: a monitor
        d.rectangle([1, 1, 20, 15], fill=DARK)
        d.rectangle([3, 3, 18, 13], fill=WHITE)
        d.rectangle([5, 5, 8, 12], fill=BLUE)
        d.polygon([(10, 12), (13, 5), (16, 12)], fill=RED)
        d.rectangle([8, 16, 13, 17], fill=DARK)
        d.rectangle([5, 18, 16, 19], fill=GREY)
        d.point((15, 16), fill=GREEN)
    elif n == 5:  # Utilities: a hammer
        d.rectangle([4, 1, 17, 5], fill=DARK)
        d.rectangle([5, 2, 16, 3], fill=GREY)
        d.rectangle([9, 6, 12, 19], fill=RED)
        d.line([(10, 6), (10, 19)], fill=(230, 90, 70))
        d.line([(12, 6), (12, 19)], fill=(130, 20, 20))


TAB_COUNT = 6  # must equal ui::cmdpanel::kTabCount
TW, TH = 22, 20
timg = Image.new("RGB", (TW * TAB_COUNT, TH), KEY)
for n in range(TAB_COUNT):
    cell = Image.new("RGB", (TW, TH), KEY)
    tab_icon(ImageDraw.Draw(cell), n)
    timg.paste(cell, (n * TW, 0))
tout = os.path.join(os.path.dirname(__file__), "..", "res", "panel_tab_icons.bmp")
timg.save(tout)
print("wrote", os.path.normpath(tout), timg.size)


# ---------------------------------------------------------------- 16x16 Create categories
def category_icon(d, n):
    if n == 0:    # Geometry: a sphere lit from the upper left
        d.ellipse([1, 1, 14, 14], fill=(60, 60, 70))
        for (x0, y0, x1, y1), c in ((( 2, 2, 12, 12), (100, 100, 110)), ((3, 3, 10, 10), (140, 140, 150)),
                                    ((3, 3, 8, 8), (180, 180, 190)), ((4, 4, 6, 6), (225, 225, 230))):
            d.ellipse([x0, y0, x1, y1], fill=c)
        d.point((5, 4), fill=WHITE)
        d.ellipse([1, 1, 14, 14], outline=(30, 30, 40))
    elif n == 1:  # Shapes: a curve through three control points
        d.arc([1, 3, 14, 18], 200, 340, fill=BLUE)
        d.line([(2, 13), (7, 4), (13, 13)], fill=GREY)
        for x, y in ((2, 13), (7, 4), (13, 13)):
            d.rectangle([x - 1, y - 1, x + 1, y + 1], fill=RED)
    elif n == 2:  # Lights: a spotlight and its beam
        d.polygon([(2, 2), (7, 2), (9, 6), (4, 8)], fill=DARK)
        d.polygon([(8, 6), (5, 8), (11, 15), (15, 11)], fill=YELLOW)
        d.line([(5, 8), (11, 15)], fill=(200, 160, 20))
        d.line([(8, 6), (15, 11)], fill=(200, 160, 20))
    elif n == 3:  # Cameras: a film camera
        d.ellipse([2, 1, 6, 5], outline=INK, fill=GREY)
        d.ellipse([7, 1, 11, 5], outline=INK, fill=GREY)
        d.rectangle([1, 6, 11, 12], fill=DARK)
        d.polygon([(11, 8), (15, 6), (15, 13), (11, 11)], fill=INK)
        d.point((4, 9), fill=RED)
    elif n == 4:  # Helpers: a tape measure
        d.rectangle([1, 3, 9, 11], fill=YELLOW, outline=INK)
        d.ellipse([3, 5, 7, 9], outline=INK)
        d.rectangle([9, 9, 15, 11], fill=WHITE, outline=INK)
        for x in (11, 13):
            d.point((x, 10), fill=INK)
    elif n == 5:  # Space Warps: waves
        for y in (4, 8, 12):
            pts = [(x, y + (1 if (x // 2) % 2 else -1)) for x in range(1, 15)]
            d.line(pts, fill=BLUE)
    elif n == 6:  # Systems: two gears
        for cx, cy, r in ((5, 6, 4), (11, 11, 3)):
            d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=DARK)
            for dx, dy in ((0, -r - 1), (0, r + 1), (-r - 1, 0), (r + 1, 0)):
                d.point((cx + dx, cy + dy), fill=DARK)
            d.ellipse([cx - 1, cy - 1, cx + 1, cy + 1], fill=KEY)


CATEGORY_COUNT = 7  # must equal ui::cmdpanel::kCategoryCount
C = 16
cimg = Image.new("RGB", (C * CATEGORY_COUNT, C), KEY)
for n in range(CATEGORY_COUNT):
    cell = Image.new("RGB", (C, C), KEY)
    category_icon(ImageDraw.Draw(cell), n)
    cimg.paste(cell, (n * C, 0))
cout = os.path.join(os.path.dirname(__file__), "..", "res", "panel_category_icons.bmp")
cimg.save(cout)
print("wrote", os.path.normpath(cout), cimg.size)


# ---------------------------------------------------------------- 24x24 Objects strip
# Shaded solids lit from the upper left, in the spirit of R3's (original art).
OUT = (35, 45, 95)
LIT = (190, 205, 245)
MID = (120, 140, 215)
SHD = (70, 85, 160)
GRN = (40, 190, 60)
LGRN = (150, 240, 150)


def shaded_ellipse(d, box):
    x0, y0, x1, y1 = box
    d.ellipse(box, fill=SHD, outline=OUT)
    d.ellipse([x0 + 2, y0 + 2, x1 - 3, y1 - 3], fill=MID)
    d.ellipse([x0 + 3, y0 + 3, (x0 + x1) // 2, (y0 + y1) // 2], fill=LIT)


def prism(d, top, front, side):
    d.polygon(front, fill=MID, outline=OUT)
    d.polygon(side, fill=SHD, outline=OUT)
    d.polygon(top, fill=LIT, outline=OUT)


def object_icon(d, n):
    if n == 0:    # Box
        prism(d, [(3, 7), (13, 3), (21, 6), (11, 10)], [(3, 7), (11, 10), (11, 21), (3, 17)],
              [(11, 10), (21, 6), (21, 17), (11, 21)])
    elif n == 1:  # Sphere
        shaded_ellipse(d, [2, 2, 21, 21])
    elif n == 2:  # Cylinder
        d.rectangle([5, 5, 18, 18], fill=MID, outline=OUT)
        d.rectangle([6, 6, 9, 18], fill=LIT)
        d.ellipse([5, 15, 18, 21], fill=MID, outline=OUT)
        d.rectangle([6, 9, 17, 17], fill=MID)
        d.rectangle([6, 9, 9, 18], fill=LIT)
        d.ellipse([5, 2, 18, 8], fill=LIT, outline=OUT)
    elif n == 3:  # Torus
        d.ellipse([1, 6, 22, 18], fill=MID, outline=OUT)
        d.ellipse([3, 7, 14, 12], fill=LIT)
        d.ellipse([7, 10, 16, 14], fill=KEY, outline=OUT)
    elif n == 4:  # Teapot
        d.ellipse([5, 8, 18, 20], fill=MID, outline=OUT)
        d.ellipse([7, 9, 12, 13], fill=LIT)
        d.rectangle([9, 5, 14, 8], fill=MID, outline=OUT)
        d.line([(18, 12), (22, 8)], fill=OUT, width=2)
        d.arc([1, 10, 8, 17], 90, 270, fill=OUT)
    elif n == 5:  # Cone
        d.polygon([(11, 2), (3, 19), (19, 19)], fill=MID, outline=OUT)
        d.polygon([(11, 2), (5, 18), (9, 19)], fill=LIT)
        d.ellipse([3, 16, 19, 22], fill=SHD, outline=OUT)
    elif n == 6:  # GeoSphere: a faceted sphere
        shaded_ellipse(d, [2, 2, 21, 21])
        for a, b in (((11, 2), (4, 15)), ((11, 2), (18, 15)), ((4, 15), (18, 15)), ((2, 9), (21, 9))):
            d.line([a, b], fill=OUT)
    elif n == 7:  # Tube
        d.rectangle([4, 5, 19, 18], fill=MID, outline=OUT)
        d.rectangle([5, 7, 8, 18], fill=LIT)
        d.ellipse([4, 15, 19, 21], fill=MID, outline=OUT)
        d.rectangle([5, 9, 18, 17], fill=MID)
        d.rectangle([5, 9, 8, 18], fill=LIT)
        d.ellipse([4, 2, 19, 9], fill=LIT, outline=OUT)
        d.ellipse([8, 4, 15, 7], fill=SHD, outline=OUT)
    elif n == 8:  # Pyramid
        prism(d, [(11, 2), (11, 2), (11, 2)], [(11, 2), (3, 18), (12, 21)], [(11, 2), (12, 21), (20, 16)])
        d.line([(11, 2), (5, 17)], fill=LIT)
    elif n == 9:  # Plane: a grid
        d.rectangle([2, 4, 21, 19], fill=MID, outline=OUT)
        for x in (7, 12, 17):
            d.line([(x, 4), (x, 19)], fill=OUT)
        for y in (8, 12, 16):
            d.line([(2, y), (21, y)], fill=OUT)
    elif n == 10:  # Hedra: a star
        d.polygon([(11, 1), (14, 8), (21, 8), (15, 13), (18, 21), (11, 16), (4, 21), (7, 13), (1, 8), (8, 8)],
                  fill=MID, outline=OUT)
        d.polygon([(11, 1), (8, 8), (11, 11), (14, 8)], fill=LIT)
    elif n == 11:  # ChamferBox: a box with its edges rounded off
        prism(d, [(3, 7), (13, 3), (21, 6), (11, 10)], [(3, 7), (11, 10), (11, 21), (3, 17)],
              [(11, 10), (21, 6), (21, 17), (11, 21)])
        d.line([(4, 8), (11, 11)], fill=LIT)
        d.line([(12, 11), (20, 7)], fill=LIT)
        d.line([(11, 11), (11, 20)], fill=LIT)
    elif n == 12:  # OilTank: a cylinder with domed ends
        d.rectangle([5, 7, 18, 16], fill=MID, outline=OUT)
        d.pieslice([5, 2, 18, 12], 180, 360, fill=LIT, outline=OUT)
        d.pieslice([5, 11, 18, 21], 0, 180, fill=SHD, outline=OUT)
        d.rectangle([6, 7, 17, 16], fill=MID)
        d.rectangle([6, 7, 9, 16], fill=LIT)
    elif n == 13:  # ChamferCyl
        d.rounded_rectangle([4, 3, 19, 20], radius=4, fill=MID, outline=OUT)
        d.rectangle([6, 6, 9, 17], fill=LIT)
        d.ellipse([5, 3, 18, 8], fill=LIT, outline=OUT)
    elif n == 14:  # Spindle: a double cone on a cylinder
        d.polygon([(11, 1), (3, 8), (19, 8)], fill=LIT, outline=OUT)
        d.rectangle([3, 8, 19, 14], fill=MID, outline=OUT)
        d.polygon([(3, 14), (19, 14), (11, 22)], fill=SHD, outline=OUT)
    elif n == 15:  # Gengon: a five-sided prism
        prism(d, [(3, 8), (9, 3), (17, 4), (21, 9), (12, 11)], [(3, 8), (12, 11), (12, 20), (3, 17)],
              [(12, 11), (21, 9), (21, 17), (12, 20)])
    elif n == 16:  # RingWave: a toothed ring
        for i in range(12):
            import math
            a = 2 * math.pi * i / 12
            d.ellipse([11 + 9 * math.cos(a) - 2, 11 + 9 * math.sin(a) - 2, 11 + 9 * math.cos(a) + 2,
                       11 + 9 * math.sin(a) + 2], fill=MID, outline=OUT)
        d.ellipse([4, 4, 18, 18], outline=OUT, width=2)
        d.ellipse([7, 7, 15, 15], fill=KEY)
    elif n == 17:  # Torus Knot
        d.ellipse([2, 3, 14, 15], outline=OUT, width=3)
        d.ellipse([8, 8, 20, 20], outline=OUT, width=3)
        d.ellipse([3, 4, 13, 14], outline=LIT)
        d.ellipse([9, 9, 19, 19], outline=MID)
    elif n == 18:  # Capsule
        d.rounded_rectangle([7, 2, 16, 21], radius=4, fill=MID, outline=OUT)
        d.rectangle([9, 5, 10, 18], fill=LIT)
    elif n == 19:  # L-Ext
        prism(d, [(2, 10), (12, 6), (15, 7), (5, 11)], [(2, 10), (5, 11), (5, 18), (2, 17)], [(5, 11), (15, 7), (15, 14), (5, 18)])
        prism(d, [(12, 6), (15, 7), (20, 3), (17, 2)], [(15, 7), (20, 3), (20, 10), (15, 14)], [(15, 7), (15, 7), (15, 7)])
    elif n == 20:  # C-Ext
        prism(d, [(2, 8), (12, 4), (21, 7), (11, 11)], [(2, 8), (11, 11), (11, 19), (2, 16)], [(11, 11), (21, 7), (21, 15), (11, 19)])
        d.polygon([(6, 9), (12, 7), (17, 8), (11, 10)], fill=SHD, outline=OUT)
    elif n == 21:  # Prism: a triangular prism
        prism(d, [(3, 6), (18, 3), (12, 9)], [(3, 6), (12, 9), (12, 21), (3, 18)], [(12, 9), (18, 3), (21, 16), (12, 21)])
    elif n in (22, 23):  # Quad Patch, Tri Patch: green grids
        d.polygon([(11, 1), (22, 11), (11, 22), (1, 11)], fill=GRN, outline=OUT)
        for k in range(1, 4):
            d.line([(11 - 2.5 * k + 0.5, 1 + 2.5 * k), (22 - 2.5 * k, 11 + 2.5 * k)], fill=LGRN)
            d.line([(11 + 2.5 * k, 1 + 2.5 * k), (1 + 2.5 * k, 11 + 2.5 * k)], fill=LGRN)
        if n == 23:
            d.line([(1, 11), (22, 11)], fill=OUT)
    elif n in (24, 25):  # NURBS Point Surface, CV Surface: blue grids
        d.polygon([(11, 1), (22, 11), (11, 22), (1, 11)], fill=MID, outline=OUT)
        for k in range(1, 4):
            d.line([(11 - 2.5 * k + 0.5, 1 + 2.5 * k), (22 - 2.5 * k, 11 + 2.5 * k)], fill=SHD)
            d.line([(11 + 2.5 * k, 1 + 2.5 * k), (1 + 2.5 * k, 11 + 2.5 * k)], fill=SHD)
        if n == 25:
            for x, y in ((11, 1), (22, 11), (11, 22), (1, 11)):
                d.rectangle([x - 1, y - 1, x + 1, y + 1], fill=OUT)
    elif n == 26:  # Bones: red bones, blue joints
        d.line([(4, 18), (11, 9)], fill=RED, width=2)
        d.line([(11, 9), (19, 13)], fill=RED, width=2)
        for x, y in ((4, 18), (11, 9), (19, 13)):
            d.ellipse([x - 2, y - 2, x + 2, y + 2], fill=(90, 110, 220), outline=OUT)
    elif n == 27:  # Damper
        d.line([(3, 3), (8, 8)], fill=DARK, width=2)
        d.polygon([(8, 6), (12, 10), (19, 17), (15, 21), (8, 14), (6, 10)], fill=GREY, outline=OUT)
        d.polygon([(8, 6), (11, 9), (7, 13), (6, 10)], fill=WHITE)
        d.line([(18, 19), (21, 22)], fill=DARK, width=2)
    elif n == 28:  # Spring: a coil
        for y in range(5, 20, 4):
            d.arc([4, y - 3, 19, y + 3], 180, 360, fill=GREY, width=2)
            d.arc([4, y - 3, 19, y + 3], 0, 180, fill=OUT, width=2)


OBJ_COUNT = 29  # must equal the icon count in ObjectsToolbarSpec
oimg = Image.new("RGB", (S * OBJ_COUNT, S), KEY)
for n in range(OBJ_COUNT):
    cell = Image.new("RGB", (S, S), KEY)
    object_icon(ImageDraw.Draw(cell), n)
    oimg.paste(cell, (n * S, 0))
oout = os.path.join(os.path.dirname(__file__), "..", "res", "objects_icons.bmp")
oimg.save(oout)
print("wrote", os.path.normpath(oout), oimg.size)
