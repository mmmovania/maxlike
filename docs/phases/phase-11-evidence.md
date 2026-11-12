# Phase 11 — List B: R3.1 evidence

Branch: `phase/11-evidence` (from `main` after Phase 10).

Phase 11 works through list B of Phase 9's inventory with screenshots and sample files
supplied from R3.1 (saved under `docs/reference/screenshots/r3.1-*.png` and
`docs/reference/samples/`): each item is compared with maxlike, corrected, and ticked off.

## Evidence received
- Batch 1: Edit Mesh at Edge, Face, Polygon and Element level (`r3.1-editmesh-*.png`),
  Soft Selection open on the teapot, a face deleted; the Clone, New Scene, Reset, save-prompt
  and Merge dialogs (`r3.1-dlg-*.png`).
- Batch 2: the Transform Type-In, Mirror, Array, Align, Group, Detach and Explode dialogs; an
  open group (`r3.1-group-open.png`).
- Batch 3: `bend.max`, `taper.max`, `twist.max`, `bend-gizmo.max`, `group.max`,
  `backface-off.max`, `material.max`; Bend's gizmo at Gizmo level.
- Batch 4: the Select By, Region and Grids submenus (`r3.1-menu-*.png`).

## Steps
- [x] **11.1** Edit Mesh: the count wording ("Edge 16 Selected", "2 Faces Selected"); the
      controls available at each level, measured (27 controls x 5 levels: all match);
      Divide and Bevel; hidden edges of selected faces dotted; the Delete question only
      when isolated vertices would remain; Soft Selection's measured layout with its
      falloff curve graph; Edge Distance's spinner follows its check box
- [ ] 11.2 The dialogs: Clone, New Scene, the save prompt, Merge's object list, Transform
      Type-In, Mirror, Array, Align, Detach and Explode names
- [ ] 11.3 Menus and displays: Region's check marks, the Grids items, the open group's
      colour, the gizmo's colours
- [ ] 11.4 Reading the new `.max` samples: Bend / Taper / Twist, gizmos, groups, Backface
      Cull, materials
- [ ] Wrap-up

## Still wanted from R3.1
- Edit Mesh with the panel scrolled to the bottom, at Edge and Face level.
- `bend.max` with the box selected in Modify, Sub-Object off (the object-level gizmo).
- Menus opened with Alt+letter (underlines); a far Perspective view; each primitive's
  Creation Method and Keyboard Entry rollouts.

## Step 11.1 verification
- [x] Builds warning-free; forty test programs pass (MinGW + Wine)
- [x] `test_parameters_spec`: the count wording for one item (an index from 1), several,
      none, and Element counting faces; the controls by level (Create not at Edge, Divide
      and Bevel labels, By Vertex, Ignore Visible Edges, Hide, Refine Ends).
      `test_edit_mesh`: Divide keeps the box closed (Euler 2); Bevel puts each corner 5
      further out than a plain Extrude, closed
- [x] Measured under Wine against `r3.1-editmesh-edge-a / face-a / polygon-a / element` and
      the Vertex-level screenshot: whether each of 27 controls is drawn available or greyed,
      at all five levels: no difference
- [x] Dotted hidden edges measured: one red pixel every 4 along the edge (r3.1-editmesh-face-a)
- [x] Soft Selection against `r3.1-editmesh-softsel-vertex.png`: check boxes and spinners
      exact (apart from the digits' font); the graph's box 93.8 % pixel-identical, its curve
      within 1 px of R3's on every row; the guide lines in the face colour
- [x] Found while measuring: the Phase 7 panel comparison masks all text, so it never
      checked which controls were greyed; the new enabled-state measurement does. A stale
      capture (rewritten later than the others) first showed a false difference; fresh
      captures of all five levels settle it
- [ ] On Windows: the five levels' buttons; Divide and Bevel; Soft Selection's graph
      following Falloff, Pinch and Bubble; the Delete question only when needed
- [ ] Unverified: the Delete question's appearance (inferred from its absence when deleting
      one teapot face); the curve's last rows near the baseline (up to 3 px apart); Bevel's
      R3 behaviour (by dragging; here Extrude's and Bevel's amounts)
