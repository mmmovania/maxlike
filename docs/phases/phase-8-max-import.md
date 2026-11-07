# Phase 8 — `.max` import

Branch: `phase/8-max-import`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 8.

## Steps
- [x] **8.1 Container and chunks**: `io::CompoundFile` (CFB: FAT with DIFAT, mini stream,
      the directory's red-black tree, 512- and 4096-byte sectors, bounds-checked) and
      `io::ParseChunks` (32- and 64-bit headers, containers, depth limit)
- [ ] 8.2 `ClassDirectory3` / `DllDirectory` and the Scene stream's reference table
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

## Notes
- Compression: none of the R3.1 samples were compressed (`max-format.md` §1); zlib is
  needed only for files from much later Max versions, so it is deferred.
