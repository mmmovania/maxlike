# Phase 6 — Selection and transforms

Branch: `phase/6-select-transform`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 6.

## Steps
- [x] **6.1 Objects tab strip** (deferred from Phase 5): measured from R3.1's screenshot
      `r3.1-objects-tab.png`; 29 buttons in the main toolbar's geometry with original
      icons; the ten Standard Primitives start creation like their Create panel buttons
- [x] **6.2 Selection**: `Scene` selection, `SelectCommand`, `DeleteSelectionCommand`;
      picking (`view::Picking`: nearest edge in wireframe, nearest surface in shaded views,
      crossing regions); click / region / Ctrl adds / Alt removes; selected objects white,
      brackets in shaded views; "N Objects Selected"; Delete; new objects are selected
- [ ] 6.3 Name and Color for the selection (rename, recolour)
- [ ] 6.4 Move, Rotate, Scale with axis constraints
- [ ] 6.5 Wrap-up

## Step 6.1 verification
- [x] Builds warning-free (clean rebuild); twenty-one test programs pass (MinGW + Wine)
- [x] `test_toolbar_spec`: the strip's measured layout (buttons from x 4 every 32 px; the
      22nd at 676; separator lines at 711, 781, 851, 889; the last button at 924)
- [x] Under Wine, Objects tab selected, compared with R3.1: tab row and strip 0 differing
      pixels of 81,384 (tab labels and icons masked); the default window is unchanged
- [x] Clicking Box in the strip activates Box creation (green in the panel, pressed in
      the strip)
- [ ] On Windows: the same; the other buttons report "not implemented yet"
- [ ] Unverified: names after the ten Standard Primitives (read from the icons); whether
      R3 shows the active strip button pressed

## Step 6.2 verification
- [x] Builds warning-free (clean rebuild); twenty-two test programs pass (MinGW + Wine)
- [x] `test_picking`: edge distance, nearest edge within tolerance, surfaces by depth
      (either winding), crossing regions in any drag direction
- [x] `test_undo`: Select is undoable; Delete removes the selection in one step and undo
      restores it (hierarchy intact, selected); deleting drops nodes from the selection
- [x] `test_creation_tool`: a new object is selected
- [x] Under Wine: create (selected, white, "1 Object Selected"), click empty (None), click
      the edge (selected), Delete (gone), Undo (back, selected); a region selected 3 demo
      objects; brackets around a selected object in Perspective; default window unchanged
- [ ] On Windows: the same; Ctrl+click adds, Alt+click removes
- [ ] Unverified: pick tolerance (4 px), crossing as the default region mode, Ctrl/Alt
      rules, the selection rectangle's look (solid white here), bracket length (¼ edge,
      measured roughly), "N Objects Selected" plural

## Notes
- The strip is a second `Toolbar` (`ObjectsToolbarConfig`) shown in the main toolbar's
  place with the Objects tab; the other tabs' strips remain placeholders until their
  features exist (decision B).
