# Phase 2 — Technical documentation

Phase 2 replaced the grey placeholder with R3.1's four viewports: OpenGL rendering inside
Win32 windows, the measured frames, the home grid in all views, labels, axis tripods, the
viewport menu, resizing, and navigation. This document collects the explanations given
while building it, organised by topic. Measured values live in `docs/spec/01-ui-look.md`;
the step-by-step record is `docs/phases/phase-2-viewports.md`.

Code at the end of Phase 2: tag `phase-2-done` (version 0.2.0).

## Contents
1. Architecture and file map
2. OpenGL inside a Win32 window (2.1)
3. Shaders, buffers and GPU-object lifetime (2.2a)
4. Fitting R3's ortho grid (2.2a)
5. Fitting R3's perspective camera (2.2b)
6. 3D maths: from world point to pixel (2.2b)
7. Labels and tripods: pixels as data (2.3)
8. Viewport menu, view types, dividers, Min/Max (2.4)
9. Navigation (2.5)
10. The whole-window check (2.6)
11. Testing
12. Git lessons
13. Open items
14. Exercises

---

## 1. Architecture and file map

The same split as Phase 1: pure, unit-tested logic separate from Windows/OpenGL code.

| Layer | Files | Depends on |
|---|---|---|
| Maths | `src/math/Mat4.*` (`Vec3`, `Mat4`, `LookAt`, `PerspectiveHorizontal`, `PixelToClip`, `ProjectToPixel`) | nothing |
| View logic | `src/view/OrthoGrid.*`, `PerspectiveView.*`, `Tripod.*`, `Navigation.*` | maths |
| Layout | `src/ui/ViewportLayout.*` (frames, dividers, maximise) | nothing |
| OpenGL | `src/gl/GLContext.*` (context), `src/gl/LineRenderer.*` (shader + buffer) | GLAD |
| Windows | `src/ui/ViewPanel.*` (frames, dividers, context owner), `src/ui/Viewport.*` (one view), `src/ui/LabelText.*` (GDI text → pixels) | all of the above |
| Third party | `third_party/glad` (generated loader; README explains regeneration) | — |

Data flow for one paint of one viewport:
```
Viewport::Render → MakeCurrent → clear to #B0B0B0
   → grid lines (OrthoGridLines in pixels, or HomeGridLines in world space)
   → label + tripod (pixel lists) → [region rectangle] → SwapBuffers
```

---

## 2. OpenGL inside a Win32 window (step 2.1)

### Device context and pixel format
A window draws through its device context (HDC). Before OpenGL can use it, the DC needs
a **pixel format**: RGBA colour, 24-bit depth, 8-bit stencil, double buffering, described
in a `PIXELFORMATDESCRIPTOR`. `ChoosePixelFormat` returns the closest one the driver has;
`SetPixelFormat` applies it. A window's pixel format can be set **only once**.

### `CS_OWNDC`
Windows normally lends DCs from a shared pool and recycles them, losing the pixel
format. `CS_OWNDC` gives each viewport window a private DC for its whole life. Every
OpenGL window class needs it.

### Creating a modern context
The function that creates OpenGL 3.3 contexts is itself an extension, and extensions can
only be looked up while *some* context is current. Hence the dance in
`GLContext::CreateOn`:
1. `wglCreateContext` (old style) and make it current.
2. `gladLoadWGL` → `wglCreateContextAttribsARB` becomes available.
3. Create the real context: version 3.3, **core profile** (modern OpenGL only).
4. Delete the temporary context, make the real one current, `gladLoadGL`.

### Loading functions (GLAD)
`opengl32.dll` exports only OpenGL 1.1. Everything newer is looked up by name. `LoadProc`
handles a known quirk: `wglGetProcAddress` returns null (or 1, 2, 3, −1 on some drivers)
for 1.1 functions, which then come from `opengl32.dll` via `GetProcAddress`. GLAD is
generated code (`third_party/glad`, built as a small C library; CMake enables the C
language for it; the app links `opengl32`).

### One context, four windows
A context draws into any window with the same pixel format, one at a time. One
`GLContext` serves all viewports; each `WM_PAINT` does `MakeCurrent(dc)`,
`glViewport`, draw, `SwapBuffers(dc)`. Shared context = shaders and buffers created once.

### Double buffering
OpenGL draws into a hidden back buffer; `SwapBuffers` shows it at once (no half-drawn
frames). `WM_ERASEBKGND` returns 1 because OpenGL covers every pixel.

### The helper-window bug
Creating the context on the first viewport while it was still 1 × 1 px left that viewport
black under Wine (releasing the context afterwards did not help). Fix: create the context
on a hidden throw-away window (`"STATIC"` class), then destroy it; all viewports are then
equal. This is also the common pattern in OpenGL code on Windows.

### Failing gracefully
If no 3.3 core context is available, `GLContext::Error()` says why and each viewport
paints grey with GDI plus a message.

### Frames are GDI, around OpenGL
`ViewPanel` paints the frames with `DrawRing`; viewports are child windows covering only
their content. `WS_CLIPCHILDREN` (panel) and `WS_CLIPSIBLINGS` (viewports) keep GDI and
OpenGL from overwriting each other. Measured: 2-px border (white if active, black
otherwise), 1-px `#C0C0C0` ring, a 1-px line above the area; neighbours share the split
line; the active frame is drawn last so its white wins on the shared line.

---

## 3. Shaders, buffers and GPU-object lifetime (step 2.2a)

### Shaders
Modern OpenGL draws through **shaders**, small GPU programs compiled by the driver **at
runtime** (errors appear when the app runs, not when it builds):
- vertex shader: once per point; ours is `gl_Position = uMatrix * vec4(aPosition, 1.0)`;
- fragment shader: once per covered pixel; ours outputs one colour.

`Compile` checks `GL_COMPILE_STATUS` and keeps the driver's log; `Create` links the two
into a **program** and checks `GL_LINK_STATUS`. **Uniforms** (`uMatrix`, `uColor`) are
per-draw inputs, looked up by name once after linking.

### Buffers
- **VBO**: GPU memory holding the vertices; each draw uploads with `glBufferData`
  (`GL_STREAM_DRAW`; a few thousand vertices at most).
- **VAO**: remembers the layout ("attribute 0 = three floats per vertex"); set up once.
- `glDrawArrays(GL_LINES / GL_POINTS, …)`.

Minor, major and axis lines are three draws in that order, so darker lines are on top.

### GPU objects belong to the context
They can only be created or deleted while the context is current on some window. So the
renderer is created on the first paint, and released in the panel's `WM_DESTROY`, which
Windows sends to a parent **before** destroying its children (a viewport still exists).
A C++ destructor would run after the windows are gone. `LineRenderer` is non-copyable
(`= delete`): an earlier draft "reset" it by assignment, which copied handles instead of
deleting them.

### One shader for everything
The C++ side picks the matrix: `PixelToClip(w, h)` for anything given in pixels (ortho
grid, labels, tripods, region rectangle), `projection × view` for world-space lines.

---

## 4. Fitting R3's ortho grid (step 2.2a)

- The obvious model (400 units across, centred) put some lines a pixel off. Measuring
  all 180 line positions in the three ortho views and searching for a scale, centre and
  rounding rule found one formula that reproduces every one:
  - 400 world units span `W − 0.5` pixels across; the height shows `400·H/W` units over
    `H − 0.5` pixels (the two scales differ slightly: R3's own rasteriser);
  - column `floor((W−1)/2 + 0.745 + u·scaleX)`, row `floor((H−1)/2 + 0.22 − v·scaleY)`.
- The grid is computed **in pixels** (`OrthoGridLines`), not in world space: the rounding
  rule lives in testable code, and drawing each line through its pixel centre (`+ 0.5`),
  extended past both ends, gives the same pixels on every GPU.
- `test_ortho_grid` contains all measured lines. Result: Top, Front, Left identical to
  the reference.
- Zoomed far out, minor (then major) lines closer than 4 px are skipped; the axes always
  stay; indices are stepped by 10 to avoid millions of iterations (rule unverified).

### Reproducible size: `--client-size`
`maxlike.exe --client-size 1920x998` opens un-maximised with exactly the reference's
client size. Needed because maximising depends on screen, taskbar and title-bar height,
and Wine ignored attempts to shrink its virtual desktop.

---

## 5. Fitting R3's perspective camera (step 2.2b)

R3's default perspective camera is undocumented; the screenshot's grid determines it.
- Model the camera with unknowns; search for values that put every measured line where
  the screenshot shows it.
- **Degenerate fits:** the horizontal lines alone fit many cameras (wider lens closer in).
  The first fit chose a nonsense image centre; the second a 61° lens that put converging
  lines up to 22 px off. Always check a fit against data it was not fitted to.
- **Use structure:** the X axis lay on the centre row, so the camera aims at the origin
  (three unknowns instead of five).
- **Distrust messy measurements:** near the horizon converging lines merge, which broke
  the line numbering; only well-separated rows were used, with an outlier-tolerant fit.
- **Round numbers confirm:** distance 250, elevation 15°, FOV 45° (R3's documented
  default). Grid extent ±200 (its far edge is Y = 200).

Result: every line pixel within 1 px of R3's, both ways. Identical pixels are not
achievable for diagonal lines (two different rasterisers), so "within 1 px" is the right
measure.

---

## 6. 3D maths: from world point to pixel (step 2.2b)

```
world ──view──▶ eye space ──projection──▶ clip space ──÷w──▶ NDC (−1..1) ──▶ pixel
```
- **`LookAt(eye, target, up)`**: forward towards the target, right = forward × up,
  true up = right × forward. OpenGL's eye space looks down **−Z**. `up` = Z (Max is Z-up).
- **`PerspectiveHorizontal(fov, aspect, near, far)`**: Max specifies the field of view
  horizontally: horizontal scale `1/tan(fov/2)`, vertical = that × aspect (square cells).
  Depth is copied into `w`; the GPU divides by it (far things get smaller).
- **Orbit form:** the camera is distance, elevation and azimuth around a target
  (`PerspectiveView::Eye()`).
- Matrices are **column-major** (`m[column][row]`), OpenGL's layout, handed to
  `glUniformMatrix4fv` unchanged.
- `test_perspective` projects the 32 measured grid rows through the real code (each
  within 1 px) and checks `LookAt` and `PixelToClip` basics.

---

## 7. Labels and tripods: pixels as data (step 2.3)

### Points instead of textures
Labels and tripods are a few hundred pixels per view and must be exact, so they are
computed as **pixel lists** on the CPU and drawn as 1-px `GL_POINTS`. Exact, testable as
plain data; a texture-based text renderer will be better once there is much more text.

### The tripod rule (measured)
Origin 25 px from the bottom-left; each axis ends at `origin + round(24.5 × direction)`
(direction = how the world axis appears on screen; rounded in a y-up frame); a line
from origin to end; the letter's top-left 9 px above the end; drawn X, Y, Z (later
covers earlier, so the origin is blue). Glyphs: x 8×7, y 8×10, z 7×7.
`test_tripod` holds all four measured tripods as `R`/`G`/`B`/`.` grids.

### Bresenham's line algorithm
Integer-only stepping along the line with an error term; exact and identical everywhere.

### Screen directions of the world axes
Fixed tables for the ortho views; for the perspective view the view matrix's first two
rows (camera right and up): axis *a* appears as `(m[a][0], m[a][1])`. The tripod turns
with the camera without extra code.

### Text with GDI, read back
`RenderText`: a 32-bit **DIB section** (negative height = top-down rows), white text on
black with `NONANTIALIASED_QUALITY`, `GdiFlush()` before reading memory, collect lit
pixels; cached per DPI. Font: most likely MS Sans Serif Bold 10 pt (10-px capitals,
2-px strokes; verified on Windows by the project owner).

### Positioning by ink
The top-most lit row is placed at y = 4 (measured), so even a substitute font lands where
R3's does.

---

## 8. Viewport menu, view types, dividers, Min/Max (step 2.4)

### Context menu
Built fresh on each `WM_RBUTTONUP` over the label (so check marks are current):
`MF_CHECKED`, `MF_GRAYED` for features of later phases, `TPM_RETURNCMD` so
`TrackPopupMenu` returns the choice (handled in a `switch` right after).
`DestroyMenu` also destroys owned submenus. Shortcut texts are left out until
shortcuts exist (rule from Phase 1). Items reconstructed from memory.

### View types as data
Bottom, Back, Right added via the screen-direction table (Bottom: Y down; Back: X left;
Right: Y right). The tripod follows automatically.

### Dividers live in the gaps
The frame lines belong to `ViewPanel`, so it hit-tests (±3 px of the shared
column/row; both = crossing), sets cursors in `WM_SETCURSOR`, captures the mouse during
the drag, and calls `UpdateWindow` so frames follow the mouse immediately.

### Fractions, not pixels
`viewports::Arrangement` stores the dividers as fractions (0.5 default reproduces the
measured lines exactly) so proportions survive resizing; each viewport keeps ≥ 40 px.

### Maximise is a layout
The maximised viewport gets the whole area, the others empty rectangles; `Relayout`
shows/hides windows with `SWP_SHOWWINDOW`/`SWP_HIDEWINDOW` in the same batch.

### The `windows.h` macro trap
`IsMaximized` is `#define`d to `IsZoomed`; a method of that name was rewritten by the
preprocessor (renamed to `HasMaximizedView`). Others: `GetObject`, `CreateWindow`,
`SendMessage`, `DrawText`, `min`/`max`. An error about a function you never called →
suspect a macro.

---

## 9. Navigation (step 2.5)

### Modes and drags
A **mode** (`NavMode`, owned by `ViewPanel`) is what the left button does; a **drag** is
one press-move-release. Buttons *display* the mode (`onNavModeChanged` →
`Toolbar::SetChecked` + prompt text). Right-click leaves a mode; clicking the active
button again too.

### Incremental drags
Each `WM_MOUSEMOVE` applies the movement since the previous one (`dragLast_`);
`UpdateWindow` redraws at once.

### Pan
- Ortho: one pixel = `width / (W − 0.5)` units (the grid's scale), so a 50-px drag moves
  the grid exactly 50 px (tested).
- Perspective: one pixel at the target's depth = `2·d·tan(fov/2) / W` units; the target
  moves along the camera's right/up (view-matrix rows).

### Zoom multiplies
`exp(−dy × 0.01)` per drag, ×1.25 per wheel notch: the same feel at any scale, never
past zero, reversible (tested).

### Orbit
Azimuth and elevation change; elevation clamped to ±89°: at 90° forward equals up and
`LookAt`'s `forward × up` is zero (no "right" direction). Distance is unchanged (tested).

### Region zoom and Zoom Extents
`ZoomOrthoRegion` fits a pixel rectangle; Zoom Extents (empty scene) restores the
defaults. Middle-drag pans; Alt+middle orbits; Ctrl+Alt+middle zooms (unverified).

### The wheel goes to the focus
`WM_MOUSEWHEEL` goes to the focus window; a viewport calls `SetFocus` when clicked.
Deltas are in units of `WHEEL_DELTA` (120) per notch.

---

## 10. The whole-window check (step 2.6)

At `--client-size 1920x998` under Wine, compared with the reference client area:

| Region | Compared | Differing |
|---|---|---|
| Tab panel + main toolbar | 75,602 | 0 |
| Viewport frames | 30,138 | 0 |
| Top, Front, Left views | 1,089,324 | 0 |
| Perspective view | 362,826 | 41,129 (all line pixels within 1 px, both ways) |
| Command panel frame | 6,058 | 3 (its own tab row, Phase 4) |
| Time slider + track bar | 55,702 | 0 |
| Status area | 29,508 | 7 (icon art) |

Masked: label text (Wine has no MS Sans Serif), icon art, Wine-drawn combo/edit
interiors, command panel tabs and rollouts. Version → 0.2.0.

---

## 11. Testing

Eleven unit-test programs (CTest): `test_frame_layout`, `test_menu_spec`,
`test_tab_strip`, `test_toolbar_spec`, `test_time_slider`, `test_status_layout`,
`test_viewport_layout`, `test_ortho_grid`, `test_perspective`, `test_tripod`,
`test_navigation`. They hold measured data (grid lines, grid rows, tripods, frames) and
properties (round trips, pan accuracy, reversible zoom, orbit distance, clamps).

Pixel comparison: MinGW-w64 cross-compile, Wine with Mesa OpenGL on Xvfb, `xwd` capture,
Python/Pillow comparison: align, mask, normalise Wine's system colours, count, group by
cause. Notes: popups (menus, tooltips) render black on that display; Wine remembers a
virtual desktop's size by name; `--client-size` is the reliable way to get the
reference size.

---

## 12. Git lessons

| Situation | What to do |
|---|---|
| Two patches given, one may already be applied | Check `git log --oneline -1` and run **only** the matching alternative |
| `already exists in index` for every new file | The patch is already applied: `git am --abort`, then continue with build/run |
| `previous rebase directory … still exists` | A failed `git am` is pending: `git am --abort` first |
| Re-author several unpushed commits | `git rebase origin/<branch> --exec "git commit --amend --no-edit --reset-author"` |
| Downloaded patches show as untracked | `*.patch` in `.gitignore` |

(Phase 1's table in `phase1-tech-docs.md` still applies.)

---

## 13. Open items (unverified against R3.1)

- Viewport label menu: items, order, names (a screenshot of it open would settle it).
- Whether R3.1 lets you drag the viewport dividers.
- Whether enlarging a viewport (maximise, dividers) keeps the zoom (more grid) or the
  visible width (bigger cells); currently the width.
- Navigation: R3's own cursors, prompt texts, middle-button combinations, wheel step,
  drag directions, grid thinning rule, the region rectangle's look.
- Arc Rotate in ortho views (R3: turns them into User views) and Region Zoom in
  Perspective (R3: Field of View): not implemented yet.
- Grid spacing changes when zooming (R3 may adapt the spacing; not measured).

---

## 14. Exercises

| Step | Try |
|---|---|
| 2.1 | Clear to blue; remove `CS_OWNDC`; inspect `Description()` for your GPU. |
| 2.2a | Set `kOffsetX = 0.5` (`test_ortho_grid` lists moved lines); break a shader and read `Error()`; draw axes first. |
| 2.2b | `elevationDeg = 45`, `azimuthDeg = 30`, `fovDeg = 90`; remove `* aspect`. |
| 2.3 | `kOriginInset = 40`; reverse the tripod's axis order; rename a label. |
| 2.4 | Grey out Show Grid; `splitX = 0.3` (test fails); declare `IsMaximized()` and read the error. |
| 2.5 | Gentler wheel (`1.1`); slower orbit; `kMaxElevation = 90` (view breaks at the top); reverse the pan sign (test fails). |
