# Standard Primitives

How maxlike builds R3.1's ten Standard Primitives, and what is verified. Code:
`src/scene/Primitives.*`, `src/scene/TeapotData.h`; tests: `tests/test_primitives.cpp`.
Parameter indices are Max's (`docs/reference/max-format.md` §7.1); Phase 5 (creation)
and Phase 8 (import) rely on them.

✔ verified against R3.1 · ◐ partly · ○ unverified

## Rules that hold for every primitive
- Built in object space, Z up; the pivot sits at the base centre unless noted.
- **Nothing is optimised away.** Max keeps zero-size geometry: a cone with top radius 0
  keeps a full ring of vertices at the tip and a zero-size top cap; the teapot keeps the
  zero-area triangles at its knob and base. ✔ (R3.1 counts)
- Faces are triangles; a quad is two triangles `(c00, c10, c11)` and `(c11, c01, c00)`,
  each listing its two outer edges first and the hidden diagonal last (Max's Box face
  list). Wireframes show visible edges only. ✔ (Box) / ◐ (others)
- Rings of vertices start at +X and run counter-clockwise seen from above. ✔ (fitted to
  the cylinder's outline in an R3.1 screenshot) ◐ (direction)
- Caps of one segment are triangle fans whose spokes are hidden: only the rim shows. ✔
- An odd number of negative dimensions mirrors the object; faces are flipped so they
  still point outwards (a negative height builds downwards). ◐
- Vertex and face **order** is verified only for the 1 × 1 × 1 Box. It matters for
  imported Edit Mesh data (vertex moves by index). ○ for all other primitives.

## Counts (R3.1 File → Summary Info)

| Object (sample parameters) | Vertices / faces | Formula | Status |
|---|---|---|---|
| Box 60³, 1 × 1 × 1 segs | 8 / 12 | surface lattice points; 2 × 2 × (wl + wh + lh) | ✔ order and faces (JS); Summary Info ○ |
| Plane 40 × 42, 4 × 4 segs | 25 / 32 | (w+1)(l+1); 2wl | ○ |
| Pyramid 20 × 20 × 10, 1 seg | 5 / 6 | (from the JS reader's own construction) | ○ |
| Cylinder, 18 sides, 5 height segs, 1 cap seg | 110 / 216 | S(H+1) + 2; 2SH + 2S | ✔ |
| Cone r2 = 0, 24 sides, 5 height segs | 146 / 288 | same as the cylinder | ✔ |
| Tube, 18 sides, 5 height segs, 1 cap seg | 216 / 432 | 2S(H+1) + 2S(C−1); 4S(H+C) | ✔ |
| Torus, 24 segments × 12 sides | 288 / 576 | segs × sides; 2 × segs × sides | ✔ |
| Sphere, 32 segments | 482 / 960 | 2 + S(S/2 − 1); 2S + 2S(S/2 − 2) | ✔ |
| GeoSphere icosa, 4 segments | 162 / 320 | 10n² + 2; 20n² (tetra 2n²+2 / 4n², octa 4n²+2 / 8n²) | ✔ |
| Teapot radius 30, 4 segments | 530 / 1024 | 32 patches × n² × 2 faces; points merged per part | ✔ |

Screenshots: `docs/reference/screenshots/r3.1-summary-*.png`.

## Per primitive

### Box (class `0x10`)
0 length (Y), 1 width (X), 2 height (Z), 3 width segs, 4 length segs, 5 height segs.
Vertices: surface points of the (nx+1)(ny+1)(nz+1) lattice, layer by layer from the
bottom, row by row (y), then x: for 1 × 1 × 1 exactly Max's order ✔. Each side its own
smoothing group (bits 1–6 ○); material IDs ○ (0 for now).

### Plane (PB2)
0 length (Y), 1 width (X), 2 length segs ◐, 3 width segs ◐. Centred on the pivot, facing
+Z. Vertex order ○.

### Pyramid
0 width (X), 1 depth (Y), 2 height, 3 width segs, 4 depth segs, 5 height segs. Four
sides shrinking to the apex plus the base; cells collapsing at the apex become single
triangles (from the JS reader; ○ against R3.1).

### Cylinder (class `0x12`)
0 radius, 1 height, 2 height segs, 3 cap segs, 4 sides, 5 smooth. Rings up the height,
bands between them, a cap at each end. Defaults used: 5 height segs, 1 cap seg, 18 sides.

### Cone
0 radius 1 (base), 1 radius 2 (top), 2 height, 3 height segs, 4 cap segs, 5 sides,
6 smooth, 7–9 slice (○, not built). Negative radii count as 0 (Max clamps them; the
sample stores −20, shown as 0). Built like the cylinder with the radius interpolated.

### Tube
0 radius 1, 1 radius 2, 2 height, **3 sides, 4 cap segs, 5 height segs** (different from
Cone ✔), 6 smooth, 7–9 slice (○). The larger radius is the outside; the inner wall faces
the axis; annular caps.

### Torus (class `0x20` ○)
0 radius 1 (ring), 1 radius 2 (tube), 2 rotation, 3 twist (degrees assumed ○),
4 segments, 5 sides. A segments × sides grid wrapped around the ring and the tube.

### Sphere (class `0x11`)
0 radius, 1 segments, 2 smooth, 3 hemisphere (○, not built), 4 chop/squash,
5 base to pivot, 6–9 mapping/slice (○). Top pole, `segments/2 − 1` latitude rings,
bottom pole; fans at the poles (meridians visible).

### GeoSphere (PB2)
0 radius, 1 segments (frequency), 2 base type (0 tetra, 1 octa, 2 icosa), 3 hemisphere
(○), 4 smooth, 5 base to pivot. Each base triangle split into n² triangles, points pushed
onto the sphere and shared along base edges; a vertex on the +Z pole ✔ (JS). Rotation
about Z ○.

### Teapot
0 radius, 1 segments, 2 smooth, 3 unused (−1), 4 body, 5 handle, 6 spout, 7 lid.
The Utah teapot: 32 bicubic Bézier patches (Newell, with Blinn's proportions and Crow's
bottom), each evaluated on an n × n grid. Radius = body radius (data radius 2.0),
height = 1.575 × radius, spout towards +X ✔. Parts: Body = rim + body + bottom, Handle,
Spout, Lid; points are merged **within each part only** ✔ (merging all gives 529, as
the handle's end touches the body at one grid point). Predicted, to verify: Body 257 /
512, Handle 72 / 128, Spout 72 / 128, Lid 129 / 256 at 4 segments; 38 / 64, 138 / 256,
302 / 576 at 1, 2, 3 segments.

## Smoothing and shading
- Smoothing groups: a corner normal averages the faces around its vertex that share a
  bit with its face; group 0 is flat (`Mesh::CornerNormals`). Face weighting ○ (equal).
- Smooth parameter on: sides in group 1 (caps in their own groups); off: group 0.
