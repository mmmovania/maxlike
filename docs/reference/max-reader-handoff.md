# Handoff: pure-JS 3ds Max `.max` reader + three.js viewer

Paste this file (and attach the deliverables listed below) at the start of a new chat to resume.

## Goal

Read Autodesk 3ds Max `.max` scene files in pure JavaScript, with no Max installation, and display them with three.js. The format is undocumented; we are reverse-engineering it one sample file at a time.

## Deliverables so far

| File | What it is | State |
|---|---|---|
| `max-reader.html` | Single-file viewer: drag-drop a .max, scene outline, File structure tab (streams, class directory, DLLs, reader notes), wireframe/smooth/grid toggles, click-to-select. Uses three.js **r128** UMD scripts (cdnjs `three.min.js`, jsDelivr `three@0.128.0/examples/js/controls/OrbitControls.js` and `.../geometries/TeapotGeometry.js`). | Working, published as artifact `https://claude.ai/artifact/1NMTsFHJLV5pki3TPgzGdC` |
| `max_loader.js` | Reusable ES-module `MaxLoader extends THREE.Loader` for three.js **r150+**: `loadAsync(url)` / `parseAsync(arrayBuffer)` → `{ scene, nodes, classes, dlls, streams, log, chunkCount }`. Options: `zUp`, `materialFactory`. | Tested in Node with three r160 on all samples |
| `max-format.md` | Format reference with ✔ verified / ◐ partial / ○ unverified markers and an "open questions" table | Current as of this handoff |

The viewer and the loader share the same parsing code (sections 1–3 of the viewer's script were copied into the loader). **The viewer still has its own inline copy.** If parsing changes, update both, or switch the viewer to import the loader (offered, not done yet).

## How the user works

- Uses **3ds Max R3.1**. Sends one `.max` sample plus a Max screenshot (four viewports and the parameter panel) per feature.
- Note: the panel can show creation defaults or UI-clamped values. Always cross-check geometry against the **viewport** too (e.g. Cone stored radius2 = −20 while the panel showed 0).
- Reports issues with a screenshot of the viewer.
- Prefers incremental, verified progress.

## Verification method (keep using it)

1. Extract the core parser from the viewer: the script text between the `1.` and `4.` section banners.
2. Wrap it as a CommonJS module with `const THREE=require('three')` (three **0.128.0** npm, plus `TeapotGeometry.js` from `three@0.128.0/examples/js/geometries/` loaded with `global.THREE`).
3. Dump chunks and parameters for the new sample, decode, implement, then rebuild the mesh and assert bounds, triangle counts, normals and winding.
4. Run a **regression** over all samples: box, sphere, geosphere, cone, tube, pyramid, teapot, plane, instances.
5. Edit the published artifact with Artifact `action: "edit"` (url above), and run `node --check` on the extracted script after each edit.
6. For the loader: an ESM test with three **0.160.0** that parses all samples, checks parallel parses, shared instance geometry and the sRGB wire colour.

## Verified results (expected regression output)

| Sample | Result |
|---|---|
| box | Box 60×60×60, 12 tris, base at z=0, wire `#1AB194` |
| sphere | r 10, 32 segs, 960 tris, smooth, `#860606` |
| geosphere | r 10, icosa, 4 segs → 320 tris, vertex on +Z pole, `#99E4B8` |
| cone | r1 10, r2 clamped 0 (stored −20), h 20, 264 tris, `#57E0C6` |
| tube | ro 10, ri 6.185, h 10, 18 sides, 5 hsegs, 432 tris, `#1AB158` |
| pyramid | 20×20×10, 6 tris, `#390888` |
| teapot | radius 30 = body radius, height 47.25, spout +X, 992 tris, `#C6E057` |
| plane | 40 (X) × 42 (Y), 4×4 segs, 32 tris, `#B1581A` |
| instances | 7 nodes share one Derived Object (Box + Edit Mesh); top verts 4–7 rotated +25°; Box01 rotZ −41.5°, Box07 +47.0°; smoothing groups give smooth twisted sides |

Cylinder and Torus were confirmed visually by the user (no byte dump).

## Key format facts learned (details in `max-format.md`)

- CFB container; streams `Scene`, `ClassDirectory3`, `DllDirectory`, …; optional zlib.
- Chunk header: `u16 type, u32 size` (bit 31 = container); `size==0` → `u64` size.
- Scene children: chunk type = class index. **Exception: `0x2032`/`0x2033` = Derived Object** (built-in).
- References: `0x2034` positional; `0x2035` = count + (slot, index) pairs. Node slots: 0 PRS, 1 object, 3 material, 6 layer.
- Node: `0x0960` parent, `0x0962` name, `0x096A/B/C` pivot offset, `0x0974` wire RGB.
- Controllers: `0x2503` pos, `0x2504` quat (**conjugate** it: Max is left-handed, verified), `0x2505` scale.
- Old ParamBlock: `0x0002` records hold mini-chunks `0x0003` index, `0x0100` float, `0x0101` int, `0x0104` bool. PB2 `0x000A`: u16 id, u32 type, value at offset 11.
- Parameter order differs per primitive (Tube ≠ Cone). Clamp negative radii; negative height builds downward.
- Derived Object `0x2500` = one group per modifier: `0x2510` tm, `0x2511` bounds, `0x2512` local data, `0x2513`. Edit Mesh local `0x4000`: `0x0100` nverts, `0x0110` nfaces, `0x0140` vertex moves.
- Editable Mesh `0x08FE` → `0x0914` verts, `0x0912` faces (a, b, c, smGroup, flags): **never tested on a real file yet**.

## Implementation notes

- `evalObject(obj)`: tries `evalRaw` (exact Max vertex order: Editable Mesh/Poly, 1×1×1 Box) with modifiers applied bottom-up; unknown modifiers make it fall back to the unmodified base and add a log note.
- Smoothing groups: per-corner normals from faces sharing a bit (non-indexed geometry); group 0 = flat.
- Primitives built with three.js geometries follow the object's Smooth flag, not per-face groups.
- Viewer quirks fixed: r148+ has no `examples/js` (hence r128); r128 BufferGeometry has no `applyQuaternion` (use `applyMatrix4`).

## Known gaps

Editable Mesh/Poly untested; pivot offsets only seen as identity; hierarchy (linking) untested; materials untested; Hemisphere is uncapped and Slice is ignored; Teapot body/handle/spout cannot be toggled separately; Edit Mesh topology and smoothing edits are not decoded; other modifiers are not evaluated; no splines, cameras, lights, UVs, textures or animation; newer Max versions untested.

## Suggested next steps (in order)

1. **Editable Mesh** sample: a box and a teapot converted to Editable Mesh, with moved vertices and a deleted face.
2. **Hierarchy + pivot**: a child linked to a rotated parent; a box with its pivot moved (Affect Pivot Only).
3. **Materials**: a Standard material with a strong diffuse colour; Multi/Sub-Object with face material IDs.
4. Edit Mesh delete/extrude; Bend/Taper/Twist on a segmented box.
5. Cameras, lights and helpers as markers.
6. Optional: make the viewer import `max_loader.js`.
