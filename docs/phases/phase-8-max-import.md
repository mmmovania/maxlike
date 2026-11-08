# Phase 8 — `.max` import

Branch: `phase/8-max-import`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 8.

## Steps
- [x] **8.1 Container and chunks**: `io::CompoundFile` (CFB: FAT with DIFAT, mini stream,
      the directory's red-black tree, 512- and 4096-byte sectors, bounds-checked) and
      `io::ParseChunks` (32- and 64-bit headers, containers, depth limit)
- [x] **8.2 Directories and the scene list**: `io::MaxScene` (classes with IDs, DLLs, the
      scene list, references from `0x2034` and `0x2035`); verified on all seventeen samples
- [x] **8.3 Nodes**: `io::ReadNodes` (names, parents, wire colours, PRS values with the
      quaternion conjugate, object offsets; unsupported controllers reported as notes) and
      `WorldMatrix` / `ObjectMatrix` in `scene::Node`'s order
- [x] **8.4a Primitives**: `io::ReadParamBlock` (old ParamBlock and ParamBlock2),
      `io::ReadPrimitive` (class → `scene::MakePrimitive`, parameters by index, radii
      clamped, Slice reported)
- [x] **8.4b Editable Mesh and modifier stacks**: `io::ReadEditableMesh` (vertices, faces,
      smoothing, material IDs, edge visibility); `io::ObjectReader` (each object built once,
      so instances stay shared; Derived Objects with Edit Mesh's vertex moves; other
      modifiers and edits reported)
- [ ] Bend / Taper / Twist from files: no sample has them (Phase 7's parameter indices stay
      unverified until one does)
- [x] **8.5 File > Open**: `io::ImportMax` (nodes, links, colours, transforms, offsets,
      shared objects, a report) and `MainFrame::OpenFile` (File > Open…, drag-and-drop,
      command line; the scene replaced only if the file reads)
- [x] **8.6 Against R3.1**: saved viewports read and restored (`io::ReadViewports`);
      wireframes with back faces culled; the ortho grid's zoom-dependent shade and
      subdivision; compared with R3.1 screenshots of two opened samples
- [x] **8.7 Saving**: maxlike's own `.mlk` files (`io::SaveNative` / `io::LoadNative`, the
      .max chunk structure, objects once so instances stay shared, exact doubles); File >
      Save and Save As…; `.mlk` files open like `.max` files (recognised by signature)
- [x] **8.8 Wrap-up**: checks, `docs/tech/phase8-tech-docs.md`, version 0.8.0, a sweep of
      the plan, overview, README and tech index

## Step 8.1 verification
- [x] Builds warning-free (MinGW, clean rebuild); twenty-six test programs pass (MinGW + Wine)
- [x] `test_cfb`: against files written by an independent implementation (SheetJS cfb
      1.2.2, `tools/gen_cfb_fixtures.js`, `tests/data/cfb-*.cfb`): the same stream names
      in the same order as SheetJS's own reader lists, every byte of small (mini stream)
      and large (regular sector) streams; damaged files give errors (signature,
      truncation, a FAT sector outside the file)
- [x] `test_chunks`: nested containers, the 64-bit form (leaf and container), empty
      payloads; sizes past the parent, smaller than the header, cut headers, 70 levels deep
- [x] On Windows: `ctest` (MSVC /W4)
- [ ] Not covered by the fixtures: files with more than 109 FAT sectors (the DIFAT chain;
      over ~7 MB) and 4096-byte sectors; zlib-compressed streams (not in R3 files;
      deferred)

## Step 8.1b verification (a fix found on Windows)
- [x] MSVC's Debug build caught an out-of-bounds read in `test_cfb` (a truncated file:
      the directory's sectors were followed without checking they lie inside the file);
      reproduced here with bounds-checked containers (`_GLIBCXX_DEBUG`) and the address
      and undefined-behaviour sanitizers; every whole-sector read (FAT, DIFAT, directory,
      mini FAT) is now checked
- [x] A robustness sweep under those checks: the seventeen R3.1 samples truncated every
      997 bytes and randomly corrupted (300 each, fixed seed), 7,067 runs: no crash, no
      out-of-bounds access (5,072 still open, 1,995 cleanly rejected)
- [x] On Windows: `ctest` (Debug)

## Step 8.2 verification
- [x] Builds warning-free; twenty-eight test programs pass (MinGW + Wine)
- [x] `test_max_scene` on the samples: `box.max`'s 35 classes with the 3ds Max SDK's class
      IDs (Box 0x10 / geometry 0x10, PRS 0x2005 / 0x9008, Node 1 / 1), Box's DLL `prim.dlo`;
      the node's eight slots (controller #271, Box #273, Base Layer #274) and the chain
      beneath it; `instances.max`: seven nodes on one Derived Object (Edit Mesh over Box);
      in all seventeen: references inside the scene list, valid DLL indices, every type
      known, the expected node count and geometry
- [x] Under bounds-checked containers and sanitizers; a sweep of 4,714 truncated and
      corrupted samples through `MaxScene`: no crash, no out-of-bounds access
- [x] Format notes corrected from the data: `DllDirectory`'s `0x2039` / `0x2037` were the
      other way round; `0x2035`'s first value is the slot count
- [x] Found: three samples hold an Editable Mesh (collapsed), the object type §7.3 had no
      sample for
- [x] On Windows: `ctest` (Debug)

## Step 8.3 verification
- [x] Builds warning-free; twenty-nine test programs pass (MinGW + Wine)
- [x] `test_max_nodes` on the samples, matrices checked against independently computed
      positions: `box` (Box01 at the origin, teal 26/177/148); `pivot` (the first
      non-identity object offset, (0, 0, −20.16222), with the node at z = 20.16222);
      `parent_child_link` (Box01 linked to Sphere01, its world position through the
      sphere); `instances` (Box01 turned −41.5° as R3's Top view shows, §5.2; Box07
      +47°; all seven on one object); `boxes` (Box04's scale 0.6061 × 1.45); every node in
      all seventeen read with nothing left unread
- [x] Under sanitizers; a sweep of 4,250 corrupted samples through `ReadNodes` and the
      matrices: no crash, no out-of-bounds access, no hang
- [x] On Windows: `ctest` (Debug)
- [x] To compare with R3.1: positions in `parent_child_link` and `pivot` (a screenshot of
      either open in R3.1)
- Not in any sample: non-identity object-offset rotation or scale, scale axes, controller
  types other than Bezier Position / TCB Rotation / Bezier Scale (reported as notes)

## Step 8.4a verification
- [x] Builds warning-free; thirty test programs pass (MinGW + Wine)
- [x] `test_max_objects`: cone, tube, cylinder, torus, sphere, geosphere and teapot,
      imported and built by maxlike, give exactly R3.1's Summary Info vertex and face
      counts; values from both block formats (Box 60³, Plane 42 × 40 with 4 × 4 segments,
      GeoSphere's out-of-order entries, Tube's own order); the Cone's stored radius 2 of
      −20 becomes 0; not-a-primitive and not-a-block give notes
- [x] Found: `pivot.max`'s object offset is minus half the Box's height (the pivot was
      centred, not moved to the top as 8.3 guessed); GeoSphere's class ID is in the second
      word; Torus's class ID confirmed
- [x] Under sanitizers; 2,040 corrupted samples through the readers and the builders: no
      crash, no out-of-bounds access (the builders' clamps bound damaged counts)
- [x] On Windows: `ctest` (Debug)

## Step 8.4b verification
- [x] Builds warning-free; thirty test programs pass (MinGW + Wine)
- [x] R3.1's collapsed Box equals maxlike's 60-unit Box in every field (found and fixed:
      maxlike's Box had no material IDs; now each side has Max's)
- [x] R3.1's collapsed teapot equals maxlike's in all 530 points (found and fixed: Max's
      teapot data has control point 16 at x = −0.874 where the common data has −0.784;
      derived from 12 rim points that all imply exactly this one change)
- [x] `teapot_smoothinggroups`: 56 flat faces, 968 in group 1
- [x] `instances.max`: seven nodes share one imported object; its Edit Mesh turns the top
      vertices +25.00° (measured on the evaluated mesh), the bottom unmoved
- [x] Every node of all seventeen samples imports without a note
- [x] Under sanitizers; 2,550 corrupted samples through `ObjectReader`: no crash, no
      out-of-bounds access (every count checked against its chunk's size)
- [x] On Windows: `ctest` (Debug)

## Step 8.5 verification
- [x] Builds warning-free; thirty-one test programs pass (MinGW + Wine)
- [x] `test_max_import`: in all seventeen samples every imported node's world and object
      matrices equal those computed straight from the file (`io::ReadNodes`), with links,
      wire colours and names; instances share one object; nothing left unimported; a
      mutation check confirms the test catches the linking-order bug found while writing it
      (Scene::Link keeps world positions, so transforms are set after linking)
- [x] Under Wine, opened from the command line and compared with the hand-built demo
      scenes: box, plane, cone, tube, sphere, geosphere and teapot 0 differing pixels;
      pyramid 3 (the file's node sits at z = 0.005); cylinder, torus and instances differ
      because their demo scenes used estimated sizes and colours (the files: cylinder
      10 × 20, torus 7.549 / 2.451, instances' boxes 21.8 × 20.4 × 19.5 in crimson)
- [x] A file named `.max` that is not one: a message, the scene and title untouched
- [x] On Windows: File > Open…, drag-and-drop, double-clicking a `.max` file associated
      with maxlike (the command line)
- [x] Compare an opened sample with R3.1 (a screenshot of the same file open in R3.1)

## Step 8.6 verification
- [x] Builds warning-free; thirty-two test programs pass (MinGW + Wine)
- [x] `test_max_viewports`: the four views of every sample; instances' Top width
      400 × 0.79068 (R3.1's 2.73 px per unit), its perspective camera 15° up, 45°
- [x] `test_ortho_grid`: shade and spacing for the six measured views (all exact); Phase
      2's 180 default lines unchanged; `test_primitives`: culled edges
- [x] Under Wine against `r3.1-open-instances.png` / `r3.1-open-parent-child-link.png`:
      all views framed like R3.1; ortho views 979 / 2,053 / 2,970 and 774 / 2,557 / 822
      differing pixels (from 14,000–40,000 before the grid shade; line rasterisation and
      labels remain); perspective views ~80,000 (the larger home grid, and lighter
      shading in R3.1: open)
- [x] Default window, Modify panels (Bend 0, Edit Mesh 0): unchanged
- [x] Found: R3.1 restores saved viewports; culls back faces in wireframe; darkens minor
      grid lines with spacing and subdivides zoomed-in grids; draws a larger perspective
      grid for a distant camera (rule unknown)
- [x] On Windows: open both samples and compare with the screenshots
- [ ] Open: the perspective grid's extent; perspective shading; the cull flag's bit

## Step 8.7 verification
- [x] Builds warning-free; thirty-three test programs pass (MinGW + Wine)
- [x] `test_native_file`: all seventeen samples imported, saved and loaded back equal the
      original exactly (names, colours, links, world and object matrices, every vertex
      and face, shared objects), and saving again gives the same bytes; the deform demo's
      Bend / Taper / Twist settings and a switched-off modifier; an Edit Mesh's moves and
      selection; damaged input rejected
- [x] Under sanitizers; 7,200 truncated or corrupted saved scenes: no crash, no
      out-of-bounds access
- [x] Under Wine: `instances.mlk` (2 KB; the .max is 112 KB) opens from the command line,
      titled "instances.mlk - maxlike", seven shared boxes
- [x] On Windows: Save As…, Save (overwrites the .mlk; a scene opened from .max always
      asks for a name), reopen
- Not stored yet: viewports, the selection; no keyboard shortcuts (none exist yet)

## Step 8.8 verification
- [x] Clean rebuild: 0 warnings; thirty-three test programs pass (MinGW + Wine)
- [x] Default window unchanged; Modify panels against R3.1: Bend 0, Edit Mesh 0
- [x] On Windows: `ctest`; the "Try it" steps of 8.5–8.7

## Notes
- Compression: none of the R3.1 samples were compressed (`max-format.md` §1); zlib is
  needed only for files from much later Max versions, so it is deferred.
