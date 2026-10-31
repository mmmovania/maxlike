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
