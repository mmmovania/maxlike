# Phase 12 — Polish existing behaviour

Branch: `phase/12-polish` (from Phase 11's wrap-up). Before new features, fix what the
project owner found using the program on Windows.

## Reported
- Flicker while creating an object, and on the command panel when a modifier is applied.
- The transform gizmo does not respond to the mouse: no highlight over an axis, no
  constraint from it; the cursor does not change for Move (or Rotate, Scale).
- The gizmo is missing after an object is created.
- Right-click should end object creation.

## Steps
- [x] **12.1** Flicker-free painting (`win::PaintBuffer`: the command panel, status bar, tab
      panel, time slider, track bar, view panel and main window draw off screen and copy
      once); the Move gizmo's part under the mouse highlighted and taken by a drag
      (`view::GizmoPartAt`: an axis, or a plane in its corner); a press on the gizmo drags
      the selection even off the object; cursors for Select, Move, Rotate and Scale;
      right-click ends creation mode (a second right-click after cancelling an object
      being built)

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
