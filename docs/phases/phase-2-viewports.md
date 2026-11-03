# Phase 2 — Viewports

Branch: `phase/2-viewports`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 2.

## Steps
- [x] **2.1 OpenGL in a Win32 window**: `ViewPanel` with four `Viewport` child windows
      (`CS_OWNDC`), one shared OpenGL 3.3 core context (`gl::GLContext`, GLAD loader in
      `third_party/glad`), measured frames with the white active border; click to activate
- [ ] 2.2 Cameras and the grid (Top / Front / Left / Perspective, Z-up)
- [ ] 2.3 Labels and axis tripods
- [ ] 2.4 Viewport interaction: right-click label menu, Min/Max toggle, splitter dragging
- [ ] 2.5 Navigation: pan, zoom, orbit; the status-area navigation buttons
- [ ] 2.6 Pixel check and `docs/tech/phase2-tech-docs.md`

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

## Notes
- If OpenGL 3.3 core is unavailable (very old drivers, some remote-desktop sessions), the
  viewports show a message instead of failing.
