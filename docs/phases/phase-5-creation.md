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
- [ ] 5.3 Spinner and the Parameters rollout (needs the R3.1 screenshot after clicking Box)
- [ ] 5.4 Creation Method and Keyboard Entry rollouts (same screenshot)
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

## Notes
- Steps are from memory of R3; the defaults (Creation Method: Box not Cube, Sphere from
  the centre, Pyramid from the base corner) are assumed until the screenshots arrive.
