# Phase 5 — Interactive creation

Branch: `phase/5-creation`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 5.

## Steps
- [x] **5.1 Creation logic**: `scene::Creator` (each primitive's mouse steps as a state
      machine, live parameters) and construction planes per view (`TopPlane`,
      `FrontPlane`, `LeftPlane`, `NodeTransform`); pure, unit-tested
- [ ] 5.2 Creating in the viewports: mouse to construction plane (ortho and perspective),
      the object drawn live, one undo step per object, R3 names, colours, cancelling
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

## Notes
- Steps are from memory of R3; the defaults (Creation Method: Box not Cube, Sphere from
  the centre, Pyramid from the base corner) are assumed until the screenshots arrive.
