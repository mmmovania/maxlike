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
