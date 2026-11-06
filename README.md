# maxlike

A Windows C++ application that recreates the look and workflow of 3D Studio MAX R3.1
(four viewports, command panel, Standard Primitives, modifier stack), plus a native
importer for R3-era `.max` files. "maxlike" is a working name; rename freely.

Stack: C++17, Win32 + classic common controls behind a thin in-house wrapper,
OpenGL 3.3 (GLAD) viewports, CMake + MSVC. Icons are drawn from scratch.

## Status

| Phase | State | Version |
|---|---|---|
| 0 Skeleton | ✔ | |
| 1 Frame layout (menus, toolbar, time slider, status area) | ✔ pixel-checked | 0.1.0 |
| 2 Viewports (grid, cameras, labels, tripods, navigation) | ✔ pixel-checked | 0.2.0 |
| 3 Scene core (nodes, primitives, modifiers, shading, undo) | ✔ counts match R3.1 | 0.3.0 |
| 4 Command panel (tabs, categories, rollouts, Object Type, Name and Color) | ✔ pixel-checked | 0.4.0 |
| 5 Interactive creation (all primitives, live Parameters, Keyboard Entry, undo) | ✔ panel pixel-checked | 0.5.0 |
| 6 Selection and transforms (6.1 Objects tab strip ✔ pixel-checked) | in progress | |
| 7–8 Modify panel, `.max` import | planned | |

Details: `docs/spec/02-plan.md`; per-phase checklists in `docs/phases/`.

## Build

Requires Visual Studio 2019+ (Desktop development with C++) and CMake 3.20+.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug      # 21 unit-test programs
build\Debug\maxlike.exe
```

`maxlike.exe --client-size 1920x998` opens the window un-maximised with exactly the
client size of the R3.1 reference screenshot (useful for pixel comparisons).
`--demo-scene <name>|all` opens with objects from the verified `.max` samples (box, plane,
pyramid, cylinder, cone, tube, torus, sphere, geosphere, teapot), or `instances` (seven nodes sharing one
modified box).

## Branches

| Branch | Purpose |
|---|---|
| `main` | Integrated, always-building code. Phases land here when done. |
| `spec` | The original specification. Since Phase 1, spec updates travel with the phase that measured or decided them; use `spec` only for spec-only edits between phases. |
| `phase/0-skeleton` … `phase/8-max-import` | One branch per phase of the plan in `docs/spec/02-plan.md`. |

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
