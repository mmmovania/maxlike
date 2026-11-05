# Phase 3 — Technical documentation

Phase 3 built the scene core: the data model behind everything maxlike will show and
edit. Nodes, transforms, objects with parameter blocks, the ten Standard Primitives,
the modifier stack, smoothing groups, shaded display, and undo/redo. This document
collects the explanations given while building it, organised by topic. The step record
is `docs/phases/phase-3-scene-core.md`; the `.max` format notes are in
`docs/reference/max-format.md`.

Code at the end of Phase 3: tag `phase-3-done` (version 0.3.0).

## Contents
1. Architecture and file map
2. Nodes, objects and ownership
3. Transforms and quaternions
4. Meshes as Max keeps them
5. The primitives, and how R3.1 corrected them
6. The modifier stack
7. Smoothing groups and shading normals
8. Drawing the scene: wireframes and lighting
9. Undo and redo
10. Testing
11. Open items
12. Exercises

---

## 1. Architecture and file map

The scene core is pure C++ (no Windows, no OpenGL) and fully unit-tested; the viewports
read it.

| Area | Files |
|---|---|
| Maths | `src/math/Quat.*` (quaternions, `Translation`, `Scaling`, `TransformPoint`, `AffineInverse`) |
| Model | `src/scene/Mesh.*`, `ParamBlock.*`, `Object.h`, `Node.*`, `Scene.*` |
| Objects | `src/scene/Primitives.*`, `TeapotData.h` (Utah teapot patches, three.js MIT), `Modifier.*` |
| Undo | `src/scene/Undo.*` |
| Demo | `src/scene/DemoScene.*`, `--demo-scene <name>|all|instances` |
| Drawing | `src/ui/Viewport.cpp` (`DrawScene`, `DrawShaded`), `src/gl/MeshRenderer.*`, `view::OrthoWorldToClip` |
| Tests | `test_scene`, `test_primitives`, `test_modifiers`, `test_undo` (+ `test_ortho_grid` extended) |

`MainFrame` owns the `scene::Scene` and the `scene::UndoStack`; the viewports get a
read-only pointer to the scene.

---

## 2. Nodes, objects and ownership

- A **node** is *where* something is: name, wire colour, parent/children, transform,
  object offset. An **object** is *what* it is (a Box with these dimensions). A node
  points at an object through `std::shared_ptr`, so several nodes can share one object:
  Max's **instances** (`instances.max`: seven nodes, one object).
- The `Scene` **owns** nodes through `std::unique_ptr`; parent/child links are plain
  observing pointers. Only the `Scene` may change links (`friend class Scene`), so the
  invariants (a parent lists its children; no cycles) live in one place.
- **Naming:** `UniqueName("Box")` gives the next free two-digit number (Box01, Box02, …),
  reusing freed numbers, as R3 does.
- **Stable ids:** every node has an `Id` assigned once and never reused; `RestoreNode`
  recreates a node with its old id. This exists for undo (section 9).

---

## 3. Transforms and quaternions

- A node's transform relative to its parent is a **PRS**, applied scale, then rotation,
  then translation: `Translation × Rotation × Scale` (column vectors, read right to left).
  World matrix = parent's world matrix × local.
- **Object offset** (Affect Pivot Only): `ObjectMatrix = WorldMatrix × offset`.
- **Quaternions** store rotations: no drift when composing, smooth interpolation, no
  gimbal lock. maxlike uses the standard right-handed convention; `.max` files store the
  **conjugate** (verified in the JS project), so `Quat::FromMaxFile` is the one place
  that converts. Tested with real numbers from `instances.max` (−41.5°).
- **Linking keeps world placement:** `Link` remembers the child's world matrix, changes
  the parent, and solves `local = inverse(parentWorld) × world`, then decomposes it into
  PRS. A mirrored parent (negative determinant) cannot be represented by a rotation; the
  flip moves into the X scale. Deleting a node re-links its children the same way.

---

## 4. Meshes as Max keeps them

`scene::Mesh` holds shared vertices and triangles (`Face`), each with:
- a **smoothing-group** bitmask (shading, section 7);
- a **material ID** (Multi/Sub-Object, later);
- **edge visibility** (3 bits): which edges show in wireframe. A Box side is two
  triangles drawn as a quad: the diagonal is hidden. `VisibleEdges()` returns each
  visible edge once.

Parameters live in a `ParamBlock` addressed **by index**, as in Max (Box: 0 length,
1 width, 2 height, …); each has a type (float, int, bool) that `Set` respects; unknown
indices are ignored (useful for importing).

---

## 5. The primitives, and how R3.1 corrected them

### Building blocks
- **Grid cells (`AddCell`)**: Max's Box face list (verified) shows one rule for every
  side: triangles `(c00, c10, c11)` and `(c11, c01, c00)`, outer edges first, diagonal
  last and hidden. A table of six (corner, U, V) entries builds all box sides; the test
  reproduces the verified 8 vertices and 12 faces exactly.
- **Lattice vertices**: a segmented box keeps only the surface points of an integer
  lattice; walking layer by layer, row by row, gives Max's order 0–7 for a 1×1×1 box.
- **Rings (`Ring`, `Band`, `Cap`)**: Cylinder, Cone, Tube (and the Sphere's latitudes)
  are rings of vertices around Z, bands of quads between rings, and caps (fans or
  annuli).
- **Mirroring**: an odd number of negative dimensions flips every face; edge-visibility
  bits are remapped with the swap (edge k runs v[k] → v[k+1]).

### What R3.1's numbers taught us
The JS loader's counts for most primitives came from **three.js** generators, not from
Max. R3.1's File → Summary Info gave ground truth:

| Object | R3.1 vertices / faces | What it showed |
|---|---|---|
| Cylinder (18 × 5) | 110 / 216 | ring structure confirmed |
| Cone (r2 = 0, 24 × 5) | 146 / 288 | the tip keeps a full ring of 24 vertices and a zero-size top cap (my collapsed tip was wrong) |
| Tube (18 × 5 × 1) | 216 / 432 | confirmed |
| Torus (24 × 12) | 288 / 576 | confirmed |
| Sphere (32) | 482 / 960 | 2 poles + 15 rings of 32 |
| GeoSphere (icosa, 4) | 162 / 320 | 20 × 4² faces |
| Teapot (r 30, 4) | 530 / 1024 | every triangle kept; points merged **per part** (merging all gives 529: the handle's end touches the body) |

Screenshots added: cap fans hide their spokes; rings start at +X (a regular 18-gon fitted
to the cylinder's outline: 0.76 px rms, vertex at 0°).

Lessons: Max does not "optimise" degenerate geometry, and imported Edit Mesh data
addresses vertices by index, so matching Max's counts and structure matters more than
tidiness. Tests now carry R3.1's numbers with their source.

### Teapot
32 bicubic Bézier patches (Newell's data with Blinn's proportions and Crow's bottom),
evaluated on an n × n grid with Bernstein weights; Radius = body radius (data radius
2.0), height 3.15 × radius / 2. Parts: Body = rim + body + bottom, Handle, Spout, Lid.

---

## 6. The modifier stack

- `DerivedObject` = base object + modifiers; `Evaluate()` evaluates the base and applies
  enabled modifiers from the bottom up. The base stays live (edit the box under an
  Edit Mesh and the moves still apply).
- `EditMeshModifier` stores vertex moves **by index** plus the vertex count it expects;
  if the mesh below has a different count, the indices would point elsewhere, so it
  skips itself and the Derived Object records it in `Skipped()`.
- Instances share the whole Derived Object; each node keeps its own transform.

---

## 7. Smoothing groups and shading normals

`Mesh::CornerNormals()` gives one normal per **face corner**: the average of the unit
normals of faces around that vertex that share at least one smoothing bit with the face;
a face with no bits is flat; zero-area faces contribute nothing. Per corner, not per
vertex, because a box corner belongs to three sides with different normals. Results
(tested): box sides flat, sphere radial, twisted box sides smooth across their hidden
diagonals (both halves share the side's group), as in `instances.max`.

---

## 8. Drawing the scene: wireframes and lighting

### Wireframes
Visible edges only, all of them (no hidden-line removal), in the node's wire colour.
Ortho views map world points through the grid's own `ColumnF`/`RowF` and snap to pixel
centres, so edges lying on grid lines land on the grid's pixels (the demo box's outline
in Top is exactly the predicted 516-pixel rectangle). OpenGL leaves out a line's last
pixel (diamond-exit rule), so only the end is extended by half a pixel.

### Shaded display
- `gl::MeshRenderer`: a second shader pair; vertices uploaded **per corner** (position +
  normal, six floats), because corner normals differ.
- **Depth buffer**: `GL_DEPTH_TEST` with `GL_LESS` during shaded drawing; cleared every
  frame. Grid first without depth test, labels last.
- **Normals transform by the inverse transpose** of the object matrix (correct under
  non-uniform scale).
- **Lighting**: ambient 25 % + diffuse `max(n·l, 0)` × 75 % in the object colour, plus a
  white Blinn-Phong highlight `pow(n·h, 40)` in the Highlights modes; two-sided
  (`gl_FrontFacing`); one light above, left of and behind the viewer. Unverified against
  R3.
- Modes from the label menu: Smooth + Highlights (Perspective's default), Smooth, Facets
  + Highlights, Facets, Wireframe, Bounding Box. Ortho views shade through
  `OrthoWorldToClip`, built from the same pixel formula (tested).

---

## 9. Undo and redo

- **Command pattern**: each change is a `Command` with `Do`, `Undo`, `Name`; the
  `UndoStack` executes and records it. Undo moves it to a redo list; a new command
  clears redo.
- **Ids, not pointers**: undoing a delete creates a new `Node` at a new address; commands
  store ids and use `FindById`/`RestoreNode`.
- **Exactness**: `DeleteNodeCommand` snapshots the node (name, object, colour, transform,
  offset, parent) and each child's **exact** local transform, so undo restores them bit
  for bit instead of re-solving them.
- Commands: create, delete, transform, parameter (affects every instance), link/unlink,
  rename, wire colour; `MacroCommand` makes several changes one step.
- **Levels** (20 assumed, R3's default from memory) forget the oldest steps; **saved
  state** (`MarkClean`/`IsClean`) survives undo/redo and is lost when a new branch
  replaces it.
- Edit menu: `WM_INITMENUPOPUP` rewrites "Undo <name>" / "Redo <name>" and greys them;
  until Phase 5 nothing in the UI creates commands, so they stay grey.

---

## 10. Testing

Fifteen test programs. Phase 3 added `test_scene`, `test_primitives`, `test_modifiers`,
`test_undo`. Useful patterns:
- **Shape properties that hold whatever the numbering**: watertightness (every directed
  edge appears once and its reverse once), faces outwards (for convex shapes, from the
  bounding-box centre; for revolved shapes, from the axis; for open shapes such as the
  teapot's parts, a positive signed volume).
- **Counts by formula**, derived independently (a segmented box's 104 visible edges).
- **Ground truth with its source** in a comment (R3.1 Summary Info, verified JS results).
- A placeholder check (`|| true`) was caught in review: a test that cannot fail proves
  nothing.

---

## 11. Open items (unverified against R3.1)

- Vertex and face order inside meshes, except the 1×1×1 Box (needed for imported Edit
  Mesh data on other primitives).
- Smoothing-group bits per side and material IDs (all 0 for now).
- Summary Info of Box, Plane, Pyramid (the Pyramid's 6 triangles come from the JS code).
- Teapot per-part and other-segment counts (predictions in the phase doc).
- Sphere hemisphere/slice, Cone/Tube slice, Torus rotation/twist units.
- How Max weights faces when averaging normals.
- Viewport lighting (direction, ambient, highlight); Lit Wireframes drawn as wireframe.
- Undo levels default and Edit-menu wording.
- Where the sample files place their objects.

---

## 12. Exercises

| Step | Try |
|---|---|
| 3.1 | Swap the PRS matrix order (tests fail); remove the mirror fix; R3 naming with a freed number. |
| 3.2 | Segment the demo box; make every edge visible (`0x7`); remove the end extension (lost corner pixels). |
| 3.3 | Hide/show cap spokes; drop a cap (watertightness fails); make a frustum. |
| 3.4 | Coarse sphere (8 segments: 26 vertices); other geodesic bases; collapse the cone tip (R3.1 counts fail); teapot without handle and spout; merge all teapot points (529). |
| 3.5 | Twist 60°; disable the Edit Mesh; change height segments under it (skipped). |
| 3.6 | Move the light; change the highlight exponent; disable the depth test (the tube). |
| 3.7 | Remove the exact child-transform restore (test fails); fewer undo levels. |
