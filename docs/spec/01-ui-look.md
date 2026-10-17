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

## Layout at 1920×1040 (client coordinates are approximate)
| Region | Rect / size |
|---|---|
| Menu bar | y 24–42 |
| Tab panel | y 46–60 |
| Main toolbar | y 62–98, buttons ~30 px |
| Viewport area | x 0–1736, y 100–956 |
| Viewport split | vertical at x ~868, horizontal at y ~528; gap ~6 px incl. frames |
| Command panel | x ~1740–1920 (~180 px wide) |
| Time slider | y ~957–973, label `0 / 100` |
| Status area | y ~990–1040 |

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
