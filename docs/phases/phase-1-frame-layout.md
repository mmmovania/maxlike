# Phase 1 — Frame layout

Branch: `phase/1-frame-layout`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 1.

## Steps
- [x] **1.1 Layout skeleton**: `ComputeFrameLayout` (pure, unit-tested), seven placeholder
      region windows, bevel helpers, font handed to children with `WM_SETFONT`
- [ ] 1.2 Menu bar
- [ ] 1.3 Tab panel
- [ ] 1.4 Main toolbar (buttons, flyouts, combos)
- [ ] 1.5 Time slider, track bar, status area
- [ ] 1.6 Overlay check against the reference screenshot

## Step 1.1 verification
- [x] Builds warning-free; `ctest` passes (checked with MinGW + Wine)
- [x] On Windows: regions appear as labelled placeholders, sizes at 1920×1080 maximised
      (before the menu exists, the client is 19 px taller than the reference, so only
      the viewport height differs: 877 instead of 858)

## Notes
- Measurements are client-relative at 100%: client top = bottom of the menu bar
  (screen y 42 in the reference), client size 1920×998.
- Viewport borders are more intricate than the spec's "two rings": between viewports the
  reference shows `C0 | FF FF | 00 C0`, the outer top is `A0 FF FF C0`, the bottom
  `C0 00 00`. Re-measure per viewport in Phase 2.
- The command panel edge is exactly `DrawEdge(EDGE_RAISED)` with system colours
  (`FFFFFF/E3E3E3` top-left, `A0A0A0/696969` bottom-right).
- Inactive tabs in the tab panel are `#A0A0B3` (for step 1.3).
