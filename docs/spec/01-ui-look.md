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
| Viewport border (2 px) | `#FFFFFF` around the active viewport, `#000000` around the others |
| Viewport ring (1 px, inside the border) | `#C0C0C0` |
| UI face | `GetSysColor(COLOR_BTNFACE)` (`#F0F0F0` on Win10, `#D4D0C8` on Win2000) |
| Menu bar | `#FFFFFF` (system menu) |
| MAXScript mini-listener | `#DBC3C3` |
| Name and Color default swatch (sample) | random per object, e.g. `#8D073A` |

Active viewport (measured in Phase 2): the reference shows Top active, with a white
2-px border; see "Viewport frames" below.

## Grid
- Spacing 10 units; major line every 10 minor lines; status bar shows `Grid = 10.0`.
- Colours: minor `#8D8D8D`, major `#747474`, axes `#000000`; drawn minor, major, axis.
- Ortho views (measured, step 2.2a; all 180 lines of the three ortho views reproduced):
  400 world units span `W − 0.5` pixels across; the visible height is `400·H/W` units
  over `H − 0.5` pixels; a line at coordinate `u` lands in column
  `floor((W−1)/2 + 0.745 + u·scaleX)` and a line at `v` in row
  `floor((H−1)/2 + 0.22 − v·scaleY)` (top-down). Code: `src/view/OrthoGrid.*`.
- Perspective (fitted, step 2.2b): the camera looks at the origin from 250 units away,
  15° above the ground, from the −Y side (looking along +Y), horizontal FOV 45°; the
  home grid spans ±200 units (its far edge is the line Y = 200). Every line pixel lies
  within 1 px of R3's. Code: `src/view/PerspectiveView.*`, `src/math/Mat4.*`.

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

## Time slider and track bar (measured, step 1.5a)
Code: `src/ui/TimeSliderGeometry.*`, `src/ui/TimeSlider.*`, `src/ui/TrackBar.*`.

- Time slider (17 px): trough `#E1E1E1`. A 157-px block slides along it: "<" (16 px),
  thumb (125 px), ">" (16 px); at frame 0 it starts at x = 0, at the last frame it ends
  at the right edge.
- Arrow buttons: rings white / `#696969` and `#E3E3E3` / `#A0A0A0` (Windows corner
  convention); 1-px chevron 7 rows tall from row 5, spanning columns 6–9.
- Thumb: ring white / black (R3 corner convention), inner `#A0A0A0` at the right and
  bottom, one white pixel at the inner bottom-left; label "frame / end", centred.
- Track bar (16 px): rows 0–1 white, 2 `#A0A0A0`, 3–13 `#E1E1E1`, 14 white,
  15 `#A0A0A0`; the last column stays `#E1E1E1`. A "[" marker in `#1E1E1E` at x = 0
  at frame 0 (meaning unverified; drawn at the current frame).

## Status area, left part (measured, step 1.5b)
Code: `src/ui/StatusLayout.*`, `src/ui/StatusBar.*`. Rows are relative to the status-area top.

- Mini-listener: two `EDIT` controls with `WS_EX_CLIENTEDGE`, x = 1, 148 wide, y 2–24
  (pink `#DBC3C3`, read-only output) and y 25–47 (white, input).
- Splitter: 2 px after the listener, 4 columns (white, face, `#A0A0A0`, black), full
  height; its top row is white across the first three columns. Draggable.
- Prompt lines: thin sunken fields (`#A0A0A0` / white, R3 corner convention), 21 high at
  y = 4 and y = 29, starting 2 px after the splitter; text 7 px from the left.
- Right-anchored (offsets from the right edge, left..right): lock 620..591, X 591..521,
  Y 519..449, Z 447..377, grid 375..275 (top row); top prompt ends at 620, bottom
  prompt at 566, Time Tag field 564..464 (bottom row; tooltip "Time Tag"; in R3 a
  click opens a menu to add/edit tags, not implemented yet).

## Status area, right part (measured, step 1.5c)
Code: `toolbar::StatusSnapConfig` / `StatusAnimConfig` in `src/ui/ToolbarSpec.cpp`,
drawn by the same `Toolbar` control as the main toolbar.

- Two fixed-size groups anchored to the right edge: toggles/snaps 464..275 from the right,
  rows 26–49; Animate/playback/navigation 275..0 from the right, full height.
- Buttons 23 × 22 with 16 × 16 icons at (4, 3); separators as on the main toolbar;
  the flyout notch sits 1 px further right than on the main toolbar.
- Animate: 48 × 44, raised with two rings white / `#A0A0A0` (R3 corners); red when on
  (colour unverified).
- Frame field: drawn frame, outer `#A0A0A0` / white, inner black / face; a borderless
  EDIT inside. It selects its text on focus; Enter sets the frame.
- Item names, tooltips and flyout alternatives: from memory, unverified.

## Viewport frames (measured, step 2.1)
Code: `src/ui/ViewportLayout.*` (geometry), `src/ui/ViewPanel.*` (frames).

- The area's first row is a 1-px line: white at x = 0, `#A0A0A0` elsewhere.
- Each viewport: 2-px border (white if active, black otherwise), a 1-px `#C0C0C0` ring,
  then the content (inset 3 px).
- Split lines at x = width / 2 and y = 1 + (height − 1) / 2 (column 868 and area row 429
  at 1736 × 858); neighbours share that line. The active viewport is drawn last, so
  next to it the black borders appear 1 px wide.
- At 1736 × 858: Top content x 3–865, area rows 4–426.

## Viewport labels and axis tripods (measured, step 2.3)
Code: `src/view/Tripod.*`, `src/ui/LabelText.*`, `Viewport::DrawLabelAndTripod`.

- Label: `#E5E5E5`, drawn over the grid without background; text starts at x = 4, the top
  of its ink at y = 4. Capitals 10 px with 2-px strokes: most likely MS Sans Serif Bold
  10 pt (unverified).
- Tripod: origin 25 px from the left and bottom; each axis ends at origin +
  round(24.5 × its on-screen direction) (rounded in a y-up frame); a line from origin to
  end, inclusive; the letter's top-left 9 px above the end. Letters: x 8×7, y 8×10 (with
  descender), z 7×7 (first column empty). Drawn X, Y, Z (later covers earlier).
  Colours `#FF0000`, `#00FF00`, `#0000FF`. All four reference tripods reproduced exactly.

## Viewports (default)
| Pos | View | Construction plane | Tripod | Default display |
|---|---|---|---|---|
| Top-left | Top | XY | x right, y up, z at the viewer | Wireframe |
| Top-right | Front | XZ | x right, z up, y away | Wireframe |
| Bottom-left | Left | YZ | y left, z up, x away | Wireframe |
| Bottom-right | Perspective | XY | x right, z up, y into the scene | Smooth + Highlights |

Label and tripod positions: see "Viewport labels and axis tripods" above (measured).
Other views from the label menu: Bottom (y down), Back (x left), Right (y right).

## Objects in the viewports (Phase 3)
Code: `Viewport::DrawScene`, `Viewport::DrawShaded`, `src/gl/MeshRenderer.*`.

- Wireframe: visible edges only (a quad's diagonal is hidden), all of them (no
  hidden-line removal), in the node's wire colour. Ortho views map points through the
  grid's pixel formula, so edges on grid lines land on the grid's pixels.
- Selected objects (step 6.2): white wireframes (R3.1 screenshots); in shaded views
  white brackets at the bounding-box corners, a quarter of each edge long (measured
  roughly from `r3.1-box-created.png`). Status line: "None Selected", "1 Object
  Selected" (verified), "N Objects Selected" (assumed).
- Shading modes (label menu): Smooth + Highlights, Wireframe, Other → Smooth, Facets +
  Highlights, Facets, Lit Wireframes (drawn as wireframe for now), Bounding Box.
- Lighting (unverified): ambient 25 %, diffuse 75 % in the object colour, white Blinn-Phong
  highlight (exponent 40) in the Highlights modes, one light above, left of and behind
  the viewer, two-sided. Needs an R3.1 screenshot of a shaded object.

## Navigation (Phase 2, behaviour unverified)
Status-area buttons set a mode until right-click: Zoom, Zoom All, Region Zoom, Pan, Arc
Rotate; Zoom Extents (All) frame the defaults while the scene is empty. Middle-drag pans,
Alt + middle orbits, Ctrl + Alt + middle zooms, the wheel zooms ×1.25 per notch. Grid
thins out when lines would be closer than 4 px. Cursors, prompts and directions are
from memory.

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

## Modify panel (measured, step 7.2a)
Screenshots `r3.1-modify-box.png`, `r3.1-modify-bend.png`,
`r3.1-modify-editmesh-vertex.png`, `r3.1-modifiers-dialog.png` (panel coordinates as for
the Create panel).
- Name field above the rollup: x 16–149, rows 35–54 (wider than the Create panel's
  15–146); swatch x 152–168, same rows.
- Rollup container from row 59 (Create: 94); same rollout rules (first title +5, frame
  +8, next title 7 after a frame).
- Modifiers (title 64, content 164): "Use Pivot Points" checkbox (disabled with one
  object) at title + 22; More… x 17–70 (dark-grey bottom edge) and Sets x 80–133 at rows
  104–121; an icon button x 141–161, rows 101–121; ten buttons in two columns, x 17–88
  and 92–163, 20 rows tall at rows 127, 150, 173, 195, 218: Bend, Taper / Twist, Noise /
  Extrude, Lathe (disabled for meshes) / UVW Map, Edit Patch / Mesh Select, Edit Spline
  (disabled).
- Modifier Stack (title 250, content 106): pin icon x 15–31; stack dropdown x 36–166,
  rows 271–294 ("Box", "Bend", "* Edit Mesh"); five icon buttons from x 36, rows
  302–324 (the second shown pressed); "Selection Level:" at row 331; Sub-Object button
  x 12–74 and level combo x 78–167, rows 344–364 (disabled for a base object; with Bend
  the combo shows "Gizmo"; with Edit Mesh at Vertex, Sub-Object is yellow).
- Parameters (title 378): the same layout as the Create panel's.
- Tab row with Modify selected: the body's top edge (row 28) starts at x 1; the join's
  second row (29) spans the selected tab from its left edge to one pixel short of its
  right (x 29–60 for Modify); with Create selected these are hidden by the tab.
- Verified: the Modify panel with a box, 58 differing pixels of 59,367, all in the
  disabled level combo's drop button (Wine draws its highlight white).
- Bend's Parameters (title-relative; verified, 0 differing pixels): group boxes x 17–163
  (shadow outline 17–162, white outline 18–163 whose right edge starts on the top row,
  white bottom from x 17), "Bend:" T+25…T+84, "Bend Axis:" T+97…T+135, "Limits"
  T+149…T+227, the top line cleared from x 24 to 55 / 77 / 53 around the label (text at
  x 27, 4 rows above the line); Angle and Direction spinners at T+36 and T+57 with the
  field at x 81; X / Y / Z radios at T+113, circles at x 40, 81, 120, labels 17 px after;
  Limit Effect at T+160 (box x 87); Upper and Lower Limit at T+180 and T+201 with the
  field at x 87; one decimal; content height 223. In the views, the gizmo is orange.
- Edit Mesh at Vertex level (measured): each vertex a 5-px diamond (rows of 1, 3, 5, 3, 1
  pixels) centred on it, `#FF0000` selected, `#0000FF` otherwise, red drawn last; the
  active Sub-Object button: the checked-button outline around `#F0F080`. A Selection
  rollout (five level icons, "4 Vertices Selected"), Soft Selection (collapsed), Edit
  Geometry (not yet built).
- Edit Mesh's rollouts (verified, 0 differing pixels): Selection (title 378, content 195)
  with five level icons 25 px apart from x 27 (the active one in a 1-px sunken frame,
  x 27–50, 23 rows), By Vertex / Ignore Backfacing / Ignore Visible Edges checkboxes at
  +54 / +74 / +93 (x 18), Planar Thresh (field x 99–143, arrows 146) at +110, Hide /
  Unhide All and Copy / Paste buttons (x 15–86, 92–163, 18 rows) at +131 and +170,
  "Named Selections:" and the right-aligned count; Soft Selection collapsed (595); Edit
  Geometry (615): button pairs at +23, +45, +68; unlabelled groups x 14–165 at +90…+157
  and +163…+233 (Extrude / Chamfer with fields x 92–145 and arrows 149; Normal radios at
  x 58 and 111; Slice Plane / Slice, Cut / Split, Refine Ends); the Weld group x 14–166
  from +243 (gap 21–49). Disabled controls: embossed text; spinner fields with a face
  interior and 2-px white margins; radios with a face interior. A checked box's tick:
  the classic 7 × 7 pattern, 3 px inside the box (grey when disabled).
- Modify container: bottom edge at row 884; content stops one row above it; an
  overflowing column leaves x 174–177 for a scroll indicator (grey track, black part;
  NOTE: not built).
- More… dialog (`r3.1-modifiers-dialog.png`): a 308 × 646 window titled "Modifiers", a
  list with the headings "WORLD-SPACE MODIFIERS" and "OBJECT-SPACE MODIFIERS" (world-space
  entries prefixed "* "), OK and Cancel at the top right; it lists the modifiers not on
  the button set.

## Transform display at the selection's pivot (measured, step 6.4a)
Screenshots `r3.1-box-created.png`, `r3.1-create-tripod-z.png`, `r3.1-move-selected.png`.
- Axis tripod (Select, creation): the object's own axes; lines from the pivot to 84 px,
  open arrowheads (two 8-px strokes), letters (viewport tripod glyphs, 9 rows above the
  tip); `#C82323` for axes in the active constraint, `#666666` otherwise; an axis at the
  viewer shows only its letter at the pivot.
- Move gizmo (Select and Move): world axes; shaft O + round(17·d) … O + round(60·d)
  (yellow `#FFFF00` when constrained, else the axis colour); filled head to 84.5 px
  (triangle fitted: base 62 px, half-width 3.75 px; X `#FF0000`, Y `#00FF00`, Z
  `#0000FF`); letter at O + round(84.5·d) (yellow when constrained); plane corners at
  O + round(84·(dₐ + d_b)), arms 17 px (yellow for the active plane, else each arm in the
  colour of the axis it runs beside); axes at the viewer not drawn.
- Rotate and Scale show the same figure as Move (R3.1, confirmed by the project owner).
- Prompt with Move: "Click and drag to select and move objects" (verified).

## Tab panel: Objects strip (measured, step 6.1)
Screenshot `r3.1-objects-tab.png`. The strip replaces the main toolbar with the same
geometry: 32-px buttons from x 4 (icons 24 × 24, 4 px in, rows 26–49 of the client area),
6-px separators with their etched line at +3/+4. 29 buttons: Box, Sphere, Cylinder,
Torus, Teapot, Cone, GeoSphere, Tube, Pyramid, Plane, Hedra, ChamferBox, OilTank,
ChamferCyl, Spindle, Gengon, RingWave, Torus Knot, Capsule, L-Ext, C-Ext, Prism | Quad
Patch, Tri Patch | Point Surface, CV Surface | Bones | Damper, Spring (names after the
first ten read from the icons, unverified). Tab row with Objects selected and the strip:
0 differing pixels of 81,384.

## Command panel (Phase 4)
The reference shows the Create panel: six tabs (Create, Modify, Hierarchy, Motion,
Display, Utilities); the Geometry category pressed among Geometry, Shapes, Lights,
Cameras, Helpers, Space Warps, Systems; the dropdown on `Standard Primitives`; the
Object Type rollout (disabled AutoGrid, buttons Box/Cone, Sphere/GeoSphere,
Cylinder/Tube, Torus/Pyramid, Teapot/Plane) and the Name and Color rollout. Every part is
measured below; the whole panel matches the reference (0 differing pixels of 143,519,
text and icons masked).

## Command panel: tab row (measured, step 4.1)
Code: `src/ui/CommandPanelLayout.*`, `src/ui/CommandPanel.*`. Coordinates relative to the
panel (184 px wide, client x 1736 at the reference size); outer frame: `Edge::CommandPanel`.

- Six tabs, 29 px wide, side by side from x = 2: Create 2–30, Modify 31–59, Hierarchy
  60–88, Motion 89–117, Display 118–146, Utilities 147–175.
- Unselected tab (L, R its edges): row 3 highlight L+2..R−2; row 4 highlight at L+1,
  light L+2..R−2, dark shadow at R−1; rows 5–27 highlight L, light L+1, face, shadow R−1,
  dark shadow R (2-px chamfered top corners).
- Selected tab: the same shape 2 px wider on each side, starting at row 1, drawn over
  its neighbours, its sides continuing to row 28; rows 28–29 between its sides are
  `#C0C0C0`, joining it to the body. (Only Create selected is in the reference; the look
  of other selected tabs follows the same rule, unverified.)
- Body: from row 28; top edge highlight (row 28) and light (row 29).
- Icons: 22 × 20 at (tab left + 3, 6), not moving when selected; original art in
  `res/panel_tab_icons.bmp` (`tools/make_icons.py`).
- Verified: frame + tabs 0 differing pixels of 7,722 (icons masked).

## Create panel: category row and dropdown (measured, step 4.2)
Code: `cmdpanel::CategoryBox`, `CommandPanel::PaintCategories`.

- Seven category buttons, 24 × 23, side by side from x = 9 (rows 37–59): Geometry,
  Shapes, Lights, Cameras, Helpers, Space Warps, Systems. Icons 16 × 16 at (button
  left + 4, 40); original art in `res/panel_category_icons.bmp`.
- Pressed button (the active category): 1-px shadow top and left, 2-px highlight right
  and bottom, a face-coloured inner line along its top and left, `#E1E1E1` inside; its
  icon 1 px down and right. Unpressed buttons: icon only (hover look unverified).
- Dropdown: a classic `COMBOBOX` (drop-down list), sunken frame x 21–163, rows 66–86
  (143 × 21); with classic controls Windows draws exactly R3's frame, 17 × 17 button and
  arrow. Lists per category: only "Standard Primitives" is in the reference, the rest
  from memory (unverified).
- Verified: panel rows 0–93, 0 differing pixels of 10,270 (icons and the dropdown's
  interior masked).
- Only on the Create tab.

## Command panel: rollup and rollouts (measured, step 4.3)
Code: `src/ui/Rollup.*` (layout, unit-tested), `CommandPanel::PaintRollouts`.

- Rollup container: 1-px sunken edge (shadow top and left, highlight right and bottom),
  x 4–178, rows 94–885; the shadow runs down to the bottom row.
- Rollouts stack from row 99 (container top + 5): each a **title button** x 11–168,
  16 rows tall (2-px highlight top and left, 2-px shadow bottom and right; "–" 4 px
  wide at x 17–20, 8 rows below the title's top; title text centred), over an **etched
  frame** x 7–172 from 8 rows below the title's top to the end of its contents. The
  frame is a shadow rectangle with a highlight rectangle 1 px down and right drawn over
  it, and its top edge stops 1 px short of the title button on each side.
- Next rollout: 7 rows below the previous frame's bottom (241 → 248).
- Object Type (Standard Primitives): disabled AutoGrid checkbox (13 × 13 at x 115,
  title + 19, classic look, text embossed, ending at x 103); ten buttons 69 × 18 in two
  columns at x 17 and 95, every 21 rows from title + 36 (1-px highlight top and left,
  black right and bottom with a shadow line inside). Content height 127.
- Name and Color: title rows 248–263, frame to row 299 (content height 36).
  - Name field x 15–146, rows 271–290 (title + 23 … + 42): 1-px sunken edge; inside it
    black top and left, face right and bottom; then white (one row on top, two columns
    each side); face inside while disabled (nothing selected). Enabled look: Phase 6.
  - Colour swatch x 150–166, same rows: 1-px sunken edge filled with the colour; the
    reference shows `#8D073A` (R3 picks the next object's colour at random). Clicking it
    opens Windows' colour dialog for now (R3's own "Object Color" dialog: later).
- While a primitive is active (measured, `r3.1-box-created.png`): the checked button has
  the pressed outline around light green `#C0FFC0`; AutoGrid is enabled; the name field
  is enabled (white, the new object's name); the swatch shows the new object's colour.
  Rollouts added: Creation Method (title row 306: two classic radio buttons at x 34 and
  102, title + 22), Keyboard Entry (collapsed: its frame title + 4 … + 13, "+" = a 6-px
  bar at x 17–22 on title + 8 and a 2-px upright x 19–20, title + 5 … + 11), Parameters
  (title 376). The container's top and bottom edges then stop at x 173 (a 4-px gutter).
- Spinner (measured): label right-aligned to x 87; field x 90–143 × 16 rows (sunken
  edge, black inner top/left, face inner right/bottom, white inside), value with three
  decimals; arrows x 146–157, up half rows 0–7 and down half rows 8–14 (highlight top and
  left, shadow right and bottom, dark right column and bottom row), 5-px triangles.
  Parameters rows: title + 24, + 45, + 66, gap, + 94, + 115, + 136, checkbox + 162; the
  frame ends 21 rows below the last row. Classic radio buttons are drawn from their
  measured pattern (Wine draws them differently).
- Keyboard Entry opened (NOTE: unverified, laid out by the Parameters rule): X, Y, Z at
  title + 24, + 45, + 66; the primitive's sizes from + 94; a Create button (69 × 18 at
  x 57) 5 rows below the last size row.
- Verified: whole panel 0 differing pixels of 143,519 (text, icons and the dropdown's
  interior masked); while creating a Box, 0 of 122,771 against R3.1's screenshot.
- Unverified: a checked button's look, the collapsed "+", spacing after a collapsed
  rollout, scrolling (hand drag, wheel step 30 px), when other categories show rollouts.
- With an object selected (R3.1 screenshots), Name and Color shows the object's name
  (e.g. `Cylinder01`) and its colour (Phase 6).

## Status area
MAXScript mini-listener (pink), prompt line 1 (`None Selected`), prompt line 2
(`Click or click-and-drag to select objects`), selection lock, X/Y/Z fields,
`Grid = 10.0`, Animate button, playback controls, key mode, frame field,
2×4 viewport navigation buttons.

## Custom controls
| Control | Behaviour | State |
|---|---|---|
| Rollout | Bevelled title bar with `-`/`+`, collapses; stacked in a rollup that scrolls by hand-drag | ✔ Phase 4 (`Rollup.*`; "+" and scrolling details unverified) |
| Spinner | Edit + tiny up/down arrows; drag to change, Ctrl accelerates, Alt slows, right-click resets (from memory) | ✔ Phase 5 (look measured; behaviour `SpinnerModel`) |
| Colour swatch | Sunken fill; click opens picker | ✔ Phase 4 (Windows' colour dialog stands in for R3's) |
| Check button | Looks pressed when on | ✔ Phase 4 (Object Type buttons; checked look unverified) |
| Flyout button | Corner triangle; hold to open strip of alternatives | ✔ Phase 1 (toolbar, status area) |
| Dropdown | Classic combo box | ✔ Phase 4 (real `COMBOBOX`) |
