# Phase 3 — Scene core

Branch: `phase/3-scene-core`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 3.

## Steps
- [x] **3.1 Scene model**: `math::Quat` and affine helpers; `scene::Mesh` (faces with
      smoothing groups, material IDs, edge visibility), `ParamBlock`, `Object` /
      `MeshObject`, `Node` (PRS transform, object offset, hierarchy), `Scene` (ownership,
      R3 naming, linking that keeps world placement); reference `max_loader.js` and
      `max-format.md` added to `docs/reference/`
- [ ] 3.2 Box, Plane, Pyramid in Max's vertex order; scene drawn in the viewports
      (wireframe, node colour); `--demo-scene`
- [ ] 3.3 Cylinder, Cone, Tube, Torus
- [ ] 3.4 Sphere, GeoSphere, Teapot
- [ ] 3.5 Modifier stack: Derived Object, Edit Mesh vertex moves, smoothing groups
- [ ] 3.6 Shaded display (Smooth + Highlights)
- [ ] 3.7 Undo/redo
- [ ] 3.8 Wrap-up: pixel checks, `docs/tech/phase3-tech-docs.md`, version 0.3.0

## Step 3.1 verification
- [x] Builds warning-free; twelve test programs pass (MinGW + Wine)
- [x] `test_scene`: right-handed quaternions; the `.max` rotation rule with real numbers
      from `instances.max` (stored (0, 0, 0.3543, 0.9351) gives −41.5° about Z); transform
      order; linking and deleting keep world placement (also under a mirrored parent);
      object offset; R3 naming (Box01, Box02, reuse of freed numbers); instances share
      one object; mesh bounds, winding and hidden edges; parameter blocks
- [ ] On Windows: `ctest` passes (no visible change in the app yet)

## Notes
- Rotations are standard right-handed quaternions inside maxlike; `.max` files store the
  conjugate, so the importer (Phase 8) uses `Quat::FromMaxFile`.
- Wireframes draw only visible edges (`Face::edgeVisible`), as Max does: a quad's
  diagonal is hidden.
