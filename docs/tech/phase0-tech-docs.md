# Phase 0 — Technical documentation

Phase 0 built the skeleton every later phase stands on: a CMake project, a thin C++
wrapper over Win32 windows, a DPI-aware main window, and one file that defines the look.
This document collects the explanations given while building it, organised by topic.

Code at the end of Phase 0: tag `phase-0-done`.

| File | Purpose |
|---|---|
| `CMakeLists.txt` | Build definition |
| `res/maxlike.manifest`, `res/maxlike.rc` | Manifest (DPI awareness, classic controls) and version info |
| `src/main.cpp` | Entry point |
| `src/win/App.*` | Instance handle and message loop |
| `src/win/Window.*` | The thin window wrapper |
| `src/ui/Theme.*` | Colours, metrics and the UI font |
| `src/ui/MainFrame.*` | The main window |

---

## 1. How a Win32 program runs

### Windows calls you
A console program runs top to bottom. A GUI program mostly waits. Whenever something
happens to a window, Windows packages it as a **message**: a number (`WM_MOUSEMOVE`,
`WM_SIZE`, `WM_PAINT`, `WM_CLOSE`, …) plus two pointer-sized parameters, `wParam` and
`lParam`, whose meaning depends on the message. The program reacts to the messages it
cares about and passes the rest to Windows' default handling.

The path of every message:

```
Windows event → thread message queue → App::Run (GetMessage / DispatchMessage)
  → Window::StaticWndProc → MainFrame::HandleMessage → DefWindowProcW (if unhandled)
```

### The entry point
CMake marks the program `WIN32`, so it starts in `wWinMain` (the `w` = Unicode) and no
console window appears. `main.cpp` stores the `HINSTANCE` (a handle identifying the
`.exe`, needed when registering windows), creates the main frame, shows it maximised and
enters the message loop until the app quits.

### Window classes
Before a window can be created, Windows needs a **window class**: a registered template
(not a C++ class) naming a cursor, a style, and the **window procedure** that handles
the window's messages. `Window::EnsureClassRegistered()` registers a class the first time
a window of that type is created, using `ClassName()` (e.g. `L"maxlike.MainFrame"`).
Every class points at the same procedure, `Window::StaticWndProc`.

### The message loop (`App::Run`)
```cpp
while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);   // key presses -> WM_CHAR text input
    DispatchMessageW(&msg);   // calls the window procedure of msg.hwnd
}
```
`GetMessageW` sleeps until a message arrives (an idle app uses no CPU) and returns 0 on
`WM_QUIT`, which ends the loop and the program.

---

## 2. The thin wrapper: connecting Windows to C++ objects

### The problem
Windows can only call a plain function; it knows nothing about C++ objects. But every
window should be an object with its own data and handler. When a message reaches the
shared `StaticWndProc`, which object does it belong to?

### The solution: store `this` on the window
```cpp
// Window::Create — the last argument travels to WM_NCCREATE
CreateWindowExW(..., App::Instance(), this);
```

`Window::StaticWndProc`, line by line:

```cpp
 1  LRESULT CALLBACK Window::StaticWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
 2      Window* self = nullptr;
 3      if (msg == WM_NCCREATE) {
 4          auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
 5          self = static_cast<Window*>(cs->lpCreateParams);
 6          self->hwnd_ = h;
 7          SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
 8      } else {
 9          self = reinterpret_cast<Window*>(GetWindowLongPtrW(h, GWLP_USERDATA));
10      }
12      if (!self)
13          return DefWindowProcW(h, msg, wp, lp);
15      LRESULT r = self->HandleMessage(msg, wp, lp);
16      if (msg == WM_NCDESTROY) {
17          SetWindowLongPtrW(h, GWLP_USERDATA, 0);
18          self->hwnd_ = nullptr;
19      }
20      return r;
21  }
```

- **Line 1.** The signature Windows requires. `LRESULT` is a pointer-sized result whose
  meaning depends on the message. `CALLBACK` is the calling convention (`__stdcall`,
  relevant on 32-bit). `HWND h` says which window; `msg` is the message number. The
  function is a `static` member because Windows needs a plain function address (a
  normal member has a hidden `this`), yet it can still touch private members.
- **Line 3.** `WM_NCCREATE` is sent while `CreateWindowExW` is still running. It is not
  the very first message (`WM_GETMINMAXINFO` comes earlier), but the first carrying our
  pointer.
- **Lines 4–5.** For `WM_NCCREATE`, `lp` is the address of a `CREATESTRUCTW`.
  `reinterpret_cast` turns the integer into a pointer. `lpCreateParams` is the last
  argument of `CreateWindowExW` (our `this`), handed back untouched.
- **Line 6.** Store the handle now: `CreateWindowExW` has not returned yet, but
  `WM_CREATE` (where `MainFrame` calls `Dpi()`) also arrives before it returns.
- **Line 7.** Every window has a spare pointer-sized slot, `GWLP_USERDATA`. The `Ptr`
  functions and `LONG_PTR` are needed for 64-bit safety.
- **Line 9.** Every later message: read the pointer back (very cheap).
- **Lines 12–13.** Safety net for messages before `WM_NCCREATE` and after the link has
  been cleared: let Windows handle them instead of dereferencing a bad pointer.
- **Line 15.** `HandleMessage` is virtual, so the call reaches `MainFrame::HandleMessage`
  (or any later window class). Rule: if you ever handle `WM_NCCREATE` yourself, return
  TRUE, or creation is cancelled.
- **Lines 16–19.** `WM_NCDESTROY` is the last message a window gets. Unlink both ways so
  nothing can call into a destroyed object.

The destructor clears the slot **before** `DestroyWindow`, for the opposite order (the
object dies first): by the time `~Window` runs, the derived part is gone, so the final
messages must take the line 12–13 path.

Lifecycle in one line:
```
GETMINMAXINFO (no self) → NCCREATE (link) → CREATE … every message … → NCDESTROY (unlink)
```

### Forgetting the fallback
Unhandled messages must reach `DefWindowProcW` (via `Window::HandleMessage`). Without it
the window cannot be moved, resized or closed: the classic Win32 bug.

---

## 3. `MainFrame`'s messages

| Message | What `MainFrame` does |
|---|---|
| (creation) | `WS_OVERLAPPEDWINDOW` (title bar, border, buttons) + `WS_CLIPCHILDREN` (don't paint over child windows; matters from Phase 1). Created at 1280×800, shown maximised; 1280×800 is the restore size. |
| `WM_CREATE` | One-time setup (the UI font needs the window's DPI). Return 0 to continue, −1 to cancel creation. |
| `WM_ERASEBKGND` | Return 1: "background handled". Windows normally erases, then paints; both drawing causes flicker. We paint everything in `WM_PAINT`. |
| `WM_PAINT` | See below. |
| `WM_GETMINMAXINFO` | Minimum drag size (800×600 at 100%, scaled). |
| `WM_DPICHANGED` | Accept the suggested rectangle, rebuild the font, repaint (section 4). |
| `WM_SETTINGCHANGE`, `WM_SYSCOLORCHANGE` | Repaint, because the face colour comes from the system. |
| `WM_DESTROY` | `PostQuitMessage(0)`. |

### Painting
```cpp
PAINTSTRUCT ps;
HDC dc = BeginPaint(Hwnd(), &ps);              // 1
FillRect(dc, &ps.rcPaint, theme::FaceBrush()); // 2
HGDIOBJ old = SelectObject(dc, uiFont_);       // 3
SetBkMode(dc, TRANSPARENT);
TextOutW(dc, x, y, text, len);
SelectObject(dc, old);                         // 4
EndPaint(Hwnd(), &ps);                         // 5
```
1. `BeginPaint` returns a device context (the canvas) and fills `ps.rcPaint` with the
   area that actually needs repainting.
2. Fill only that area.
3. GDI is a state machine: *select* the font into the DC; `SelectObject` returns the
   previous one. `TRANSPARENT` stops text from painting a box behind each letter.
4. **Always restore what you replaced.** Leaving your font selected, or deleting a font
   that is still selected, is a classic GDI leak.
5. `EndPaint` releases the DC and marks the area valid. Forget it and Windows sends
   `WM_PAINT` forever (one CPU core busy).

### How the app closes
```
click X → WM_CLOSE → DefWindowProcW calls DestroyWindow → WM_DESTROY
  → PostQuitMessage(0) → WM_QUIT → GetMessageW returns 0 → wWinMain returns
```
`WM_CLOSE` is the last moment to cancel closing; a future "Save changes?" prompt goes
there.

---

## 4. DPI scaling

### Terms
Windows expresses scaling as DPI: **96 = 100%**, 120 = 125%, 144 = 150%, 192 = 200%.

### Three awareness modes
- **Unaware:** draws at 96 DPI; Windows stretches the result (blurry).
- **System aware:** correct on the main monitor at startup; stretched elsewhere.
- **Per-monitor v2:** the app handles every DPI itself; Windows sends `WM_DPICHANGED`
  and also scales the title bar, menus and scrollbars. maxlike uses this.

### Declared in the manifest
The manifest is XML embedded in the `.exe` (`1 RT_MANIFEST "maxlike.manifest"` in the
`.rc`). Windows reads it before any code runs, which is the earliest and most reliable
place to set DPI awareness:
```xml
<dpiAware ...>true/pm</dpiAware>              <!-- Windows 8.1: per-monitor v1 -->
<dpiAwareness ...>PerMonitorV2</dpiAwareness> <!-- Windows 10 1703+: wins when understood -->
```

### The scaling rule
All sizes are written at 96 DPI and scaled where used:
```cpp
int Scale(int px96) const { return MulDiv(px96, Dpi(), 96); }  // 180 -> 225 at 120 DPI
```
`MulDiv` rounds correctly and cannot overflow in the intermediate product.

### `WM_DPICHANGED`
Arrives when the window moves to a monitor with different scaling, or the setting
changes while running. `wParam` = new DPI; `lParam` = a suggested rectangle that keeps
the same physical size. Accept it with `SetWindowPos`, rebuild fonts (a font is created
for one pixel size), repaint.

### Font sizing
```cpp
lf.lfHeight = -MulDiv(8, dpi, 72);   // 8 pt -> pixels; negative = character height
```
At 100% the UI font is the bitmap **MS Sans Serif** (R3's font, exists only in fixed
sizes). At other scales the TrueType **Microsoft Sans Serif** (same design) is used so it
scales cleanly.

### Testing DPI with one monitor
Task Manager → Details → right-click a column header → *Select columns* → *DPI
Awareness* shows "Per-Monitor (v2)". Changing Settings → Display → Scale while the app
runs triggers `WM_DPICHANGED` (the Phase 0 diagnostic line showed 96 → 120).

---

## 5. The manifest's other half: classic controls

Windows ships two versions of the common controls side by side:
- **Version 5** (default): the classic look of 3ds Max R3: raised grey buttons, square
  tabs, sunken edits.
- **Version 6** (opt-in via a manifest dependency): Windows 10 "visual styles".

maxlike deliberately does **not** opt in, so every standard control looks classic with
no custom drawing. Trade-off: a few v6-only features (e.g. `TaskDialog`); if ever
needed, opt in and disable theming per control with `SetWindowTheme(hwnd, L"", L"")`.
The title bar is drawn by Windows itself (DWM), so it keeps the Windows 10 look, as in
the R3.1 reference screenshot. `<supportedOS>` declares Windows 10/11, so Windows does
not apply compatibility shims.

---

## 6. `theme`: one place for the look

- `struct Rgb` stores each colour once, with `.Ref()` for GDI (`COLORREF`, byte order
  0x00BBGGRR, built with `RGB()`) and `.R()/.G()/.B()` floats for OpenGL.
- `inline constexpr` (C++17): compile-time constants that can live in a header included
  everywhere without duplicate-definition errors.
- Every constant maps to `docs/spec/01-ui-look.md`; unmeasured values carry a
  `TODO(spec)`.
- The face colour comes from `GetSysColor(COLOR_BTNFACE)` (`#F0F0F0` on Windows 10,
  `#D4D0C8` on Windows 2000), as R3 did. `GetSysColorBrush` returns a brush **owned by
  Windows**: never delete it.
- `CreateUiFont` returns a new `HFONT`: the caller owns and deletes it. GDI objects are
  a limited per-process resource (default cap 10,000); always ask "who deletes this?"
- Constants and helpers only, so a namespace rather than a class.

(From Phase 1 on, layout metrics live in `src/ui/Metrics.h`, without `windows.h`, so
they can be unit-tested.)

---

## 7. The CMake build

CMake generates a build system (a Visual Studio solution), which then compiles:
```sh
cmake -S . -B build                   # configure (out-of-source: everything in build\)
cmake --build build --config Debug    # build
```

| Line | Meaning |
|---|---|
| `project(maxlike VERSION … LANGUAGES CXX RC)` | Version; `RC` enables the resource compiler. |
| `CMAKE_CXX_STANDARD 17`, `…_REQUIRED ON`, `…_EXTENSIONS OFF` | Exactly C++17, no compiler extensions. |
| `add_executable(maxlike WIN32 …)` | GUI program, entry `wWinMain`. **Every new file must be listed.** Headers are listed so they appear in Visual Studio. |
| `target_include_directories(... src)` | `#include "win/Window.h"` paths are relative to `src/`. |
| `UNICODE _UNICODE` | Windows headers default to the `W` functions. |
| `WIN32_LEAN_AND_MEAN` | Skip rarely used parts of `windows.h` (faster builds). |
| `NOMINMAX` | Stop `windows.h` defining `min`/`max` macros, which break `std::max`. |
| `WINVER=0x0A00 _WIN32_WINNT=0x0A00` | Target Windows 10; exposes e.g. `GetDpiForWindow`. |
| `MAXLIKE_VERSION="${PROJECT_VERSION}"` | The version in code. |
| `/W4 /permissive- /utf-8` | High warnings (kept at zero), standard conformance, UTF-8 sources. |
| `/MANIFEST:NO` | Our manifest comes from the `.rc`; avoid a second one. |
| `-municode -static` (MinGW) | Used for the cross-compile checks. |

Known wart: the version also appears in `maxlike.rc` and the manifest; all three must be
changed together (done for v0.1.0).

---

## 8. Git workflow (as set up in Phase 0)

- `main` = integrated code; `spec` = specifications; `phase/N-…` = one branch per phase.
- A branch is only a **label on a commit**; commits belong to no branch. A branch
  "contains" every commit reachable through parent links.
- Finishing a phase: merge it into `main` (`--no-ff`), tag `phase-N-done`.
- Starting the next phase: `git switch phase/N+1-…` then `git rebase main`.
- `git branch -f <b> <commit>` moves a label anywhere (can orphan commits);
  `git merge --ff-only` moves it only forward (safe).
- Merge conflicts (add/add): both sides created the same file. Pick a side with
  `git checkout --ours|--theirs <file>`, `git add`, `git commit`.
- `cmd` shows UTF-8 text in the old code page (`—` as `ΓÇö`); `chcp 65001` fixes the
  display. The files are fine.

More Git lessons, learned the hard way in Phase 1, are in `phase1-tech-docs.md`.

---

## 9. Exercises from the lessons

| Lesson | Try |
|---|---|
| Message flow | Breakpoint in `StaticWndProc`; watch `WM_GETMINMAXINFO` (0x24, `self == nullptr`), `WM_NCCREATE` (0x81), `WM_NCCALCSIZE` (0x83), `WM_CREATE` (0x01). |
| Painting | Comment out `EndPaint`: one CPU core goes busy. Remove `SetBkMode(TRANSPARENT)`: boxes behind text. |
| Closing | Handle `WM_CLOSE` with a Yes/No `MessageBoxW` and call `DestroyWindow` only on Yes. |
| DPI | Task Manager DPI column; change Scale while running. |
| CMake | Change `project(VERSION)`: the code changes, *Properties → Details* does not (the wart). Remove `NOMINMAX` and use `std::max`. |
| Theme | Paint the client with `kViewportBg` instead of the face colour (create and delete the brush). |
