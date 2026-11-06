# Plan and acceptance criteria

Each phase lives on `phase/<n>-<name>` and merges into `main` when every criterion passes.
Every phase ends with a technical document, `docs/tech/phase<n>-tech-docs.md`, collecting
the explanations, design reasons and lessons of that phase (index: `docs/tech/README.md`).

## Phase 0 — Skeleton (`phase/0-skeleton`) ✔
- [x] CMake project builds with MSVC (Debug/Release, Unicode, C++17).
- [x] Thin wrapper: `Window` base class, message routing, `App` message loop.
- [x] `Theme` struct with the colours and metrics known at the time (later phases moved
      measured metrics into `Metrics.h` and per-control code).
- [x] Per-monitor DPI awareness; metric scaling helper.
- [x] Empty main window titled `Untitled - <app name>`.

## Phase 1 — Frame layout (`phase/1-frame-layout`) ✔ v0.1.0
- [x] Menu bar with all R3 top-level menus.
- [x] Tab panel switching button strips (only the Main Toolbar strip has buttons; see
      "Tab panel strips" below).
- [x] Custom-drawn main toolbar with flyouts, check states and combos.
- [x] Time slider, track bar and the full status area (beyond the planned placeholders);
      placeholders for the viewport area and command panel.
- [x] Overlay test at 1920×1040: 0.137% of 171,929 compared pixels differ, all in parts
      that later phases replace (command panel rollouts) or in icon art.

## Tab panel strips (decision, Phase 1)
The other tab-panel strips are built together with the features they belong to, so each
arrives working rather than as empty buttons. Each needs a screenshot of that tab in R3.1.
| Tab | Built with |
|---|---|
| Objects | Phase 5 (creating primitives) |
| Shapes, Compounds, Particles | when splines / compounds / particles exist (after Phase 8) |
| Lights & Cameras, Helpers, SpaceWarps | when those object types exist (after Phase 8) |
| Modifiers, Modeling | Phase 7 (modify panel) |
| Rendering | when rendering exists (after Phase 8) |

## Phase 2 — Viewports (`phase/2-viewports`) ✔ v0.2.0
- [x] Four GL child windows sharing one context; measured frames (white active border).
- [x] Top / Front / Left / Perspective cameras (plus Bottom, Back, Right); Z-up. The
      perspective camera was fitted to the reference: 250 units, 15°, 45° FOV.
- [x] Infinite ortho grid (pixel-exact); perspective home grid (±200, within 1 px);
      minor/major/axis colours; thinning when zoomed far out.
- [x] Labels and axis tripods (tripods pixel-exact).
- [x] Active viewport, right-click label menu (view type, shading, Show Grid).
- [x] Divider drag, Min/Max toggle, navigation modes and buttons, middle-drag pan, wheel zoom.
- [x] Overlay test at 1920 × 998: Top, Front, Left, frames, all bars 0 differing pixels;
      Perspective lines all within 1 px; remaining differences only in icon art and
      the command panel's tab row (Phase 4).

## Phase 3 — Scene core (`phase/3-scene-core`) ✔ v0.3.0
- [x] Node (name, wire colour, parent, PRS, pivot offset, object ref, stable id).
- [x] ParamBlock; Object → modifier stack (Derived Object, Edit Mesh vertex moves) → Mesh
      (verts, faces, smoothing groups, material IDs, edge visibility).
- [x] All ten Standard Primitives; Box in Max's verified vertex/face order; counts of
      Cylinder, Cone, Tube, Torus, Sphere, GeoSphere, Teapot equal R3.1's Summary Info.
- [x] Scene drawn in the viewports: wireframe (node colour), Smooth + Highlights and the
      other shading modes.
- [x] Undo/redo (command pattern).
- [x] Unit tests hold the loader's verified results and R3.1's own counts (the JS
      loader's counts for most primitives turned out to be three.js's, not Max's).

## Phase 4 — Command panel (`phase/4-command-panel`) ✔ v0.4.0
- [x] Rollout, rollup, colour swatch, check buttons (spinners move to Phase 5: no
      spinner is visible in the reference; flyouts already exist in the toolbar).
- [x] Six tabs, category row, Standard Primitives combo, Object Type, Name and Color.
- [x] Pixel check of the Create panel against the reference: 0 differing pixels of
      143,519 (text and icons masked). The selected-object screenshots are zoomed and
      show the same panel; the enabled name field is checked in Phase 6.

## Phase 5 — Interactive creation (`phase/5-creation`) ✔ v0.5.0
- [x] Creation state machines for all ten Standard Primitives, through the undo system
      (one undo step per created object); construction planes per view.
- [x] Spinner control (measured from R3.1's screenshot `r3.1-box-created.png`).
- [x] Parameters rollout updates live while dragging; edits afterwards are undo steps.
- [x] Creation Method and Keyboard Entry rollouts (Keyboard Entry creates typed objects).
- [x] Pixel check: the panel while creating a Box, 0 differing pixels of 122,771.
- [ ] Objects tab strip of the tab panel: deferred until an R3.1 screenshot of the
      Objects tab exists (strips are built from measurements, decision B).

## Phase 6 — Selection and transforms (`phase/6-select-transform`)
- [ ] Click and region selection, selection filter; selected objects drawn white.
- [ ] Move/Rotate/Scale with tripod, axis constraints, reference coord systems.
- [ ] Status bar X/Y/Z type-in.

## Phase 7 — Modify panel (`phase/7-modify`)
- [ ] Modifier stack UI.
- [ ] Edit Mesh / Editable Mesh vertex and face levels.
- [ ] Bend, Taper, Twist.

## Phase 8 — `.max` import (`phase/8-max-import`)
- [ ] CFB reader, zlib, chunk walker, class directory, DLL directory.
- [ ] ParamBlock / PB2, Derived Object, node controllers (quaternion conjugate).
- [ ] All loader samples import with R3.1's vertex/face counts (`04-primitives.md`; the
      JS regression table's counts for most primitives are three.js's) and the verified
      transforms, colours and instances.
- [ ] Native save in own chunked format.
