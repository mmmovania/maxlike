# maxlike

A Windows C++ application that recreates the look and workflow of 3D Studio MAX R3.1
(four viewports, command panel, Standard Primitives, modifier stack), plus a native
importer for R3-era `.max` files. "maxlike" is a working name for now.

Stack: C++17, Win32 + classic common controls behind a thin in-house wrapper,
OpenGL 3.3 (GLAD) viewports, CMake + MSVC. Icons are drawn from scratch.

## Status
This is how it looks so far
<img width="1920" height="1040" alt="image" src="https://github.com/user-attachments/assets/6195c5bf-6522-4448-9273-99d49c85f3a0" />

| Phase | State | Version |
|---|---|---|
| 0 Skeleton | ✔ | |
| 1 Frame layout (menus, toolbar, time slider, status area) | ✔ pixel-checked | 0.1.0 |
| 2 Viewports (grid, cameras, labels, tripods, navigation) | ✔ pixel-checked | 0.2.0 |
| 3 Scene core (nodes, primitives, modifiers, shading, undo) | ✔ counts match R3.1 | 0.3.0 |
| 4 Command panel (tabs, categories, rollouts, Object Type, Name and Color) | ✔ pixel-checked | 0.4.0 |
| 5 Interactive creation (all primitives, live Parameters, Keyboard Entry, undo) | ✔ panel pixel-checked | 0.5.0 |
| 6 Selection and transforms (Objects strip, selection, Name and Color, Move/Rotate/Scale, X/Y/Z) | ✔ strip pixel-checked, gizmo measured | 0.6.0 |
| 7 Modify panel (modifier stack, Bend/Taper/Twist, Edit Mesh at Vertex level) | ✔ panels pixel-checked | 0.7.0 |
| 8 `.max` import (open R3-era files, saved viewports; maxlike's own `.mlk` files) | ✔ samples verified against R3.1 | 0.8.0 |
| 9 Deferred items (menus and shortcuts as R3.1, File / Edit commands, all Edit Mesh levels, Transform Type-In, coordinate systems, Mirror, Array) | ✔ menus verified against R3.1 | 0.9.0 |
| 10 Groups and the rest of list A (`docs/phases/phase-10-groups.md`) | in progress | |

Details: `docs/spec/02-plan.md`; per-phase checklists in `docs/phases/`.

## Build

Requires Visual Studio 2019+ (Desktop development with C++) and CMake 3.20+.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug      # 39 unit-test programs
build\Debug\maxlike.exe
```

`maxlike.exe --client-size 1920x998` opens the window un-maximised with exactly the
client size of the R3.1 reference screenshot (useful for pixel comparisons).
`maxlike.exe scene.max` (or File > Open…, or dropping a file on the window) opens a 3ds Max
R3-era `.max` file: primitives, Editable Meshes and Edit Mesh stacks, with names, hierarchy,
transforms and wire colours; a report lists anything it could not import. The seventeen R3.1
samples are in `docs/reference/samples/`.
The menus and their keyboard shortcuts follow R3.1 (Ctrl+O, Ctrl+S, Ctrl+Z, Ctrl+A for
Redo as in R3.1, F12 for the Transform Type-In, M, 1, …); single-key shortcuts are ignored
while typing in a field. The File menu lists the nine most recent files.
`--demo-scene <name>|all` opens with hand-built objects (box, plane, pyramid, cylinder, cone,
tube, torus, sphere, geosphere, teapot), `instances` (seven nodes sharing one modified box) or
`deform` (three tall boxes with Bend, Twist and Taper). They were transcribed before maxlike
could read `.max` files: most match their sample exactly, but cylinder, torus and instances
use estimated sizes and colours — open the sample itself for the real scene.

## Branches

| Branch | Purpose |
|---|---|
| `main` | Integrated, always-building code. Phases land here when done. |
| `spec` | The original specification. Since Phase 1, spec updates travel with the phase that measured or decided them; use `spec` only for spec-only edits between phases. |
| `phase/0-skeleton` … `phase/10-groups` | One branch per phase of the plan in `docs/spec/02-plan.md`. |

## Workflow

Phases build on each other, so work on them one at a time:

```sh
git switch phase/1-frame-layout
git rebase main                 # pick up the previous phase and latest spec
# ...work, commit...
git switch main
git merge --no-ff phase/1-frame-layout
git tag phase-1-done
```

Work arrives as patch files, one per step, each built on the latest pushed commit:

```sh
findstr /B "From " <step>.patch                  # which commit it was made from
git am <step>.patch && git commit --amend --no-edit --reset-author
cmake --build build --config Debug && ctest --test-dir build -C Debug
git push
```

If `git am` fails, `git am --abort` and report the error; never amend after a failed
`git am`. More Git lessons: `docs/tech/phase1-tech-docs.md` and `phase2-tech-docs.md`.

Each phase branch has a checklist in `docs/phases/`: items marked "On Windows" are
ticked by the project owner after trying them; "Unverified" items need a check in R3.1.

## Documentation

| Where | What |
|---|---|
| `docs/spec/` | Specification: decisions, measured UI look, plan, menus, Standard Primitives |
| `docs/phases/` | Per-phase checklists and verification results |
| `docs/tech/` | Technical documentation, one per finished phase |
| `docs/reference/` | R3.1 screenshots (reference scene, Summary Info counts), the JS `.max` reader and its format notes |
