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
- [x] **11.2a** Dialogs that change behaviour: New Scene (Keep Objects and Hierarchy / Keep
      Objects / New All), Merge's object list (only the chosen objects merged), Clone laid
      out from measurements with its (greyed) Controller group, the save prompt's wording,
      Detach's "Mesh01" and Explode's "Mesh"
- [x] **11.2b** Transform Type-In (offsets in the view's screen axes, arrows), Mirror (R3's
      layout, Mirror IK Limits), Align (positions along the reference system, Apply), Array
      (R3's layout: Totals, Re-Orient, Uniform, Total in Array, Reset All Parameters); the
      system's name as R3 shows it (View: Screen in orthographic views)
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

## Step 11.2b verification
- [x] Builds warning-free; forty test programs pass (MinGW + Wine)
- [x] `test_arrange`: aligning X pivots in a frame turned 90 degrees moves along world Y
      only; Totals of 100 units over 5 copies is 25 a step, 160 % over 3 its square root;
      Re-Orient off places the copy as with it on but keeps its orientation.
      `test_typein`: a 10-unit Move offset in that frame goes along world Y; 90 degrees
      about its X turns world X to -Z
- [x] Under Wine on `box.max`, the box selected in the Front view: Type-In Offset Y = 10
      raises it exactly 60 px (10 units) in the Front view, the Top view unchanged; Ctrl+Z
      restores it. The floater measures 288 x 101, R3.1's client area within a pixel
- [ ] On Windows: each dialog next to its screenshot; Array's arrows and Totals; Align's
      Apply; the Type-In arrows
- [ ] Unverified / not built: R3's spinners dragging (here a click steps by 1); Mirror IK
      Limits has no effect (no IK); Reference cloning (no references); Array's Re-Orient
      off keeps scale too; Array Totals for Scale as a root per step; Align's live preview

## Step 11.2a verification
- [x] Builds warning-free; forty test programs pass (MinGW + Wine)
- [x] `test_merge`: `instances.max` lists its 7 boxes; choosing the 1st and 3rd merges
      exactly Box01 and Box03; `parent_child_link.max`'s child alone comes in at the top,
      exactly where it was; choosing all equals a plain Merge
- [x] Under Wine: `instances.max`, Select All, Group, File > New (the save prompt answered
      No), Keep Objects: Select All finds the 7 boxes, the document is Untitled. The New
      Scene dialog measures 255 x 153 px, exactly R3.1's client size (r3.1-dlg-new.png)
- [ ] On Windows: each dialog next to its screenshot; Merge's name filter, Select Subtree
      and List types; Keep Objects and Hierarchy
- [ ] Unverified: R3's Keep Objects and Hierarchy beyond keeping the scene (it also drops
      animation, which maxlike lacks); Merge's Sort (greyed, as in the screenshot) and its
      duplicate-name question; the dialogs are laid out from measured pixels converted to
      dialog units, not compared pixel by pixel (the test display does not paint them)

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
