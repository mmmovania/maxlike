# Phase 12 — Polish existing behaviour

Branch: `phase/12-polish` (from Phase 11's wrap-up). Before new features, fix what the
project owner found using the program on Windows.

## Reported
- Flicker while creating an object, and on the command panel when a modifier is applied.
- The transform gizmo does not respond to the mouse: no highlight over an axis, no
  constraint from it; the cursor does not change for Move (or Rotate, Scale).
- The gizmo is missing after an object is created.
- Right-click should end object creation.
- MSVC level-4 warnings (C4456 / C4457 hidden names, C4706 assignments in conditions,
  C4244 a narrowing), not reported by the MinGW build's -Wall -Wextra.
- During creation the other views lag behind the one drawn in; the axes appear only when
  the object is finished (R3 shows them from the first click).
- Separate gizmos for Move, Rotate and Scale.

## Steps
- [x] **12.1** Flicker-free painting (`win::PaintBuffer`: the command panel, status bar, tab
      panel, time slider, track bar, view panel and main window draw off screen and copy
      once); the Move gizmo's part under the mouse highlighted and taken by a drag
      (`view::GizmoPartAt`: an axis, or a plane in its corner); a press on the gizmo drags
      the selection even off the object; cursors for Select, Move, Rotate and Scale;
      right-click ends creation mode (a second right-click after cancelling an object
      being built)

- [x] **12.2** All MSVC warnings fixed in the code (hidden names renamed within their
      scope, assignments taken out of conditions, the narrowing made explicit); GCC now
      builds with -Wshadow=local, which reports what C4456 / C4457 do; every view repainted
      immediately during drags; the object being created is selected from the first click
- [ ] 12.3 Separate gizmos for Rotate and Scale

## Step 12.2 verification
- [x] 0 warnings with -Wall -Wextra -Wshadow=local (it also found hidden names MSVC does not
      report: lambda parameters, a local struct); no assignment inside a condition left in
      the tree (searched)
- [x] Forty test programs pass; default window unchanged; Edit Mesh panel 0 px from R3.1
- [x] Under Wine, with the mouse button still held while dragging out a box: the Top view
      shows its axes (281 tripod pixels), the Front and Perspective views already show it
- [x] Found: two of my own fixes needed fixing (a binding named after the function's
      parameter, then after a helper; an earlier build killed by the time limit left a
      truncated object file)
- [ ] On Windows: no MSVC warnings; the other views keep up while creating

## Step 12.1 verification
- [x] Builds warning-free; forty test programs pass (MinGW + Wine)
- [x] `test_transform_axes`: the part under the mouse for a Top view (X's shaft and
      arrowhead, Y, the XY corner; nothing between, in the gap at the pivot, beyond the
      tip, or for Z pointing at the viewer); every drawn shaft pixel hits its axis
- [x] Buffered painting changes nothing drawn: the default window pixel-identical, the
      Edit Mesh panel still 0 px from R3.1
- [x] Under Wine on `box.max`, Move: hovering X's shaft leaves X yellow and turns Y green
      (the toolbar's XY overridden); dragging from it 61 px right and 40 px down moves the
      box exactly 60 px right, 0 px down
- [x] Under Wine: Box, right-click in a view: the prompt returns to selection, the Box
      button is released, a drag then creates nothing
- [x] Found: the gizmo is drawn after creation when Move, Rotate or Scale is active; with
      Select (maxlike's tool at start) the tripod is shown, as measured from R3.1's
      `r3.1-box-created.png`. Asked the project owner which tool was active
- [x] Found: a shell parse error stopped a step before its edit ran, and the old test
      "passed"; the edit's presence is now checked (`grep -c`) before trusting a run
- [ ] On Windows: no flicker while creating or applying a modifier; the cursors; the
      gizmo's hover and drags in each view, including Perspective
- [ ] Unverified: R3's Rotate, Scale and Select cursors (drawn from memory); whether R3
      shows the gizmo after creation with Select active
