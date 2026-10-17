# Overview and technical decisions

## Goal
Recreate the look and workflow of 3D Studio MAX R3.1 on modern Windows, pixel-close at
100% scaling, and read R3-era `.max` files natively (port of the JS `max_loader`).

## Decisions
| Area | Choice | Why |
|---|---|---|
| Language | C++17, MSVC, Unicode | Native Win32, no runtime |
| UI | Raw Win32 + classic common controls behind a thin wrapper (`Window` base class, `this` in `GWLP_USERDATA`) | Exact classic bevel look; no heavy framework |
| Theming | No comctl32 v6 manifest, or `SetWindowTheme(hwnd, L"", L"")` | Keeps 1999-style 3D controls |
| Font | `MS Shell Dlg` / MS Sans Serif 8 pt | Matches R3 |
| Viewports | OpenGL 3.3 via GLAD, one shared `HGLRC`, four child windows with `CS_OWNDC` | Matches Max's per-viewport windows |
| Viewport text | `wglUseFontBitmaps` | No GDI-over-GL flicker |
| Build | CMake | VS and command line |
| `.max` import | Own CFB reader + zlib | Format already understood |
| Coordinates | Z-up, Max convention internally | Same as loader; no conversion |
| Redraw | On demand only | Same as Max |
| DPI | Per-monitor aware; all metrics are 96-DPI values scaled by `GetDpiForWindow/96` | Exact at 100%, clean elsewhere |
| Undo | From Phase 3 | Hard to retrofit |

## Out of scope / rules
- No Autodesk icons, bitmaps or name. All art is original.
- Writing real `.max` files is out of scope; save uses our own chunked format.
