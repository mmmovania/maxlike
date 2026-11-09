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
| Objects | ✔ Phase 6, step 6.1 (planned for Phase 5; built when R3.1's screenshot of the tab arrived). Standard Primitives work; the other buttons wait for their features |
| Shapes, Compounds, Particles | when splines / compounds / particles exist (after Phase 8) |
| Lights & Cameras, Helpers, SpaceWarps | when those object types exist (after Phase 8) |
| Modifiers, Modeling | Planned for Phase 7, deferred: needs R3.1 screenshots of those tabs (the Modify panel's buttons cover the same modifiers meanwhile) |
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
- Objects tab strip of the tab panel → moved to Phase 6 (step 6.1): no screenshot of the
  tab existed during Phase 5.

## Phase 6 — Selection and transforms (`phase/6-select-transform`) ✔ v0.6.0
- [x] Objects tab strip (moved from Phase 5): 29 buttons in the main toolbar's geometry;
      the ten Standard Primitives start creation; pixel check 0 differing of 81,384.
- [x] Click and region selection (Ctrl adds, Alt removes); selected objects drawn white,
      brackets in shaded views; "N Objects Selected"; undoable (step 6.2).
- Selection filter (the toolbar's "All" combo) → deferred until objects other than
  geometry exist (with geometry only, every choice selects all or nothing).
- [x] Delete the selection (one undo step).
- [x] Name and Color for the selection: rename, recolour (undoable) (step 6.3).
- [x] Move/Rotate/Scale with the axis tripod and the Move gizmo (measured from R3.1), axis
      constraints, R3's default View coordinate system; one undo step per drag (6.4a–c).
- Reference coordinate systems other than View (World, Local, Screen, …) and transform
  centres other than each pivot → done in Phase 9 (9.9), which also corrected View in
  orthographic views (R3 uses the screen's axes there).
- [x] Status bar X/Y/Z display: the cursor's world position, and the movement, angle or
      scale percentage while dragging (step 6.4d). In R3.1 these fields are display-only
      (measured: plain sunken fields, no spinners); typing exact values belongs to the
      Transform Type-In dialog.
- [x] Transform Type-In dialog (Tools menu): typed position, rotation, scale → done in
      Phase 9 (9.8).

## Phase 7 — Modify panel (`phase/7-modify`) ✔ v0.7.0
- [x] Modifier stack UI: the Modify panel (name field, Modifiers, Modifier Stack with the
      stack list, on/off, remove, Sub-Object), measured against R3.1 (steps 7.2a–b).
- [x] Edit Mesh at Vertex level: More… dialog, sub-object selection and moves through
      undo, its Selection and Edit Geometry rollouts, measured (7.4–7.4c).
- [x] Bend, Taper, Twist: geometry (Bend verified against R3.1) and rollouts (Bend's
      measured) (7.1, 7.3).
- Deferred: Edit Mesh's other levels and button actions → done in Phase 9 for Edge, Face,
  Polygon, Element, Delete, Hide / Unhide All, Weld Selected / Target, Detach (9.4–9.7);
  still open (Phase 10): Create, Chamfer and the other buttons, Soft Selection's and
  Surface Properties' contents, the modifier gizmo (orange in R3), Rotate and Scale at
  sub-object level, the other modifiers in the More… list.

## Phase 8 — `.max` import (`phase/8-max-import`) ✔ v0.8.0
- [x] CFB reader, chunk walker, class directory, DLL directory (8.1–8.2). zlib deferred:
      no R3-era file is compressed.
- [x] ParamBlock / PB2, Derived Object, node controllers (quaternion conjugate) (8.3–8.4).
- [x] All seventeen samples import: R3.1's vertex/face counts, Max's own collapsed Box
      and teapot field by field, transforms, colours, instances (8.4–8.5); opened samples
      compared with R3.1 screenshots, with the saved viewports restored (8.6).
- [x] Native save in own chunked format (`.mlk`, 8.7).
- Deferred (into Phase 9's inventory): the perspective grid's extent rule; Bend / Taper /
  Twist and Backface Cull flags from files (no samples); materials; viewports and
  selection in `.mlk` files; keyboard shortcuts (done in 9.2).

## Phase 9 — Deferred items (`phase/9-deferred`) ✔ v0.9.0
Everything earlier phases deferred, inventoried in `docs/phases/phase-9-deferred.md` as
buildable now, needing R3.1 evidence, or separate subsystems; worked through in steps.
- [x] File > New, Reset, Merge; Edit > Select All / None / Invert; Views > Redraw All;
      the save prompt (9.1).
- [x] The menus checked against R3.1 item by item (eleven screenshots); keyboard
      shortcuts from the shortcut column; R3's greying; recent files (9.2).
- [x] Edit > Clone, File > Save Selected, File > Summary Info (9.3).
- [x] Edit Mesh at Face, Edge, Polygon and Element level; Delete, Hide / Unhide All, Weld
      Selected and Target, Detach (9.4–9.7).
- [x] Transform Type-In (F12) (9.8); reference coordinate systems and transform centres
      (9.9); Tools > Mirror and Array, with mirrored objects drawn correctly (9.10).
- [x] Wrap-up: `docs/tech/phase9-tech-docs.md`, documentation sweep, v0.9.0 (9.11).

## Phase 10 — Groups and the rest of list A (`phase/10-groups`)
From Phase 9's inventory, still buildable without new evidence:
- [x] The Group menu (Group, Ungroup, Open, Close, Explode, Detach, Attach): a hidden
      group helper its members are linked to; selection and picking through groups (10.1).
- [x] Tools > Align (10.2).
- [x] The modifier gizmo (10.3).
- [ ] Viewports and the selection in `.mlk` files.
- [ ] Edit Mesh: Create, Chamfer and the other Edit Geometry buttons; Rotate and Scale at
      sub-object level; Soft Selection.
- Waiting for R3.1 evidence (list B of the inventory): dialogs built from memory (New,
  Clone, Detach, Mirror, Array, Transform Type-In), Edit Mesh's look at sub-object levels,
  the Select By / Region / Grids submenus, the perspective grid's extent, Bend / Taper /
  Twist and Backface Cull in files, materials.
