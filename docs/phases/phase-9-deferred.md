# Phase 9 — Deferred items

Branch: `phase/9-deferred` (from `main` after Phase 8).

Phase 9 works through everything earlier phases deferred. The inventory below was
collected from the repository itself: the plan's "Deferred" entries, every phase
checklist's unticked items and "Unverified" notes, the specs, the format notes, the code's
`NOTE:` markers, and the menu commands that still answer "not implemented yet" (105 of
113 at the start of the phase).

## A. Buildable now (no new R3.1 evidence needed)
- [x] **9.1** File > New, Reset, Merge (undoable, duplicate names renumbered); Edit > Select
      All / None / Invert (undoable); Views > Redraw All; asking to save a modified scene
      before New, Reset, Open, dropping a file, and closing; the Phase 7 checklist's stale
      duplicate line
- [x] **9.2** The menus checked against R3.1 item by item (eleven screenshots): labels,
      separators, ticked items; the shortcut column with working keyboard shortcuts (not
      while typing); R3's greying; recent files; Views > Show Transform Gizmo, Save /
      Restore Active View
- [x] **9.3** Edit > Clone (Copy / Instance, one undo step, clones selected); File > Save
      Selected; File > Summary Info (laid out after R3.1's dialog)
- [ ] Views: Undo / Redo view change, Save / Restore Active View, Show Home Grid
- [x] **9.4** Edit Mesh at Face level (click picks the front-most face, rectangles cross;
      Ignore Backfacing honoured; Move drags the faces' vertices) and Delete (faces, with
      or without isolated vertices; vertices with their faces), one undo step each; kept
      in `.mlk` files and clones
- [x] **9.5** Edit Mesh at Edge, Polygon and Element level: edges picked by distance
      (visible ones), polygons (faces joined across hidden edges) and elements (faces
      joined by vertices) selected as faces; Move and Delete at every level (an edge's
      Delete removes the faces on both sides)
- [x] **9.6** Edit Mesh: Hide / Unhide All (vertices' markers, or faces left out of the
      output) and Weld Selected (within the threshold, at the average; collapsed faces go);
      one undo step each; kept in `.mlk` files and clones
- [x] **9.7** Edit Mesh: Weld > Target (drag a vertex onto another within the pixels
      threshold; the target stays) and Detach (the selected faces as a new Editable Mesh in
      place, or as a clone), each one undo step
- [ ] Edit Mesh: Create, the other Edit Geometry buttons; Rotate and Scale at sub-object
      level; Soft Selection
- [x] **9.8** Transform Type-In (Tools menu, F12): a modeless floater, Absolute:World and
      Offset:World X / Y / Z for Move, Rotate (degrees) and Scale (percent); Enter applies
      the group typed in, one undo step; follows the tool and the selection
- [x] **9.9** Reference coordinate systems (View, Screen, World, Parent, Local, Grid; Pick
      not yet) for Move's constraints and drag planes, Rotate's axis and the gizmo; transform
      centres (pivot, selection centre, system origin) for Rotate and the gizmo. Corrects
      View in ortho views: R3 uses the screen's axes there (Front: Y is up the screen)
- [ ] The modifier gizmo in the views
- [ ] Viewports and the selection in `.mlk` files
- [ ] Group menu; Tools: Mirror, Array, Align
- [ ] Picking: whether culled (back-facing) edges can be clicked — R3.1's behaviour to check

## B. Need R3.1 evidence (screenshots or samples)
- ~~Keyboard shortcuts~~ (step 9.2, from the menu screenshots); still open: Edit > Select
  By and Region, Views > Grids submenu contents; underlined letters
- Dialogs: File > New's options, Reset's and the save prompt's wording, Summary Info,
  Transform Type-In, Object Color, Merge's object list and duplicate-name dialog
- The perspective home grid's extent for a distant camera; perspective shading; the ortho
  grid's subdivision threshold (4.9–18.5 px)
- Bend / Taper / Twist in a `.max` file (Phase 7's parameter indices); Backface Cull off
  (its node flag); materials
- Cursors, prompts, spinner speeds and conventions, the other primitives' Creation Method
  and Parameters rollouts, Keyboard Entry's layout (Phase 5 notes)
- Status bar details, time slider behaviour, toolbar hover look (Phase 1 notes)
- The Modifiers and Modeling tab strips; the More… dialog's layout; Edit Mesh's level
  icons; Soft Selection's and Surface Properties' contents
- Files over 7 MB (the DIFAT chain) and 4096-byte sectors (no sample)

## C. Separate subsystems (projects of their own)
The renderer (Render, Video Post, Environment, Effects, previews, RAM Player), Track View,
Schematic View, MAXScript, the Material Editor and Browser, Customize (UI, paths,
preferences, units, grid and snap settings), XRefs, Archive, the Shapes / Lights / Cameras
/ Helpers / Space Warps / Systems categories, zlib (only later Max versions compress).

## Step 9.9 verification
- [x] Builds warning-free; thirty-seven test programs pass (MinGW + Wine)
- [x] `test_move_math`: View in the Front view makes "Y" world Z and constrains to it; View
      in Perspective is World, Screen follows the camera; Local follows the object's
      rotation and pivot, Parent without a parent is World; rotation about a frame axis;
      a drag plane in the frame
- [x] Under Wine on `box.max`: Move, Y, a drag up in the Front view raises the box by the
      60 px dragged; the Top view is unchanged (before this step, "Y" there was world Y,
      into the screen)
- [ ] On Windows: each system with Move and Rotate in each view; the three centres with
      several objects; the gizmo's orientation in Local and Parent
- [ ] Unverified: R3 draws a gizmo per object with Use Pivot Point Center and several
      objects (one at the pivots' average here); Local / Parent with several objects (the
      first selected object's axes here); Scale in non-local systems (always the objects'
      own axes here); Pick; the Transform Type-In's offsets stay World

## Step 9.8 verification
- [x] Builds warning-free; thirty-seven test programs pass (MinGW + Wine)
- [x] `test_typein`: Euler angles round-trip; on parent_child_link's linked box, measured in
      the world: Absolute Move lands the pivot on the typed point, Absolute Rotate reads
      back as typed with the pivot still, the box's X axis turned 45°; Offset Move adds,
      Offset Rotate 90° about Z turns the pivot around the given point, Offset Scale 200
      doubles; computing a result leaves the node unchanged
- [x] Under Wine on `box.max`: Move tool, F12 opens "Move Transform Type-In"; Absolute X
      = 50 moves the box exactly 50 units (298 px at 5.97 px per unit); Ctrl+Z restores
- [x] Found: the new dialog header included the scene headers before <windows.h>, so
      windows.h's GetObject macro renamed Node::GetObject inconsistently; <windows.h> now
      comes first, as the project's convention requires
- [ ] On Windows: Rotate and Scale, offsets, several objects, Tab between fields
- [ ] Unverified: the floater's layout and its spinners (plain fields here), "Offset:World"
      vs R3's "Offset:Screen" in some views, Absolute Scale's frame (the object's own here),
      scaling a selection about a common centre

## Step 9.7 verification
- [x] Builds warning-free; thirty-six test programs pass (MinGW + Wine)
- [x] `test_edit_mesh`: Weld Target merges one vertex into another, which stays put, and
      cannot weld twice or into itself; Extract gives the top's two faces as a compact
      4-vertex mesh at the top's height with smoothing and material IDs, and leaves out
      faces already deleted
- [x] Under Wine on `box.max`: Detach of the top polygon gives 2 objects (Select All),
      undone to 1; Weld > Target, dragging a back top corner onto the other, merges them
      (the Perspective view shows the top collapsed to a triangle)
- [x] Found while testing: the mouse-up code for the weld drag had been inserted inside the
      `if (moving_)` block (the `case` has no brace of its own), so it never ran; found with
      temporary debug output along the chain, moved to the start of the case. Also: the
      Target button lies below the panel's edge until scrolled; the test collapses the
      Selection rollout instead (mouse-wheel delivery on the test display is unreliable)
- [ ] On Windows: Weld > Target (incl. the button reached by scrolling); Detach and
      Detach As Clone; undo
- [ ] Unverified: the Detach dialog (from memory), R3 showing the dragged vertex during
      Weld Target, the prompt for Target mode, whether a detached object keeps the
      original's parent (here: top level, same world placement)

## Step 9.6 verification
- [x] Builds warning-free; thirty-six test programs pass (MinGW + Wine)
- [x] `test_edit_mesh`: hiding the top's faces leaves 10, Unhide All 12, hidden vertices
      keep the mesh; welding two vertices 0.05 apart within 0.1 merges them at their
      average, the two faces using both collapse and go, the rest stay valid; within 0.01
      nothing; undo restores. `test_native_file`: hidden faces, vertices and welds survive
- [x] Under Wine on `box.max`, with the buttons: Polygon > Hide opens the box's top,
      Unhide All closes it; Vertex level, threshold 70, a corner's two vertices (60 apart)
      selected, Weld > Selected: "2 Vertices Selected" becomes 1
- [ ] On Windows: Hide / Unhide All at each level; Weld with several clusters; undo
- [ ] Unverified: R3 keeps hidden faces in the mesh, flagged (maxlike leaves them out of
      Edit Mesh's output; no difference without a renderer); Hide at Edge level; the
      messages for no vertices within the threshold; "1 Vertices Selected" wording

## Step 9.5 verification
- [x] Builds warning-free; thirty-six test programs pass (MinGW + Wine)
- [x] `test_edit_mesh`: every box side is a polygon of its two triangles, the box one
      element, two boxes in one mesh two elements; expansion of selections; an edge has two
      faces and deleting it deletes exactly those; segment picking (nearest within the
      tolerance, rectangles crossing)
- [x] Under Wine on `box.max`, clicks in the Top view: Face selects a triangle (diagonal
      drawn), Polygon the top square (outline only), Element the box, Edge the front edge
- [ ] On Windows: the four levels by click and region; Move and Delete at each
- [ ] Unverified: R3's look at Edge / Polygon / Element level (red outlines here), whether
      it shows hidden edges at Edge level, its counts' wording ("N Edges Selected"; faces
      counted at Polygon and Element), and its Delete at Edge level

## Step 9.4 verification
- [x] Builds warning-free; thirty-six test programs pass (MinGW + Wine)
- [x] `test_edit_mesh`: deleting a box's top faces leaves 10 with maps skipping them;
      isolated vertices only where no face remains; deleting a vertex takes its faces and
      renumbers, and a move recorded by input index still lands right; selection then
      delete undone in single steps; face picking prefers the nearer face and honours
      skipping; rectangles take faces with a vertex inside. `test_native_file` and
      `test_clone`: face selections and deletions survive saving, loading and copying
- [x] Under Wine on `box.max`: Edit Mesh, Face level, a click inside the box in the Top
      view selects the top face (front-most; drawn red); Delete asks, Yes removes it (the
      Perspective view shows the box open); Ctrl+Z restores face and selection
- [x] Found while testing: a fixed start-up wait captured a blank window after the display
      restarted; the script now waits for the window and for the box to be drawn
- [ ] On Windows: Face selection by click, region, Ctrl / Alt; Move at Face level; Delete
      both ways and at Vertex level; Ignore Backfacing
- [ ] Unverified: R3's face-level display (red triangle edges here), its "Delete isolated
      vertices?" question, the "N Faces Selected" wording, its rule for faces in a region,
      and how faces seen exactly edge-on are drawn

## Step 9.3 verification
- [x] Builds warning-free; thirty-five test programs pass (MinGW + Wine)
- [x] `test_clone`: a copied Edit Mesh-over-Box evaluates the same and is independent;
      cloning Sphere01 and its child gives Sphere02 with Box02 under it, both exactly on the
      originals; the child alone keeps the original parent; instances share; copying the
      seven instances gives seven boxes on one new object; undo / redo; Summary Info's
      totals equal R3.1's dialog for the seven samples it was shown with (Teapot01 (Teapot)
      530 / 1024, …); Save Selected keeps a child whose parent is left out where it was
- [x] Under Wine: one box selected, Edit > Clone, OK: Select All reports 8 objects; undo
      twice removes the clone; Select All reports 7
- [x] Investigated: after the Clone dialog, menu keys sent without a target window were
      lost — X kept its focus on the destroyed dialog (no window manager). Debug output
      showed the menu opened but no command arrived; keys sent to the main window work.
      A test-display artefact, not a maxlike fault (Windows returns focus on closing).
- [ ] On Windows: Clone (both options), Save Selected, Summary Info (and Save to File...)
- [ ] Unverified: the Clone Options dialog (no R3.1 screenshot; Controller options and
      Reference not offered), whether R3 keeps instance relations among copies, Summary
      Info's layout (converted, not pixel-checked), its Memory Usage figures and the
      Material column (R3's "default (Std)" shown for every object), Save to File's format

## Step 9.2 verification
- [x] Builds warning-free; thirty-four test programs pass (MinGW + Wine)
- [x] `test_menu_spec`: all 23 shortcuts parse to the keys the screenshots show, none
      twice; plain keys never fire while typing; in an empty scene exactly the 27 items
      R3.1 greys are greyed, across all eleven menus; with a selection the selection items
      come alive; recent files (newest first, case-insensitive, nine at most)
- [x] Under Wine: Select All, then Ctrl+Z (deselected) and Ctrl+A (selected again); with
      one box selected and the Name field focused, typing "Bm1x" types (M and 1 are not
      taken); without a focused field M opens the Material Editor (not implemented yet);
      opening a file records it as File1 in the registry
- [ ] On Windows: the menus next to the screenshots; the recent-files list; the shortcuts

## Step 9.1 verification
- [x] Builds warning-free; thirty-four test programs pass (MinGW + Wine)
- [x] `test_merge`: box + parent_child_link (Box01 renamed Box02, still under Sphere01,
      placed exactly as when opened alone); undo removes both merged nodes and leaves
      Box01 untouched, redo restores link, transforms and the same objects; instances
      merged twice: 14 boxes on two shared objects; a bad file changes nothing
- [x] Under Wine, through the real menus (Alt+E, A / U / I) on `instances.max`: Select
      All turns the seven boxes white ("7 Objects Selected"), Undo restores them, Select
      Invert selects all from none
- [ ] On Windows: New, Reset, Merge…, the save prompt (Yes / No / Cancel) on New, Reset,
      Open, dropping a file and closing
- [ ] Unverified: R3's New dialog (Keep Objects and Hierarchy / Keep Objects / New All;
      maxlike does New All), the wording of Reset's and the save prompt's questions, Merge's
      object list and duplicate-name dialog (maxlike merges everything, renumbering)
