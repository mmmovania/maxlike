# maxlike

A Windows C++ application that recreates the look and workflow of 3D Studio MAX R3.1
(four viewports, command panel, Standard Primitives, modifier stack), plus a native
importer for R3-era `.max` files. "maxlike" is a working name; rename freely.

Stack: C++17, Win32 + classic common controls behind a thin in-house wrapper,
OpenGL 3.3 (GLAD) viewports, CMake + MSVC. Icons are drawn from scratch.

## Branches

| Branch | Purpose |
|---|---|
| `main` | Integrated, always-building code. Phases land here when done. |
| `spec` | The specification: UI look, plan, acceptance criteria, `.max` format notes. Edit specs here, then merge into `main`. |
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

Spec changes:

```sh
git switch spec
# ...edit docs/spec/*...
git commit -am "spec: ..."
git switch main && git merge spec
```

Each phase branch has a checklist in `docs/phases/`; tick items off as you go.
