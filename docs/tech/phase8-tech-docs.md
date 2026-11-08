# Phase 8 — Technical documentation

Phase 8 taught maxlike to open 3ds Max R3-era `.max` files — the compound-file container,
the chunk trees, the class and DLL directories, the scene list, nodes, parameter blocks,
primitives, Editable Meshes and Edit Mesh stacks — to show them as R3.1 does, with the
saved viewports, and to save scenes in its own `.mlk` format. Everything was checked
against seventeen R3.1 sample files and two screenshots of samples open in R3.1. Step
record: `docs/phases/phase-8-max-import.md`; the format: `docs/reference/max-format.md`.

Code at the end of Phase 8: tag `phase-8-done` (version 0.8.0).

## Contents
1. Architecture and file map
2. The container: a compound-file reader
3. Testing against someone else's implementation
4. Different compilers, different safety nets
5. Robustness sweeps
6. The vocabulary: classes, DLLs, the scene list, references
7. Nodes and transforms
8. Objects: parameter blocks, primitives, Editable Mesh, modifier stacks
9. Reading an error's shape: Max's teapot
10. Opening files, and restoring R3's views
11. maxlike's own files
12. Results
13. Open items
14. Exercises

---

## 1. Architecture and file map

| Layer | Files | Windows? |
|---|---|---|
| Compound file (CFB): FAT, DIFAT, mini stream, directory | `src/io/Cfb.*` | no |
| Chunk trees (32- and 64-bit headers) | `src/io/Chunks.*` | no |
| Class / DLL directories, scene list, references | `src/io/MaxScene.*`, `src/io/Utf16.h` | no |
| Nodes: names, parents, colours, PRS, object offsets | `src/io/MaxNodes.*` | no |
| Parameter blocks, primitives, Editable Mesh, Derived Objects | `src/io/MaxObjects.*` | no |
| Building a `scene::Scene` with a report | `src/io/MaxImport.*` | no |
| Saved viewports | `src/io/MaxViewports.*` | no |
| maxlike's own `.mlk` files | `src/io/NativeFile.*` | no |
| File > Open / Save / Save As, drag-and-drop, command line | `src/ui/MainFrame.*`, `src/main.cpp` | yes |
| Culled wireframes, the ortho grid's shade and subdivision | `src/scene/Mesh.*`, `src/ui/Viewport.*`, `src/view/OrthoGrid.*` | partly |

Each layer reads only from the one below and is tested on its own.

---

## 2. The container: a compound-file reader

A `.max` file is a Microsoft compound file: 512-byte sectors chained through a FAT (whose
own sectors are listed in the header and, for large files, a DIFAT chain), a directory of
128-byte entries arranged as a red-black tree, and a mini stream of 64-byte sectors for
streams under 4,096 bytes. Every offset is bounds-checked; chains are checked for loops.

---

## 3. Testing against someone else's implementation

A reader tested only against files its own code wrote can share a misunderstanding with
the writer. The CFB test files were written by SheetJS `cfb`, an independent
implementation: the reader must list exactly the streams SheetJS lists, in its order, and
reproduce every byte. The same idea recurs: class IDs checked against the 3ds Max SDK's
published constants; primitives checked against R3.1's own vertex counts and against
Max's own collapsed meshes.

---

## 4. Different compilers, different safety nets

`test_cfb` passed under MinGW but aborted in MSVC's Debug build: a truncated file made
the reader follow directory sectors past the end of its buffer. GCC read garbage
silently; MSVC's checked containers caught it. Reproducing it with `_GLIBCXX_DEBUG` and
the address and undefined-behaviour sanitizers pinned the index; every whole-sector read
is now checked.

---

## 5. Robustness sweeps

After that fix, every reader was run under the same strict checks on thousands of
truncated and randomly corrupted samples (CFB 7,067; scene list 4,714; nodes 4,250;
objects 2,040 and 2,550; `.mlk` 7,200): never a crash or an out-of-bounds access. Counts
from files are checked against their chunk's size before allocating; parent chains are
bounded; the primitive builders' clamps bound damaged segment counts.

---

## 6. The vocabulary: classes, DLLs, the scene list, references

The class directory gives each class its ID, superclass and DLL; the scene list (the
Scene stream's top container) holds every object; references come as positional lists
(`0x2034`) or slot pairs (`0x2035`, whose first value is the slot count). The samples
corrected the notes twice (the DLL chunks' order, the meaning of that first value) and
revealed that GeoSphere's class ID sits in the second word.

---

## 7. Nodes and transforms

A node's PRS controller holds Bezier Position, TCB Rotation (left-handed: conjugated on
reading) and Bezier Scale values; the object offset sits between node and geometry.
`pivot.max` gave the first real offset (minus half the box's height: the pivot was
centred). Matrices were checked against positions computed another way (rotating
vectors with quaternions). Importing found that `Scene::Link` keeps world positions, so
the file's parent-relative transforms are set only after linking — a mutation check
confirmed the test catches the wrong order.

---

## 8. Objects: parameter blocks, primitives, Editable Mesh, modifier stacks

maxlike's primitives already used Max's parameter indices, so importing copies values by
index; seven imported primitives, built by maxlike, give exactly R3.1's Summary Info
counts. Editable Mesh faces carry smoothing groups, material IDs (flags >> 16) and edge
visibility (flags & 7). Derived Objects list modifiers top-down; each object is built
once (`ObjectReader`), so instances stay shared. R3.1's collapsed Box matched maxlike's in
every field once the Box got Max's material IDs.

---

## 9. Reading an error's shape: Max's teapot

The collapsed teapot matched maxlike's in counts but 12 rim points differed by up to
0.57. Their ratios matched one control point's Bernstein weights across the patch and up
the rim; every one implied the same change — point 16's x is −0.874 in Max's data where
the common Utah data has −0.784. Max R3 ships the digits transposed; maxlike adopted
Max's value, and all 530 points agree.

---

## 10. Opening files, and restoring R3's views

Opening imports into a separate scene and replaces the current one only on success.
R3.1 restores the viewports saved in the Config stream (`0x2810`–`0x2813`: view type,
view matrix, zoom or field of view and distance); maxlike now does too, and both opened
samples are framed exactly like R3.1's screenshots. The comparison also showed R3.1 culls
back faces in wireframe, darkens minor grid lines with their spacing
(floor(160 − 0.8674 g), exact for six views) and subdivides zoomed-in grids. Two pitfalls
on the way: the screenshot's title bar counted twice, and a stopped display that left
stale images — both caught by looking at the evidence before trusting a number.

---

## 11. maxlike's own files

`.mlk` files reuse the chunk structure: objects once (instances shared), modifier stacks
with their parameters and Edit Mesh data, nodes with links and transforms, all as
doubles. Every sample round-trips exactly, and saving again reproduces the same bytes.

---

## 12. Results

- Seventeen R3.1 samples open with nothing left unimported.
- Seven primitives match R3.1's vertex/face counts; R3.1's collapsed Box and teapot match
  maxlike's field by field and point by point.
- Opened samples against R3.1 screenshots: every view framed alike; ortho views 774–2,970
  differing pixels each (line rasterisation, labels).
- Thirty-three test programs; 0 warnings.

---

## 13. Open items

- The perspective home grid's extent for distant cameras (instances: x −400…+300, far
  edge 400; others ±200); perspective shading; the subdivision threshold (4.9–18.5 px).
- No sample yet with Bend / Taper / Twist (Phase 7's indices unverified against files),
  with Backface Cull off, or with materials.
- `.mlk` does not store viewports or the selection; no keyboard shortcuts.
- zlib (only for much later Max versions).

---

## 14. Exercises

| Step | Try |
|---|---|
| 8.1 | Truncate a sample by hand and open it: a message, nothing changes. |
| 8.3 | Open `parent_child_link.max`, move the sphere: the box follows. |
| 8.4 | Open `instances.max`, add a Bend to one box: all seven bend (one object). |
| 8.5 | Drag several samples onto the window in turn; read the prompt line. |
| 8.6 | Open `instances.max` and zoom the Top view in: watch the minor lines darken as they spread, then subdivide. |
| 8.7 | Save `instances.max` as `.mlk`, reopen it, compare sizes. |
