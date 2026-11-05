# Phase 3 — Scene core

Branch: `phase/3-scene-core`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 3.

## Steps
- [x] **3.1 Scene model**: `math::Quat` and affine helpers; `scene::Mesh` (faces with
      smoothing groups, material IDs, edge visibility), `ParamBlock`, `Object` /
      `MeshObject`, `Node` (PRS transform, object offset, hierarchy), `Scene` (ownership,
      R3 naming, linking that keeps world placement); reference `max_loader.js` and
      `max-format.md` added to `docs/reference/`
- [x] **3.2 Box, Plane, Pyramid; scene in the viewports**: `BoxObject` (Max's verified
      order for 1 x 1 x 1, one rule for all sides), `PlaneObject`, `PyramidObject`; wireframes
      in node colour, ortho vertices through the grid's pixel formula; `--demo-scene`
- [x] **3.3 Cylinder, Cone, Tube, Torus**: built from rings (`Ring`, `Band`, `Cap`);
      collapsed rings become triangles; Max's parameter orders (Tube differs from Cone)
- [x] **3.4a Sphere, GeoSphere; ring primitives corrected to R3.1**: counts now exactly
      Max's (Summary Info); the cone keeps its tip ring and top cap; cap spokes hidden; rings
      start at +X (fitted to the cylinder outline)
- [ ] 3.4b Teapot (waiting for R3.1 Summary Info of teapot.max and its Segments value)
- [x] **3.5 Modifier stack**: `DerivedObject` (base + modifiers, bottom up),
      `EditMeshModifier` (vertex moves by index, skipped if the vertex count changed),
      `Mesh::CornerNormals` (smoothing groups); `--demo-scene instances` (illustrative)
- [x] **3.6 Shaded display**: `gl::MeshRenderer` (lit triangles, depth test); Smooth +
      Highlights (Perspective's default, as in R3), Smooth, Facets + Highlights, Facets,
      Bounding Box from the viewport menu; ortho views use `OrthoWorldToClip`, consistent
      with the grid's pixel formula
- [x] **3.7 Undo/redo**: command pattern (`scene::Command`, `UndoStack` with levels and
      saved-state tracking); nodes get stable ids; commands for create, delete (exact
      hierarchy restore), transform, parameter, link, rename, colour, and macro steps;
      Edit > Undo/Redo and the toolbar buttons, menu text "Undo <name>", greyed when empty
- [ ] 3.8 Wrap-up: pixel checks, `docs/tech/phase3-tech-docs.md`, version 0.3.0

## Step 3.1 verification
- [x] Builds warning-free; twelve test programs pass (MinGW + Wine)
- [x] `test_scene`: right-handed quaternions; the `.max` rotation rule with real numbers
      from `instances.max` (stored (0, 0, 0.3543, 0.9351) gives −41.5° about Z); transform
      order; linking and deleting keep world placement (also under a mirrored parent);
      object offset; R3 naming (Box01, Box02, reuse of freed numbers); instances share
      one object; mesh bounds, winding and hidden edges; parameter blocks
- [ ] On Windows: `ctest` passes (no visible change in the app yet)

## Step 3.2 verification
- [x] Builds warning-free; thirteen test programs pass (MinGW + Wine)
- [x] `test_primitives`: Box 60³ has Max's exact 8 vertices and 12 faces (max-format.md
      §7.2), outward normals, 12 visible edges, one smoothing group per side; segmented box
      (104 visible edges by formula); negative height builds downwards; Plane 40 x 42, 4 x 4:
      32 triangles, 25 vertices; Pyramid 20 x 20 x 10: 6 triangles
- [x] Under Wine: without a scene the views are still identical to the reference; the demo
      box's outline in Top is exactly the predicted rectangle (columns 367–496, rows
      146–275, 516 pixels)
- [ ] On Windows: `--demo-scene box`, `plane`, `pyramid`, `all` look right
- [ ] Pixel comparison with R3.1 wireframe screenshots of box.max, plane.max, pyramid.max
      (all four views in Wireframe, default zoom); this also settles where the samples
      put their objects
- [ ] Unverified: Max's vertex order for segmented boxes, Plane and Pyramid; smoothing-group
      bits per side; material IDs (all 0 for now)

## Step 3.3 verification
- [x] Builds warning-free; thirteen test programs pass (MinGW + Wine)
- [x] `test_primitives`: every primitive is watertight (each edge shared by exactly two
      faces, wound oppositely) and faces outwards; counts by formula; tube.max (10 / 6.185,
      h 10, 18 sides, 5 height segs, 1 cap seg) gives 432 triangles; the cone's tip and a
      negative height are handled
- [ ] On Windows: `--demo-scene cylinder|cone|tube|torus|all` look right
- [x] Max's true counts (R3.1 File > Summary Info): the cone differed (now fixed, step 3.4a)
- [x] Cap spokes hidden and rings starting at +X (R3.1 screenshots, step 3.4a)
- [ ] Vertex order within the meshes; torus rotation/twist units

## Step 3.4a verification
- [x] Builds warning-free; thirteen test programs pass (MinGW + Wine)
- [x] `test_primitives` holds R3.1's Summary Info counts (vertices / faces): Cylinder
      110 / 216, Cone 146 / 288, Tube 216 / 432, Torus 288 / 576, Sphere 482 / 960,
      GeoSphere 162 / 320; all watertight and outward-facing; the cylinder shows no cap spokes
- [ ] On Windows: `--demo-scene sphere|geosphere|all`
- [ ] R3.1 Summary Info still wanted for: Box, Plane, Pyramid, Teapot

## Step 3.5 verification
- [x] Builds warning-free; fourteen test programs pass (MinGW + Wine)
- [x] `test_modifiers`: the instances.max situation (one Box + Edit Mesh turning vertices 4–7
      by +25°, shared by seven nodes; Box01 −41.5°, Box07 +47.0°); the base can still be
      edited under the modifier; a changed vertex count skips the Edit Mesh; stack order and
      disabling; smoothing: box sides flat, sphere radial, the twisted sides smooth across
      their diagonals, group 0 flat
- [ ] On Windows: `--demo-scene instances`
- [ ] Unverified: how Max weights faces when averaging normals (equal weights for now)

## Step 3.6 verification
- [x] Builds warning-free; fourteen test programs pass (MinGW + Wine);
      `test_ortho_grid` checks the ortho world-to-clip matrix against the pixel formula
- [x] Under Wine: the empty scene is still identical to the reference; the demo scenes
      shade correctly (sphere highlight, flat cylinder caps, the tube's inner wall, the
      twisted boxes smooth across their diagonals)
- [ ] On Windows: shading modes from the viewport label menu in every view
- [ ] Unverified: R3's viewport lighting (light direction, ambient, highlight size) —
      an R3.1 screenshot of a shaded object would let us tune it; Lit Wireframes is drawn
      as plain wireframe for now

## Step 3.7 verification
- [x] Builds warning-free; fifteen test programs pass (MinGW + Wine)
- [x] `test_undo`: create/undo/redo keeps the id, name, object (instances), colour and
      transform; deleting a node with a parent and a child and undoing restores the
      hierarchy and every local transform exactly; transform, parameter (all instances),
      link, rename, colour each undo and redo; a macro is one step; 3 levels forget the
      oldest steps; a new command clears redo; the saved state is tracked through undo,
      redo, and lost when a new branch replaces it
- [ ] On Windows: Edit > Undo / Redo are greyed (nothing in the UI creates commands until
      Phase 5); `ctest` passes
- [ ] Unverified: R3's undo levels default (20 assumed) and menu wording ("Undo Create")

## Notes
- Rotations are standard right-handed quaternions inside maxlike; `.max` files store the
  conjugate, so the importer (Phase 8) uses `Quat::FromMaxFile`.
- Wireframes draw only visible edges (`Face::edgeVisible`), as Max does: a quad's
  diagonal is hidden.
- In Max's Box face list every face names its two outer edges first and the hidden
  diagonal last; the same cell rule (c00, c10, c11) / (c11, c01, c00) builds every side.
- OpenGL leaves out a line's last pixel; ortho edges are snapped to pixel centres and
  only their end is extended by half a pixel.
- `maxlike.exe --demo-scene <name>|all` opens with the sample objects (box, plane,
  pyramid, cylinder, cone, tube, torus).
- The JS reader's triangle counts for primitives other than Box and Pyramid are
  three.js's, not Max's (e.g. cone 264 = 24 x (2 x 5 + 1) with degenerate tip triangles);
  maxlike builds the cone's tip as one row of triangles: 240.
