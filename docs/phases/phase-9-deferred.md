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
- [ ] Edit > Clone (copy / instance); File > Save Selected; File > Summary Info
- [ ] Views: Undo / Redo view change, Save / Restore Active View, Show Home Grid
- [ ] Edit Mesh: Face / Edge / Polygon / Element selection; Delete, Hide / Unhide All,
      Weld, Detach, Create; Rotate and Scale at vertex level; Soft Selection
- [ ] Transform Type-In (Tools menu; `scene::move::EulerXYZDegrees` exists since 6.4d)
- [ ] Reference coordinate systems other than View (World, Local, Screen) and transform
      centres other than each pivot
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
