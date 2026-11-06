# Phase 5 — Interactive creation

Branch: `phase/5-creation`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 5.

## Steps
- [x] **5.1 Creation logic**: `scene::Creator` (each primitive's mouse steps as a state
      machine, live parameters) and construction planes per view (`TopPlane`,
      `FrontPlane`, `LeftPlane`, `NodeTransform`); pure, unit-tested
- [x] **5.2 Creating in the viewports**: mouse to construction plane (ortho: the grid
      formula reversed; perspective: a ray to the ground), `scene::CreationTool` (live
      node, one undo step "Create" per object, Box01/Box02 names, swatch colour then a new
      random one, right-click or a click without drag cancels)
- [x] **5.3a Behaviour without the look**: `ui::SpinnerModel` (limits, arrow steps, drag
      with Ctrl/Alt, right-click reset and drag cancel, typed entry, display rounding);
      `CreationTool::CreateDirect` (Keyboard Entry's Create: typed values, one undo step)
- [x] **5.3b Creation rollouts, measured** (R3.1 screenshot `r3.1-box-created.png`):
      spinners, Parameters per primitive (`ui::params`), Creation Method radios, Keyboard
      Entry collapsed, the green checked button, AutoGrid and the name field enabled, the
      swatch showing the new object's colour; spinner edits become undo steps
- [ ] 5.4 Keyboard Entry contents (X, Y, Z and sizes, Create button)
- [ ] 5.5 Wrap-up: checks, `docs/tech/phase5-tech-docs.md`, version 0.5.0

## Step 5.1 verification
- [x] Builds warning-free; eighteen test programs pass (MinGW + Wine)
- [x] `test_creation`: Box (base drag, height by moving up/down, click), a click without a
      drag cancels, Sphere (one drag), Cone (three steps), Tube, Torus (tube radius from
      the ring), Plane, Pyramid; planes are orthonormal and right-handed; Front/Left
      objects face the viewer
- [ ] On Windows: `ctest` passes (no visible change yet)
- [ ] Unverified against R3.1: each primitive's exact steps; how radius 2 and the torus's
      tube radius follow the mouse; Cone radius 2 and Torus radius 2 before their steps

## Step 5.2 verification
- [x] Builds warning-free; nineteen test programs pass (MinGW + Wine)
- [x] `test_creation_tool`: live node while building, nothing undoable until finished,
      then exactly one "Create" step (undo/redo); names Box01, Box02; colours; cancel and
      click-without-drag leave nothing; other viewports ignored while one builds; Front
      plane orientation; all ten primitives create a real mesh
- [x] `test_ortho_grid`: pixel → world → pixel round trip
- [x] Under Wine: a box built in Top (shows in all views, swatch colour), a cylinder in
      Front (faces the viewer, next colour), Edit > Undo removes the cylinder
- [ ] On Windows: create each primitive in each view; undo/redo; right-click cancels
- [ ] Unverified: R3's creation cursor and prompts; R3's random colour palette; whether
      R3 leaves creation mode after an object (here it stays, ready for the next)

## Step 5.3a verification
- [x] Builds warning-free; twenty test programs pass (MinGW + Wine)
- [x] `test_spinner`: clamping, arrow steps, a drag as one change (total movement; Ctrl
      ×10, Alt ×0.1), right-click cancel during a drag, reset to 0 or the nearest limit,
      integer rounding, display ("3.25" shows "3.3", "-0.0" shows "0.0"), typed entry
      (spaces allowed, junk rejected, clamped)
- [x] `test_creation_tool`: Keyboard Entry creates Box01 with typed sizes at a typed
      position, oriented like the Front view, as one undo step
- [ ] On Windows: `ctest` passes (no visible change yet)
- [ ] Unverified: all spinner conventions above (from memory of R3)

## Step 5.3b verification
- [x] Builds warning-free (clean rebuild); twenty-one test programs pass (MinGW + Wine)
- [x] `test_rollup`: all five rollouts at the measured rows (titles 99, 248, 306, 356, 376;
      Keyboard Entry's collapsed frame 360–369; Parameters' frame to 559)
- [x] `test_parameters_spec`: Box's seven rows at the measured positions, content 168;
      every primitive's rows refer to real parameters
- [x] `test_creation_tool`: the swatch keeps the created object's colour; the next object
      gets a new one when started; a chosen colour is used as-is; the panel's target object
- [x] Under Wine, compared with R3.1's screenshot of the same state: the whole command
      panel while creating a Box, 0 differing pixels of 122,771 (text, icons, dropdown
      interior and the swatch's random colour masked); the default panel still 0 of 143,519
- [x] A spinner drag (Length +4.0 for 40 px) is one undo step; the toolbar's Undo restores
      both the box and the field
- [ ] On Windows: create each primitive; edit its Parameters by click, drag (Ctrl/Alt),
      right-click and typing; undo/redo
- [ ] Unverified: other primitives' Parameters and Creation Method lists (from memory);
      non-default creation methods (not implemented); the right-hand gutter's condition;
      spinner speeds; a pressed arrow's look

## Notes
- Steps are from memory of R3; the defaults (Creation Method: Box not Cube, Sphere from
  the centre, Pyramid from the base corner) are assumed until the screenshots arrive.
