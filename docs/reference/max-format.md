# The 3ds Max `.max` file format — decoded so far

This is a working reference for reading Autodesk 3ds Max scene files without Max. Autodesk has never published the format; everything here was reverse-engineered. It builds on two earlier community projects — the Blender `io_import_max` add-on (Sebastian Sille) and Importer3D (Jens M. Plonka) — and was checked byte by byte against sample files saved from **3ds Max R3.1** (`box`, `sphere`, `geosphere`, `cone`, `tube`, `pyramid`, `teapot`, `plane`, `instances`, plus a cylinder and torus checked visually).

Each statement carries a confidence marker:

| Marker | Meaning |
|---|---|
| **✔ verified** | Confirmed against bytes in a sample file and against Max's own viewport |
| **◐ partial** | Structure confirmed, some fields still unknown |
| **○ unverified** | Taken from community sources or the Max SDK; not yet seen in a sample |

All integers and floats are **little-endian**. `u16`/`u32`/`i32` are unsigned/signed integers, `f32` is an IEEE-754 float, and strings are **UTF-16LE**.

---

## 1. The container: OLE2 Compound File ✔ verified

A `.max` file is a Microsoft Compound File Binary (CFB), the same container as legacy `.doc`/`.xls` files. It starts with the signature `D0 CF 11 E0 A1 B1 1A E1`.

| Header offset | Type | Field |
|---|---|---|
| `0x1E` | u16 | Sector shift (sector size = `1 << shift`, normally 512) |
| `0x20` | u16 | Mini-sector shift (normally 64-byte mini sectors) |
| `0x2C` | u32 | Number of FAT sectors |
| `0x30` | u32 | First directory sector |
| `0x38` | u32 | Mini-stream cutoff (4096): smaller streams live in the mini stream |
| `0x3C` | u32 | First mini-FAT sector |
| `0x40` | u32 | Number of mini-FAT sectors |
| `0x44` | u32 | First DIFAT sector |
| `0x48` | u32 | Number of DIFAT sectors |
| `0x4C` | u32 × 109 | First 109 FAT sector numbers |

Sector `n` starts at file offset `(n + 1) × sectorSize`. Values ≥ `0xFFFFFFFA` are chain markers (end of chain, free, FAT, DIFAT). Directory entries are 128 bytes: UTF-16 name (64 bytes), name length (u16 at 64), entry type (u8 at 66; 2 = stream, 5 = root), first sector (u32 at 116), size (u32 at 120, plus a high u32 at 124 for 4096-byte sectors). The root entry's chain is the mini stream.

### Streams found in a Max file

| Stream | Contents | Status |
|---|---|---|
| `Scene` | The scene graph and all objects (chunk tree) | ✔ decoded (see §3–§9) |
| `ClassDirectory3` | Table of every class used in the scene | ✔ decoded (§2.2) |
| `DllDirectory` | Plug-in DLLs the scene needs | ✔ decoded (§2.3) |
| `ClassData` | Per-class global data | not decoded |
| `Config` | Viewport/UI configuration | not decoded |
| `VideoPostQueue` | Video Post setup | not decoded |
| `\x05SummaryInformation`, `\x05DocumentSummaryInformation` | Standard OLE property sets (title, author, Max version, mesh totals) | not decoded |

Older files may use `ClassDirectory2` or `ClassDirectory` instead of `ClassDirectory3`.

### Compression ○ unverified

Newer Max versions can store streams zlib-compressed. A stream is treated as compressed if its first two bytes form a valid zlib header (`0x78`, and `(b0 << 8 | b1) % 31 == 0`). None of the R3.1 samples were compressed.

---

## 2. Chunks

### 2.1 Chunk header ✔ verified

Every stream except the OLE property sets is a tree of chunks.

```
u16  type
u32  size            // includes this header; bit 31 set = container (holds child chunks)
if size == 0:        // 64-bit form
  u64 size           // bit 63 set = container
```

The header is 6 bytes, or 14 bytes in the 64-bit form. A container's payload is a sequence of child chunks; a leaf's payload is raw data. Some leaf payloads themselves contain chunk-formatted records without the container bit; see the old ParamBlock (§6.1) and modifier context (§8.2).

### 2.2 `ClassDirectory3` ✔ verified

One entry per class, **in order**. The entry's position is the class index used throughout the Scene stream.

| Chunk | Contents |
|---|---|
| `0x2040` | Entry (container) |
| ↳ `0x2060` | `i32 dllIndex, u32 classIdA, u32 classIdB, u32 superClassId` (16 bytes) |
| ↳ `0x2042` | Class name (UTF-16) |

The 64-bit class "GUID" used below is `classIdB << 32 | classIdA`.

### 2.3 `DllDirectory` ✔ verified

Entries (`0x2038`, containers) contain `0x2039` (description, e.g. "Standard primitive
objects") and `0x2037` (DLL file name, e.g. `prim.dlo`), both UTF-16 without a terminator.
A class's `dllIndex` is the entry's position; −1 = built into Max (Box: 2 → `prim.dlo`).
*Corrected by maxlike: this section first had the two chunk types the other way round;
every R3.1 sample shows them as above.*

---

## 3. The Scene stream ✔ verified

The Scene stream has a single top-level container (type `0x2004` in the samples). Its children form the **scene list**: every object in the scene, in order. A child's **chunk type is its class index** in `ClassDirectory3`. Its own children are that object's data chunks.

Objects refer to each other by **position in the scene list**.

The one exception found so far: chunk types **`0x2032` and `0x2033`** are not class indices. They are Max's built-in **Derived Object** (§8).

### 3.1 References ✔ verified

An object's references to other objects are stored in one of two chunks:

| Chunk | Layout | Seen in |
|---|---|---|
| `0x2034` | `i32[]` — positional: slot *i* → scene-list index (−1 = empty) | PRS controller, Box, Derived Object |
| `0x2035` | `i32 slotCount`, then `(i32 slot, i32 index)` pairs for the filled slots only | Node, sub-controllers |

*Clarified by maxlike:* the first value is the object's number of reference slots, not the
number of pairs: a Bezier Position controller's `0x2035` is just `02000000` (two empty
slots); the node below has 8 slots, 3 filled.

Example from `box.max`, a node's `0x2035`:

```
08000000  00000000 0f010000  01000000 11010000  06000000 12010000
count=8   slot 0 → #271      slot 1 → #273      slot 6 → #274
          (PRS controller)   (Box object)       (Base Layer)
```

---

## 4. Nodes ✔ verified

A node is a scene-list object whose class has GUID `0x1` and superclass `0x1` (class name "Node"). The scene root has GUID `0x2`, superclass `0x1` ("RootNode").

### Node references (`0x2035`)

| Slot | Target |
|---|---|
| 0 | Transform controller (normally Position/Rotation/Scale) |
| 1 | Object (geometry, Derived Object, light, camera, …) |
| 3 | Material ○ unverified (empty in all samples) |
| 6 | Layer |

### Node data chunks

| Chunk | Contents | Status |
|---|---|---|
| `0x0960` | `i32 parentIndex` (scene-list index; the RootNode for top-level nodes), then `u32` flags | ✔ (`parent_child_link.max`: Box01's parent is Sphere01's node) |
| `0x0962` | Node name (UTF-16) | ✔ |
| `0x096A` | Pivot (object-offset) position, `f32 × 3` | ✔ `pivot.max`: (0, 0, −20.16222) with the node at z = 20.16222 |
| `0x096B` | Pivot rotation, quaternion `f32 × 4` (x, y, z, w; Max handedness, §5.2) | ◐ only identity values seen |
| `0x096C` | Pivot scale `f32 × 3`, then scale-axis quaternion `f32 × 4` | ◐ only identity values seen |
| `0x0974` | Wire colour: bytes **R, G, B**, 0 | ✔ e.g. `1A B1 94 00` = teal |
| `0x09CE`, `0x0963`, `0x099C`, `0x0A28`, `0x09BA`, `0x0978` | Unknown | — |

The object offset is applied between the node transform and the geometry:
`world = objectOffset × nodeTM × parentTM × …` (row vectors; maxlike's column-vector form
is `parentTM × nodeTM × objectOffset`, `scene::Node::ObjectMatrix`).

*Verified by maxlike (step 8.3):* every node in the seventeen samples reads with these
chunks; the value chunks are direct children of the node and of each controller.

---

## 5. Transform controllers

### 5.1 Position/Rotation/Scale (class `0x2005`) ✔ verified

`0x2034` holds three references: position, rotation and scale sub-controllers. The resulting transform is **relative to the parent node**.

| Controller | Class GUID | Value chunk | Status |
|---|---|---|---|
| Bezier Position | `0x2008` | `0x2503`: `f32 x, y, z` | ✔ |
| TCB Rotation | `0x442313` | `0x2504`: quaternion `f32 x, y, z, w` | ✔ |
| Bezier Scale | `0x2010` | `0x2505`: `f32 sx, sy, sz`, then scale-axis quaternion `f32 × 4` | ✔ |
| Position XYZ | `0xFFEE238A118F7E02` | three float sub-controllers | ○ |
| Euler XYZ rotation | `0x2012` | three float sub-controllers, radians | ○ |
| Scale XYZ | `0xFEEE238B118F7C01` | three float sub-controllers | ○ |
| Bezier Float | `0x2007` | `0x2501`: `f32`, sometimes inside `0x7127` | ○ |
| Position/Rotation/Scale List | `0x4B4B1002` / `1003` / `1004` | first sub-controller used | ○ |

Controllers also hold `0x2500` (8 bytes), `0x3002`, `0x3003` (unknown, probably ORT and range data). Keyframes are not decoded; the stored value is the value at frame 0.

### 5.2 Quaternion handedness ✔ verified

Max stores rotations as **left-handed** quaternions. To use one in a standard right-handed system, take the conjugate: `(−x, −y, −z, w)`.

Verified with `instances.max`: Box01 stores `(0, 0, 0.3543, 0.9351)`, which is +41.5° in standard convention. The Top viewport shows the box rotated **−41.5°**, which is what the conjugate gives.

---

## 6. Parameter blocks

Parametric objects keep their settings in a parameter-block object, reached through the object's first reference.

### 6.1 Old ParamBlock (class `0x8`) ✔ verified

| Chunk | Contents |
|---|---|
| `0x0001` | `u32` parameter count |
| `0x0005` | `u16` block version |
| `0x0002` | One parameter record (repeated) |

Each `0x0002` record is a leaf whose payload is itself a sequence of mini-chunks (6-byte headers):

| Mini-chunk | Contents |
|---|---|
| `0x0003` | `u32` parameter index |
| `0x0004` | Present (empty) when the value is not animated ◐ |
| `0x0100` | Value: `f32` |
| `0x0101` | Value: `i32` |
| `0x0104` | Value: bool as `i32` |

Example (Box length = 60.0):

```
03 00 0a000000 00000000   index 0
04 00 06000000            (not animated)
00 01 0a000000 00007042   float 60.0
```

**Stored values can be outside the UI range.** Dragging during creation can store negative numbers: a Cone stored radius 2 = −20.0 while its panel showed 0.0. Max clamps these when building the mesh, so readers should clamp too (radii to ≥ 0). A negative height builds the object downward from its pivot.

### 6.2 ParamBlock2 (class `0x82`) ✔ verified

| Chunk | Contents |
|---|---|
| `0x0009` | 16-byte header |
| `0x000A` | One parameter (repeated) |
| `0x0004` | 8 bytes, unknown |

Parameter entry (`0x000A`), 15 bytes for scalars:

| Offset | Type | Field |
|---|---|---|
| 0 | u16 | Parameter id (entries are **not** necessarily in id order) |
| 2 | u32 | Type: 0 float, 1 int, 2 RGBA, 3 Point3, 4 bool, 5 angle, 6 percent, 7 world units, … |
| 6 | 5 bytes | Flags (unknown) |
| 11 | f32 / i32 | Value (colours: 3 × f32 at offset 11, or 15 when the entry is longer than 23 bytes) |

---

## 7. Geometry

### 7.1 Standard primitives ✔ verified

All primitives build in object space with Max's Z-up axes. Unless noted, they sit on the XY plane with the pivot at the base centre.

| Object | Class GUID | Block | Parameter indices | Notes |
|---|---|---|---|---|
| Box | `0x10` | old | 0 length (Y), 1 width (X), 2 height (Z), 3 width segs, 4 length segs, 5 height segs, 6 mapping | ✔ |
| Sphere | `0x11` | old | 0 radius, 1 segments, 2 smooth, 3 hemisphere, 4 chop/squash, 5 base to pivot, 6 mapping, 7 slice on, 8 slice from, 9 slice to | ✔; centred on pivot unless *base to pivot* |
| GeoSphere | `0x7F9E` | PB2 | 0 radius, 1 segments, 2 base type (0 tetra, 1 octa, 2 icosa), 3 hemisphere, 4 smooth, 5 base to pivot, 6 mapping | ✔; segments = geodesic frequency |
| Cylinder | `0x12` | old | 0 radius, 1 height, 2 height segs, 3 cap segs, 4 sides, 5 smooth | ◐ visually checked; mesh ✔ (§7.6) |
| Cone | `0xA86C23DD` | old | 0 radius 1 (base), 1 radius 2 (top), 2 height, 3 height segs, 4 cap segs, 5 sides, 6 smooth, 7 slice on, 8 slice from, 9 slice to, 10 mapping | ✔ |
| Tube | `0x7B21` | old | 0 radius 1, 1 radius 2, 2 height, **3 sides, 4 cap segs, 5 height segs**, 6 smooth, 7–9 slice, 10 mapping | ✔; note the order differs from Cone |
| Torus | `0x20` ○ | old | 0 radius 1, 1 radius 2, 2 rotation, 3 twist, 4 segments, 5 sides | ◐ visually checked; mesh counts ✔ (§7.6) |
| Pyramid | `0x76BF318A, 0x4BF37B10` | old | 0 width (X), 1 depth (Y), 2 height, 3 width segs, 4 depth segs, 5 height segs, 6 mapping | ✔ |
| Teapot | `0xACAD13D3, 0xACAD26D9` | old | 0 radius, 1 segments, 2 smooth, 3 unused (−1), 4 body, 5 handle, 6 spout, 7 lid, 8 mapping | ✔; Utah teapot, Radius = body radius |
| Plane | `0x081F1DFC, 0x77566F65` | PB2 | 0 length (Y), 1 width (X), 2 length segs ◐, 3 width segs ◐, 4 render scale, 5 render density, 6 mapping | ✔; centred, facing +Z |

Teapot proportions (Blinn-scaled Utah data, in units of the radius): height 1.575, handle reach 1.5, spout reach ≈ 1.71.

### 7.2 Box vertex order ✔ verified

Index-based edits (§8.3) need Max's own vertex order. For a 1 × 1 × 1 segment box with `x = width/2`, `y = length/2`:

| Index | Position | Index | Position |
|---|---|---|---|
| 0 | (−x, −y, 0) | 4 | (−x, −y, h) |
| 1 | (+x, −y, 0) | 5 | (+x, −y, h) |
| 2 | (−x, +y, 0) | 6 | (−x, +y, h) |
| 3 | (+x, +y, 0) | 7 | (+x, +y, h) |

This was confirmed by applying the Edit Mesh deltas in `instances.max`: they rotate vertices 4–7 rigidly by exactly +25°, which matches the viewport. Faces (two per side, each side its own smoothing group): bottom `0 2 3, 3 1 0`; top `4 5 7, 7 6 4`; front `0 1 5, 5 4 0`; right `1 3 7, 7 5 1`; back `3 2 6, 6 7 3`; left `2 0 4, 4 6 2`.

### 7.3 Editable Mesh ◐ (structure from community sources; samples now exist: `box_editmesh`, `teapot_edit_mesh`, `teapot_smoothinggroups` hold an Editable Mesh)

Mesh data lives in child `0x08FE` of the object:

| Chunk | Contents |
|---|---|
| `0x0914` | `u32 count`, then `count × f32[3]` vertices |
| `0x0912` | `u32 count`, then `count ×` 20-byte faces: `u32 a, b, c`, `u32 smoothingGroups`, `u32 flags` |

In the face flags, the high 16 bits are thought to hold the material ID and the low bits edge visibility ○.

### 7.4 Editable Poly ○ unverified (Max 4 and later; cannot exist in R3.1 files)

Also inside `0x08FE`:

| Chunk | Contents |
|---|---|
| `0x0100` | `u32 count`, then `count ×` (`u32 flags`, `f32 x, y, z`) |
| `0x011A` | `u32 count`, then polygons: `u32 n`, `u32[n]` vertex indices, `u16 flags`; if flags & `0x01`: `u32`; & `0x08`: `u16` (material ID?); & `0x10`: `u32` (smoothing groups?); & `0x20`: `u32[2·(n−3)]` (triangulation?) |
| `0x0108` | Older alternative: `u32 count`, then 6 × `i32` records |

### 7.5 Smoothing groups ✔ verified (rendering), ◐ (Editable Mesh source untested)

Each face has a 32-bit mask. A face corner's normal averages the normals of the faces around that vertex that share at least one bit with the face; faces with mask 0 are flat. The `instances.max` boxes show why this matters: each twisted side is a non-planar quad split into two triangles, and Max shades it smoothly across the diagonal because both triangles share a group.


### 7.6 Mesh topology verified against R3.1 (added by maxlike)

R3.1's File → Summary Info gives Max's own counts (screenshots in
`docs/reference/screenshots/`). They differ from the JS viewer's three.js-based counts:

| Sample | R3.1 vertices / faces | JS (three.js) triangles |
|---|---|---|
| cone (24 × 5, r2 = 0) | 146 / 288 | 264 |
| tube | 216 / 432 | 432 |
| cylinder (18 × 5) | 110 / 216 | — |
| torus (24 × 12) | 288 / 576 | — |
| sphere (32) | 482 / 960 | 960 |
| geosphere (icosa, 4) | 162 / 320 | 320 |
| teapot (4 segments) | 530 / 1024 | 992 |

Findings: Max keeps zero-size geometry (the cone's tip ring and top cap, the teapot's
degenerate triangles); the teapot merges points only within each part (Body, Handle,
Spout, Lid); cap fans hide their spokes; rings start at +X. Construction rules:
`docs/spec/04-primitives.md`.

---

## 8. Modifier stacks

### 8.1 Derived Object (chunk type `0x2032` / `0x2033`) ✔ verified

Any object with modifiers is wrapped in a Derived Object. Its type is a fixed chunk type, **not a class index**. Instances share one Derived Object: all seven nodes in `instances.max` reference the same `0x2032`.

| Chunk | Contents |
|---|---|
| `0x2034` | References: modifiers from the **top of the stack down**, then the base object last |
| `0x2500` | Modifier contexts (§8.2) |
| `0x2501` | Empty in samples |

Evaluate from the base object up: apply the last modifier reference first.

### 8.2 Modifier context (`0x2500`) ✔ verified

A leaf whose payload is a chunk sequence, with one group per modifier in reference order:

| Chunk | Contents |
|---|---|
| `0x2510` | ModContext transform: 3 × 4 `f32` matrix, then `u32` |
| `0x2511` | Bounding box (24 bytes) |
| `0x2512` | Container: the modifier's **local data** |
| `0x2513` | `u32`, unknown |

A new group starts at each `0x2510`, so a modifier with no local data does not shift the others.

### 8.3 Edit Mesh (class `0x50`, superclass `0x810`) ◐

Local data (`0x2512`) holds `0x2740` (`u32`) and a container `0x4000`:

| Chunk | Contents | Status |
|---|---|---|
| `0x0100` | `u32` vertex count of the input mesh (check before applying) | ✔ |
| `0x0110` | `u32` face count of the input mesh | ✔ |
| `0x0140` | Vertex moves: `u32 count`, then `count ×` (`u32 index`, `f32 dx, dy, dz`) | ✔ |
| `0x0300` | `u32`, unknown (2 in sample) | — |

Example from `instances.max` (top face of the box twisted +25°):

```
0x0140: 04000000
        04000000  5.559  -3.289  0   (vertex 4)
        05000000  3.648   5.330  0   (vertex 5)
        06000000 -3.648  -5.330  0   (vertex 6)
        07000000 -5.559   3.289  0   (vertex 7)
```

Topology edits (delete, extrude, and so on) and smoothing-group or material changes made in Edit Mesh are not decoded yet.

---

## 9. Materials ○ unverified

From community sources; no sample has had a material assigned yet. Parameters in old-style material parameter blocks are children whose `type & 0x100E` is non-zero, with a `u16` parameter index at offset 0 and a colour (`f32 × 3`) at offset 15 (or 11 for short entries).

| Material | GUID | Diffuse colour |
|---|---|---|
| Standard | `0x2` (superclass `0xC00`) | reference 2 → its reference 0 → parameter `0x01` |
| Multi/Sub-Object | `0x200` | last sub-material |
| VRayMtl | `0x7034695C37BF3F2F` | reference 1, parameter `0x01` |
| Architectural | `0x4A16365470B05735` | reference 0, parameter `0x1A` |

---

## 10. Coordinates and units

Max is **Z-up and right-handed**; three.js is Y-up. Rotating the scene root by −90° about X converts between them without mirroring, so triangle winding is preserved. Units are Max system units as stored (the samples use generic units); the unit scale is not decoded yet.

---

## 11. Open questions and next samples

| Topic | Sample that would settle it |
|---|---|
| Editable Mesh in a real file (§7.3) | A box and a teapot converted to Editable Mesh, with moved vertices and a deleted face |
| Pivot offsets with real values (§4) | A box whose pivot was moved with *Affect Pivot Only* |
| Linked hierarchy (§4) | A box linked to a rotated parent |
| Material slot and colours (§9) | A box with a Standard material; a mesh with Multi/Sub-Object material IDs |
| Plane length/width segment ids (§7.1) | A plane with different segment counts (e.g. 2 × 6) |
| Sphere slice angles: degrees or radians | A sphere with *Slice On*, from 0° to 90° |
| Other modifiers (Bend, Taper, Twist) | A segmented box with one modifier each |
| Edit Mesh topology changes (§8.3) | An Edit Mesh that deletes and extrudes faces |
| Newer Max versions (compression, PB2 everywhere) | Any file saved from Max 2010 or later |
