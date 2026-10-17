# Plan and acceptance criteria

Each phase lives on `phase/<n>-<name>` and merges into `main` when every criterion passes.

## Phase 0 — Skeleton (`phase/0-skeleton`)
- [ ] CMake project builds with MSVC (Debug/Release, Unicode, C++17).
- [ ] Thin wrapper: `Window` base class, message routing, `App` message loop.
- [ ] `Theme` struct with every colour and metric from `01-ui-look.md`.
- [ ] Per-monitor DPI awareness; metric scaling helper.
- [ ] Empty main window titled `Untitled - <app name>`.

## Phase 1 — Frame layout (`phase/1-frame-layout`)
- [ ] Menu bar with all R3 top-level menus.
- [ ] Tab panel switching button strips.
- [ ] Custom-drawn main toolbar with flyouts, check states and combos.
- [ ] Placeholders for viewport area, command panel, time slider, status area.
- [ ] Overlay test: matches the reference screenshot at 1920×1040.

## Phase 2 — Viewports (`phase/2-viewports`)
- [ ] Four GL child windows sharing one context; sunken frames and gaps.
- [ ] Top / Front / Left / Perspective cameras; Z-up.
- [ ] Infinite ortho grid; perspective home grid; minor/major/axis colours.
- [ ] Labels and axis tripods.
- [ ] Active viewport, right-click label menu (view type, shading, Show Grid).
- [ ] Splitter drag, Min/Max toggle, 8 navigation tools, MMB pan, wheel zoom.
- [ ] Overlay test: empty scene matches the reference screenshot.

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
