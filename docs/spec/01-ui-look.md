# UI look specification

Measured from `docs/reference/screenshots/max-r3.1-default-1920x1040.png`
(R3.1 running on Windows 10, 100% scaling). All sizes are pixels at 96 DPI.

## Colours
| Element | Value |
|---|---|
| Viewport background (all views, incl. perspective) | `#B0B0B0` |
| Minor grid lines | `#8D8D8D` |
| Major grid lines (every 10th) | `#747474` |
| Home-grid axes through origin | `#000000` |
| Viewport label text | `#E5E5E5` |
| Tripod X / Y / Z | `#FF0000` / `#00FF00` / `#0000FF` |
| Viewport frame, outer | `#A0A0A0` top-left, `#FFFFFF` bottom-right |
| Viewport frame, inner | `#000000` top-left, `#C0C0C0` bottom-right |
| UI face | `GetSysColor(COLOR_BTNFACE)` (`#F0F0F0` on Win10, `#D4D0C8` on Win2000) |
| Menu bar | `#FFFFFF` (system menu) |
| MAXScript mini-listener | `#DBC3C3` |
| Name and Color default swatch (sample) | random per object, e.g. `#8D073A` |

Unverified: active viewport highlight colour. Take a screenshot in Max after clicking a
viewport and fill this in.

## Grid
- Spacing 10 units; major line every 10 minor lines; status bar shows `Grid = 10.0`.
- Default ortho zoom ~0.46 units/px (≈ 40 cells / 400 units across an 863 px view).
- Measured minor spacing in Top view: ~21.55 px.
- Perspective default FOV 45°; grid drawn to a fixed home-grid extent.

## Layout (measured, client coordinates at 100%)
Client area = below the menu bar; a maximised 1920×1040 window gives a 1920×998 client.
Encoded in `src/ui/Metrics.h` and `src/ui/FrameLayout.cpp`, tested in
`tests/test_frame_layout.cpp`.

| Region | Rect at 1920×998 | Rule |
|---|---|---|
| Tab panel | 0,4 – 1920,19 | 4 px gap, 15 px high, full width |
| Main toolbar | 0,19 – 1920,57 | 38 px high, full width |
| Viewports | 0,57 – 1736,915 | fills the left column |
| Time slider | 0,915 – 1736,932 | 17 px, left column |
| Track bar | 0,932 – 1736,948 | 16 px, left column |
| Command panel | 1736,57 – 1920,948 | 184 px wide, `EDGE_RAISED` |
| Status area | 0,948 – 1920,998 | 50 px, full width |

## Tab panel (measured, step 1.3)
Code: `src/ui/TabStrip.*` (geometry), `src/ui/TabPanel.*` (drawing).

- 11 tabs, fixed width 89 px, first tab at x = 4, no gaps, 15 px high; labels centred.
- Each tab, columns/rows relative to its top-left (w = 89, h = 15):
  row 0 highlight cols 2..w-4 and dark col w-3; row 1 highlight col 1, dark col w-2;
  rows 2..h-2 highlight col 0, dark col w-1; bottom row highlight across inactive tabs,
  only col 0 for the active tab (open towards the toolbar).
- Inactive: fill `#A0A0B3`, dark edge `#A0A0A0`. Active: face fill, dark edge `#000000`.
- The bottom highlight continues from the last tab to 4 px from the right edge.
- Frame around tab panel + toolbar: highlight column at x = 0, `#A0A0A0` column at
  x = width − 2 (continues into the toolbar area; finish in step 1.4).

## Viewports (default)
| Pos | View | Construction plane | Tripod labels |
|---|---|---|---|
| Top-left | Top | XY | x right, y up |
| Top-right | Front | XZ | x right, z up |
| Bottom-left | Left | YZ | y left, z up (check screenshot) |
| Bottom-right | Perspective | XY | x, y, z |

Label: top-left, ~6 px inset. Tripod: bottom-left, ~40×40 px.

## Main toolbar (measured, step 1.4a)
Code: `src/ui/ToolbarSpec.*` (items + layout), `src/ui/Toolbar.*` (drawing, input).

- Items start at x = 4. Buttons 32 × 31 at y = 4, edge to edge; icon 24 × 24 at (4, 3).
- Separator: 6 px wide; etched line `#A0A0A0` at +3, white at +4, rows 12–27.
- Combos: classic sunken edge, 21 px high at y = 4; widths 70, 70, 120, 70.
- Explicit 2-px gaps before the "All" combo and before the render-type combo.
- Checked button: outer ring `#A0A0A0` / white, inner ring face / white; the left column
  and top row belong to the top-left colour except the top-right pixel; fill `#C0FFC0`
  for the active mode (Select/Move/Rotate/Scale), `#E1E1E1` for other checked buttons.
- Flyout marker (button-relative row, col): white band (25,29) (26,28) (26,29) (27,27)
  (27,28) (28,26) (28,27), black triangle (27,29) (28,28) (28,29); on a sunken button the
  band is black and the triangle `#A0A0A0`.
- Frame: white column at x = 0, `#A0A0A0` column at width − 2 (same as the tab panel).
- Radio groups: mode (Select ✔, Move, Rotate, Scale) and axis (X, Y, Z, XY ✔).
- Combos (step 1.4b) are native classic `COMBOBOX` controls; their closed height (21 px)
  follows from the 8 pt UI font. Entries, from memory and unverified:
  Selection Filter: All, Geometry, Shapes, Lights, Cameras, Helpers, Warps;
  Reference Coordinate System: View, Screen, World, Parent, Local, Grid, Pick;
  Render Type: View, Selected, Region, Crop, Blowup; Named Selection Sets: editable, empty.
- Tooltips: standard tooltip control in the UI font, one per button and combo.
- Flyouts (step 1.4c), alternatives from memory and unverified:
  Region: Rectangular, Circular, Fence · Scale: Uniform, Non-uniform, Squash ·
  Pivot: Pivot Point, Selection, Transform Coordinate Center · Plane: XY, YZ, ZX ·
  Array: Array, Snapshot, Spacing Tool · Align: Align, Normal Align, Place Highlight,
  Align Camera, Align to View · Quick Render: Production, Draft.
  Holding 300 ms opens a vertical strip whose first cell lies over the button.
- Unverified: hover look (a thin raised ring for now), pressed look, tooltip texts,
  combo entries, flyout alternatives, flyout strip look and delay.

## Tab panel (measured, step 1.3)
Code: `src/ui/TabStrip.*` (geometry), `src/ui/TabPanel.*` (drawing).

- 11 tabs, fixed width 89 px, first tab at x = 4, no gaps, 15 px high; labels centred.
- Each tab, columns/rows relative to its top-left (w = 89, h = 15):
  row 0 highlight cols 2..w-4 and dark col w-3; row 1 highlight col 1, dark col w-2;
  rows 2..h-2 highlight col 0, dark col w-1; bottom row highlight across inactive tabs,
  only col 0 for the active tab (open towards the toolbar).
- Inactive: fill `#A0A0B3`, dark edge `#A0A0A0`. Active: face fill, dark edge `#000000`.
- The bottom highlight continues from the last tab to 4 px from the right edge.
- Frame around tab panel + toolbar: highlight column at x = 0, `#A0A0A0` column at
  x = width − 2 (continues into the toolbar area; finish in step 1.4).

## Viewports (default)
| Pos | View | Construction plane | Tripod labels |
|---|---|---|---|
| Top-left | Top | XY | x right, y up |
| Top-right | Front | XZ | x right, z up |
| Bottom-left | Left | YZ | y left, z up (check screenshot) |
| Bottom-right | Perspective | XY | x, y, z |

Label: top-left, ~6 px inset. Tripod: bottom-left, ~40×40 px.

## Main toolbar (left to right)
Help, Undo, Redo, Select and Link, Unlink, Bind to Space Warp, Select Object,
Region type (flyout), Selection filter combo (`All`), Select by Name, Move, Rotate,
Scale (flyout), Reference coord system combo (`View`), Pivot center (flyout),
X, Y, Z, XY (flyout), Mirror, Array (flyout), Align (flyout), Named selection combo,
Track View, Schematic View, Material Editor, Render Scene, Quick Render (flyout),
Render type combo (`View`), Render last.

## Tab panel
Main Toolbar, Objects, Shapes, Compounds, Lights & Cameras, Particles, Helpers,
SpaceWarps, Modifiers, Modeling, Rendering.

## Command panel
- Tabs: Create, Modify, Hierarchy, Motion, Display, Utilities.
- Create categories: Geometry, Shapes, Lights, Cameras, Helpers, Space Warps, Systems.
- Subcategory combo: `Standard Primitives`.
- Rollout `Object Type`: AutoGrid (disabled checkbox), two-column buttons
  Box/Cone, Sphere/GeoSphere, Cylinder/Tube, Torus/Pyramid, Teapot/Plane.
- Rollout `Name and Color`: edit box + colour swatch.

## Status area
MAXScript mini-listener (pink), prompt line 1 (`None Selected`), prompt line 2
(`Click or click-and-drag to select objects`), selection lock, X/Y/Z fields,
`Grid = 10.0`, Animate button, playback controls, key mode, frame field,
2×4 viewport navigation buttons.

## Custom controls
| Control | Behaviour |
|---|---|
| Rollout | Bevelled title bar with `-`/`+`, collapses; stacked in a rollup that scrolls by hand-drag |
| Spinner | Edit + tiny up/down arrows; drag to change, Ctrl accelerates, right-click resets |
| Colour swatch | Bevelled fill; click opens picker |
| Check button | Looks pressed when on |
| Flyout button | Corner triangle; hold to open strip of alternatives |
