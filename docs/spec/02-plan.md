# Plan and acceptance criteria

Each phase lives on `phase/<n>-<name>` and merges into `main` when every criterion passes.
Every phase ends with a technical document, `docs/tech/phase<n>-tech-docs.md`, collecting
the explanations, design reasons and lessons of that phase (index: `docs/tech/README.md`).

## Phase 0 — Skeleton (`phase/0-skeleton`)
- [ ] CMake project builds with MSVC (Debug/Release, Unicode, C++17).
- [ ] Thin wrapper: `Window` base class, message routing, `App` message loop.
- [ ] `Theme` struct with every colour and metric from `01-ui-look.md`.
- [ ] Per-monitor DPI awareness; metric scaling helper.
- [ ] Empty main window titled `Untitled - <app name>`.

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

## Phase 3 — Scene core (`phase/3-scene-core`)
- [ ] Node (name, wire colour, parent, PRS, pivot offset, object ref).
- [ ] ParamBlock; Object → modifier stack → Mesh (verts, faces, smoothing, mat IDs).
- [ ] Primitive builders ported from the JS loader with Max vertex order.
- [ ] Undo/redo.
- [ ] Unit tests reproduce the loader's regression table (box 12 tris, sphere 960, …).

## Phase 4 — Command panel (`phase/4-command-panel`)
- [ ] Rollout, rollup, spinner, colour swatch, check button, flyout controls.
- [ ] Six tabs, category row, Standard Primitives combo, Object Type, Name and Color.

## Phase 5 — Interactive creation (`phase/5-creation`)
- [ ] Creation state machines for all ten Standard Primitives.
- [ ] Parameters rollout updates live while dragging.

## Phase 6 — Selection and transforms (`phase/6-select-transform`)
- [ ] Click and region selection, selection filter.
- [ ] Move/Rotate/Scale with tripod, axis constraints, reference coord systems.
- [ ] Status bar X/Y/Z type-in.

## Phase 7 — Modify panel (`phase/7-modify`)
- [ ] Modifier stack UI.
- [ ] Edit Mesh / Editable Mesh vertex and face levels.
- [ ] Bend, Taper, Twist.

## Phase 8 — `.max` import (`phase/8-max-import`)
- [ ] CFB reader, zlib, chunk walker, class directory, DLL directory.
- [ ] ParamBlock / PB2, Derived Object, node controllers (quaternion conjugate).
- [ ] All loader samples pass the regression table.
- [ ] Native save in own chunked format.
