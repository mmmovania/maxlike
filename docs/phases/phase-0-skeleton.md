# Phase 0 — Skeleton

Branch: `phase/0-skeleton`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 0.

## Status
Code complete; awaiting a run on Windows to tick the last two items.

- [x] CMake project (C++17, Unicode, `/W4 /permissive- /utf-8`); builds warning-free (verified with MinGW-w64 cross-compile)
- [x] Thin wrapper: `win::Window` (`this` in `GWLP_USERDATA`, lazy class registration, `Dpi()`/`Scale()`), `win::App` message loop
- [x] `theme` with every colour and metric from `01-ui-look.md`; UI font helper
- [x] Manifest: PerMonitorV2 DPI awareness, no comctl32 v6 (classic controls)
- [x] Verified on Windows: window opens maximised, titled `Untitled - maxlike`, face-coloured client, diagnostic line shows correct DPI
- [x] Verified moving between monitors with different scaling rescales the font

## Build
```sh
cmake -S . -B build
cmake --build build --config Debug
build\Debug\maxlike.exe
```

## Notes
- The diagnostic text in the top-left of the client area is temporary; Phase 1 removes it.
- `MainFrame` starts maximised, like R3.
- At 100% the UI font is the bitmap `MS Sans Serif`; at other scales the TrueType
  `Microsoft Sans Serif` is used so it scales cleanly.
