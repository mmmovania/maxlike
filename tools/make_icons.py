"""Generates res/toolbar_icons.bmp: original placeholder icons for the main toolbar.

One 24x24 cell per icon, left to right in the order of ui::toolbar::Icon
(src/ui/ToolbarSpec.h). Magenta (#FF00FF) is the transparent colour.
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


COUNT = 42  # must equal ui::toolbar::kIconCount

img = Image.new("RGB", (S * COUNT, S), KEY)
for n in range(COUNT):
    cell = Image.new("RGB", (S, S), KEY)
    icon(ImageDraw.Draw(cell), n)
    img.paste(cell, (n * S, 0))

out = os.path.join(os.path.dirname(__file__), "..", "res", "toolbar_icons.bmp")
img.save(out)
print("wrote", os.path.normpath(out), img.size)
