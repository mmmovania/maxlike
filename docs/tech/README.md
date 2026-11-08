# Technical documentation

One document per phase, written when the phase is finished. Each explains the
techniques used, the reasoning behind the design, the problems found and how they were
solved, and exercises to try. Measured values live in `docs/spec/01-ui-look.md`; the
per-step checklists in `docs/phases/`.

| Phase | Document | Topics |
|---|---|---|
| 0 — Skeleton | [phase0-tech-docs.md](phase0-tech-docs.md) | Win32 message loop, the window wrapper, painting, DPI, manifest and classic controls, theme, CMake |
| 1 — Frame layout | [phase1-tech-docs.md](phase1-tech-docs.md) | Measuring, pure layout + tests, child windows, menus, custom controls, toolbar, combos, tooltips, flyouts, sliders, subclassing, pixel comparison, Git lessons |
| 2 — Viewports | [phase2-tech-docs.md](phase2-tech-docs.md) | OpenGL on Win32 (WGL, pixel formats, contexts, GLAD), shaders and buffers, fitting formulas and cameras to measurements, 3D maths, pixel-exact drawing, menus on demand, dividers, navigation |
| 3 — Scene core | [phase3-tech-docs.md](phase3-tech-docs.md) | Nodes and objects, ownership, transforms and quaternions, primitives built Max's way (and how R3.1's counts corrected them), modifier stack, smoothing groups, lighting and the depth buffer, undo/redo |
| 8 — `.max` import | [phase8-tech-docs.md](phase8-tech-docs.md) | Testing against independent implementations, compiler safety nets, robustness sweeps, surveys before code, end-to-end checks against Max, reading an error's shape, sentinels in parsers, restoring R3's views |
| 7 — Modify panel | [phase7-tech-docs.md](phase7-tech-docs.md) | Deformers as coordinate maps, verifying geometry from a screenshot, undo of structure, generalising with defaults, rollouts as data, sub-object editing at a stack level, two observations make a model, sentinels |
| 6 — Selection and transforms | [phase6-tech-docs.md](phase6-tech-docs.md) | Reusing a measured strip, selection as undoable state, picking what is drawn, identity vs. labels, reverse-engineering a rasteriser, rays and drag planes, rotations in a parent's frame, letting measurements question the plan |
| 5 — Interactive creation | [phase5-tech-docs.md](phase5-tech-docs.md) | Interaction as state machines, construction planes, live vs. recorded changes, behaviour vs. appearance, spinners, data-driven rollouts, one mechanism for two lists |
| 4 — Command panel | [phase4-tech-docs.md](phase4-tech-docs.md) | Measuring controls, painted vs. real controls, the rollup layout engine, near-standard edges, clipping, fonts and notifications, deciding what not to build yet |
