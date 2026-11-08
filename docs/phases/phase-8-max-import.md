# Phase 8 — `.max` import

Branch: `phase/8-max-import`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 8.

## Steps
- [x] **8.1 Container and chunks**: `io::CompoundFile` (CFB: FAT with DIFAT, mini stream,
      the directory's red-black tree, 512- and 4096-byte sectors, bounds-checked) and
      `io::ParseChunks` (32- and 64-bit headers, containers, depth limit)
- [x] **8.2 Directories and the scene list**: `io::MaxScene` (classes with IDs, DLLs, the
      scene list, references from `0x2034` and `0x2035`); verified on all seventeen samples
- [ ] 8.3 Nodes: names, hierarchy, PRS controllers (quaternion conjugate), wire colours
- [ ] 8.4 Objects: old ParamBlock and ParamBlock2 → primitives; Derived Objects with Edit
      Mesh and Bend / Taper / Twist (checks Phase 7's parameter indices)
- [ ] 8.5 File > Open (and drag-and-drop) into the scene, with a report of what was skipped
- [ ] 8.6 The samples: R3.1's vertex/face counts, transforms, colours and instances
      (needs the `.max` files)
- [ ] 8.7 Native save in an own chunked format; wrap-up

## Step 8.1 verification
- [x] Builds warning-free (MinGW, clean rebuild); twenty-six test programs pass (MinGW + Wine)
- [x] `test_cfb`: against files written by an independent implementation (SheetJS cfb
      1.2.2, `tools/gen_cfb_fixtures.js`, `tests/data/cfb-*.cfb`): the same stream names
      in the same order as SheetJS's own reader lists, every byte of small (mini stream)
      and large (regular sector) streams; damaged files give errors (signature,
      truncation, a FAT sector outside the file)
- [x] `test_chunks`: nested containers, the 64-bit form (leaf and container), empty
      payloads; sizes past the parent, smaller than the header, cut headers, 70 levels deep
- [ ] On Windows: `ctest` (MSVC /W4)
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
- [ ] On Windows: `ctest` (Debug)

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
- [ ] On Windows: `ctest` (Debug)

## Notes
- Compression: none of the R3.1 samples were compressed (`max-format.md` §1); zlib is
  needed only for files from much later Max versions, so it is deferred.
