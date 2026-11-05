# Phase 2 — Viewports

Branch: `phase/2-viewports`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 2.

## Steps
- [x] **2.1 OpenGL in a Win32 window**: `ViewPanel` with four `Viewport` child windows
      (`CS_OWNDC`), one shared OpenGL 3.3 core context (`gl::GLContext`, GLAD loader in
      `third_party/glad`), measured frames with the white active border; click to activate
- [x] **2.2a Ortho grid and first shader**: `view::OrthoGridLines` computes R3's grid in
      pixels (fitted to all 180 reference lines); `gl::LineRenderer` (GLSL 3.30 shader,
      VAO/VBO) draws it; `--client-size WxH` option for reproducible comparisons
- [x] **2.2b Perspective camera and grid**: R3's default camera fitted to the reference
      (target at the origin, distance 250, elevation 15°, 45° horizontal FOV); `math::Mat4`
      with `LookAt` / `PerspectiveHorizontal`; one matrix-based shader for both grids
- [x] **2.3 Labels and axis tripods**: tripods computed as exact pixels (`view::TripodPixels`,
      measured rule and glyphs); labels drawn by GDI into an off-screen bitmap and read back
      (`ui::RenderText`); both drawn as 1-px points
- [x] **2.4 Viewport interaction**: right-click label menu (shading, Show Grid, Views:
      Top/Bottom/Front/Back/Left/Right/Perspective); Min/Max toggle; dragging the dividers
      (`viewports::Arrangement`, unit-tested)
- [x] **2.5 Navigation**: modes from the status-area buttons (Zoom, Zoom All, Region Zoom,
      Pan, Arc Rotate; right-click leaves a mode), Zoom Extents (All), middle-drag pan,
      wheel zoom; pure maths in `view/Navigation.*` (unit-tested); the ortho grid thins out
      when zoomed far out
- [x] **2.6 Wrap-up**: whole-window pixel check, `docs/tech/phase2-tech-docs.md`,
      version 0.2.0

## Step 2.1 verification
- [x] Builds warning-free; seven test programs pass (MinGW + Wine)
- [x] Under Wine with Mesa: all four viewports cleared by OpenGL; clicking a viewport
      moves the white border
- [x] Pixel comparison of the frames (contents masked; top, split and bottom aligned
      separately): 0 differing pixels out of 24,330
- [x] Found and fixed: creating the context on the first viewport (then 1 × 1 px) left
      that viewport black under Wine; the context is now created on a hidden helper window
- [x] On Windows: four grey viewports with frames; clicking moves the white border;
      no "OpenGL 3.3 is not available" message

## Step 2.2a verification
- [x] Builds warning-free; eight test programs pass (MinGW + Wine)
- [x] `test_ortho_grid` contains every measured grid line of the three ortho views
- [x] Pixel comparison of Top, Front and Left at the reference size
      (`--client-size 1920x998`), labels masked: 94 differing pixels out of 1,083,564,
      all of them axis-tripod letters (step 2.3)
- [x] Found and fixed before it shipped: GPU-object cleanup (now `LineRenderer::Release`
      on the panel's `WM_DESTROY`, while the viewport windows still exist)
- [x] On Windows: the grids in Top, Front and Left look like R3's; Perspective is still
      plain grey

## Step 2.2b verification
- [x] Builds warning-free; nine test programs pass (MinGW + Wine); `test_perspective`
      projects all 32 measured grid rows to within 1 px
- [x] Perspective view at the reference size: every reference line pixel has one of ours
      within 1 px and vice versa (34,093 vs 34,097 line pixels); 88.6 % identical pixels,
      the rest are 1-px rasterisation differences on diagonal lines
- [x] Ortho views unchanged: 0 differing pixels (tripods masked)
- [x] On Windows: the perspective grid looks like R3's

## Step 2.3 verification
- [x] Builds warning-free; ten test programs pass (MinGW + Wine); `test_tripod` holds all
      four measured tripods (64 x 40 pixels each), identical
- [x] Top, Front, Left at the reference size, grid + tripod: 0 differing pixels out of
      1,089,324 (labels masked: Wine has no MS Sans Serif)
- [x] Perspective tripod: the same 128 coloured pixels as the reference
- [x] On Windows: labels "Top", "Front", "Left", "Perspective" look like R3's
      (font is a best guess: MS Sans Serif Bold 10 pt; compare letter shapes)

## Step 2.4 verification
- [x] Builds warning-free; ten test programs pass (MinGW + Wine)
- [x] Default layout unchanged: ortho views and frames still 0 differing pixels
- [x] Under Wine: dragging the vertical divider, Views > Bottom from the label menu (label
      and tripod change), Min/Max toggle on and off
- [x] On Windows: the same; cursors over the dividers (↔, ↕, ✥ at the crossing)
- [x] Unverified against R3.1: the label menu's items and order; whether R3.1 lets you
      drag the dividers; whether enlarging a view keeps the zoom or the visible width

## Step 2.5 verification
- [x] Builds warning-free; eleven test programs pass (MinGW + Wine)
- [x] Under Wine: middle-drag (+50, +30) in Top moves the axes by exactly (+50, +30); two
      wheel notches zoom x1.5625; Arc Rotate turns the perspective camera and its tripod;
      Zoom Extents All returns every view to the default (0 differing pixels again)
- [x] On Windows: each navigation button; the pressed button follows the mode; right-click
      leaves the mode; the prompt line explains the mode
- [ ] Unverified against R3.1: cursors (R3 has its own), prompt texts, middle-button
      combinations (Alt = orbit, Ctrl+Alt = zoom), wheel step, drag directions, grid
      thinning rule, Arc Rotate in ortho views (R3 turns them into User views; not yet),
      Region Zoom in Perspective (R3: Field-of-View; not yet), the region rectangle's look

## Step 2.6 verification (whole client area at 1920 × 998, Wine)
Masked: label text (no MS Sans Serif under Wine), icon art, Wine-drawn combo and edit
interiors, the command panel's tabs and rollouts (Phase 4). Wine's system greys normalised.

| Region | Compared | Differing |
|---|---|---|
| Tab panel + main toolbar | 75,602 | 0 |
| Viewport frames | 30,138 | 0 |
| Top, Front, Left views | 1,089,324 | 0 |
| Perspective view | 362,826 | 41,129 (every line pixel within 1 px, both ways) |
| Command panel frame | 6,058 | 3 (edge pixels of its own tab row, Phase 4) |
| Time slider + track bar | 55,702 | 0 |
| Status area | 29,508 | 7 (icon art) |

- [ ] On Windows: a final look with `--client-size 1920x998` next to the R3.1 screenshot

## Notes
- `maxlike.exe --client-size 1920x998` opens un-maximised with the reference's exact
  client size; use it for screenshots that are compared with the reference.
- R3 changes the grid spacing when zooming far out or in; not implemented yet (2.5).
- If OpenGL 3.3 core is unavailable (very old drivers, some remote-desktop sessions), the
  viewports show a message instead of failing.
