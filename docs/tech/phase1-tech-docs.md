# Phase 1 — Technical documentation

Phase 1 turned the empty Phase 0 window into the complete 3ds Max R3.1 frame: menu bar,
tab panel, main toolbar, time slider, track bar and status area, measured against a
reference screenshot. This document collects the explanations given while building it,
organised by topic. Measured values themselves live in `docs/spec/01-ui-look.md`; the
step-by-step record is `docs/phases/phase-1-frame-layout.md`.

Code at the end of Phase 1: tag `phase-1-done` (version 0.1.0).

## Contents
1. Method: measure, then build
2. Architecture: pure logic vs. Win32 code
3. Frame layout and child windows (1.1)
4. Menus (1.2)
5. Tab panel (1.3)
6. Main toolbar: data, icons, mouse (1.4a)
7. Combo boxes and tooltips (1.4b)
8. Flyouts (1.4c)
9. Time slider and track bar (1.5a)
10. Status area, left (1.5b)
11. Status area, right: generalising the toolbar (1.5c)
12. The whole-window check (1.6)
13. Testing
14. Git lessons
15. Open items
16. Exercises

## File map

| File | Purpose |
|---|---|
| `src/ui/Metrics.h` | 96-DPI layout constants, `metrics::Scale` (no `windows.h`) |
| `src/ui/FrameLayout.*` | `ComputeFrameLayout`: rectangles of all main regions |
| `src/ui/Bevel.*` | `FillSolid`, `DrawRing`, `DrawButtonRing`, `DrawFrameEdge` |
| `src/ui/Placeholder.*` | Labelled stand-in regions (viewports, command panel) |
| `src/ui/MenuSpec.*`, `MenuBuilder.*`, `CommandTable.*` | Menus as data, built into an `HMENU`; command names ↔ IDs |
| `src/ui/TabStrip.*`, `TabPanel.*` | Tab panel geometry and drawing |
| `src/ui/ToolbarSpec.*` | Toolbar items, `Config`s and layout |
| `src/ui/Toolbar.*`, `FlyoutStrip.*` | The toolbar control and its flyout popup |
| `src/ui/TimeSliderGeometry.*`, `TimeSlider.*`, `TrackBar.*` | Time slider and track bar |
| `src/ui/StatusLayout.*`, `StatusBar.*` | Status area |
| `res/resource.h`, `res/*.bmp`, `tools/make_icons.py` | Icon strips and their generator |
| `tests/*.cpp` | Six unit-test programs |

---

## 1. Method: measure, then build

**Measure, don't guess.** Every size and colour was read from the R3.1 reference
screenshot (`docs/reference/screenshots/max-r3.1-default-1920x1040.png`) by scanning
rows and columns for colour changes, dumping small areas as character maps
(`W` white, `s` shadow, `K` black, `.` face, …), and reading edges off those maps.
Values are recorded in **client coordinates** (relative to the top-left of the area
below the menu bar; 1920 × 998 in the reference).

**Describe rules, not positions.** A maximised window is one size; users resize. So
layouts are rules ("38-px toolbar, full width", "viewports fill what is left") that
reproduce the measured rectangles at 1920 × 998 and behave correctly at other sizes.

**Record what is there, even if irregular.** R3 has a 2-px gap before one combo but not
another, and two different bevel-corner conventions. These are recorded as explicit
gaps and separate helpers instead of being "corrected".

**Verify by pixel comparison.** Each step ends with the app running under Wine, the
screen captured and compared pixel by pixel with the reference (section 13).

---

## 2. Architecture: pure logic vs. Win32 code

Every control is split in two:
- a **pure** part (no `windows.h`): geometry, layout, data. Examples:
  `ComputeFrameLayout`, `TabRect`, `LayoutToolbar`, `PartsAt`, `ComputeStatusLayout`,
  `MainMenuSpec`, `CommandTable`. They use their own `ui::Rect`, so they compile and are
  unit-tested anywhere.
- a **window** part that draws and handles input using the pure part.

Because drawing and hit-testing share one geometry function, what you see and what you
click can never disagree. Tests check the pure part against measured values.

Other patterns used throughout:
- **One command system.** Menus, toolbar buttons, status buttons and combos all report
  command *names* (`"edit.undo"`) to `MainFrame::OnCommand`.
- **Callbacks for events.** Our own controls report with `std::function` members
  (`onTabChanged`, `onCommand`, `onFrameChanged`, …). Setter functions (`SetFrame`,
  `SetActiveTab`) deliberately do *not* fire callbacks; only user input does, which
  prevents feedback loops.
- **Parent owns, children borrow.** Fonts are created by `MainFrame` and lent to
  children with `WM_SETFONT`.

---

## 3. Frame layout and child windows (step 1.1)

### Layout rules (at 100%)
From the top: 4-px gap, 15-px tab panel, 38-px toolbar (full width). From the bottom:
50-px status area (full width). Right column: 184-px command panel between toolbar and
status area. Left column from the bottom up: 16-px track bar, 17-px time slider; the
viewports fill the rest. `ComputeFrameLayout(w, h, dpi)` encodes this, with
`std::min`/`std::max` clamps so tiny windows never produce negative sizes.

### Child windows
- **Two kinds of class.** One C++ class (e.g. `Placeholder`) can have many instances; the
  Windows *window class* is registered once (`EnsureClassRegistered`) and all instances
  share one window procedure. The `GWLP_USERDATA` link routes messages to the right
  instance.
- **Styles.** `WS_CHILD` (inside the parent's client area, coordinates relative to it),
  `WS_VISIBLE`, `WS_CLIPSIBLINGS` (siblings don't paint over each other). The parent has
  `WS_CLIPCHILDREN` so its own fill doesn't flash over children during resizing.
- **Creation** in the parent's `WM_CREATE`; returning −1 cancels the whole frame.
  Children start at 0 × 0; the first `WM_SIZE` positions them.
- **Lifetime.** Destroying a parent destroys its children first; each child's
  `WM_NCDESTROY` clears its `hwnd_`.

### Layout on `WM_SIZE`
`lp` packs width (low 16 bits) and height (high 16 bits): `LOWORD`/`HIWORD`. Skip
layout when `wp == SIZE_MINIMIZED`. Move all children as one batch:
```cpp
HDWP dwp = BeginDeferWindowPos(n);
dwp = DeferWindowPos(dwp, child, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);  // each
EndDeferWindowPos(dwp);   // all move together, one repaint
```
Separate `SetWindowPos` calls would repaint each child separately (flicker, tearing).
Check `dwp` for null after each call (failure).

### Fonts with `WM_SETFONT`
The parent sends `WM_SETFONT` (font in `wp`, redraw flag in `LOWORD(lp)`); every
standard control understands it, and our controls implement it (plus `WM_GETFONT`).
Safe replacement order: **create new → hand it to every child → delete old.** Deleting
first leaves children briefly holding a deleted handle (silent failures, recycled
handles). On a DPI change the frame recreates fonts, then `SetWindowPos` triggers
`WM_SIZE`, which re-lays out at the new DPI.

### Drawing without leaking: `DC_BRUSH`
```cpp
SetDCBrushColor(dc, c);
FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
```
A stock object owned by Windows whose colour is a DC setting: no create/delete per
fill, nothing to leak. `FillSolid` wraps it; all 1-px lines are `FillSolid` calls.

### Two bevel corner conventions
- **Windows convention** (`DrawRing`, `DrawEdge`): the bottom-right colour owns both odd
  corners (top-right and bottom-left).
- **R3 convention** (`DrawButtonRing`): the top-left colour owns the left column and the
  top row except its last pixel; the bottom-right colour owns the right column and the
  bottom row except its first pixel, so the **bottom-left corner belongs to the
  top-left colour**.

R3 mixed both (standard Windows parts and its own custom parts). Only measuring tells
which is which.

### Small details
- `DT_VCENTER` only works with `DT_SINGLELINE`; `DT_NOPREFIX` keeps `&` literal.
- `CS_HREDRAW | CS_VREDRAW` repaint the whole window on resize (the size labels update).
- `kSystemFace = 0xFFFFFFFF` is a safe "use the system colour" marker: a real
  `COLORREF` always has a zero top byte.

---

## 4. Menus (step 1.2)

### Win32 menus
- `CreateMenu()` makes a **menu bar**; `CreatePopupMenu()` a dropdown/submenu.
- `AppendMenuW(menu, MF_STRING, id, L"&Open...")`, `MF_SEPARATOR`, and
  `MF_POPUP | MF_STRING` with the *submenu handle* as the ID argument; the parent then
  owns the submenu.
- `&` marks the mnemonic; Windows draws the underline and handles Alt+letter.
- A top-level window's menu is passed in `CreateWindowExW`'s "menu or ID" slot (for
  child windows the same slot holds the control ID). Once attached, `DestroyWindow`
  destroys the menu; only if creation fails must you call `DestroyMenu`.
- The menu bar is non-client area: attaching it shrinks the client area, Windows sends
  `WM_SIZE`, and the layout adapts (the viewport area went from 877 to the measured 858).

### Menus as data
`MenuSpec.cpp` describes the menus as a nested list (`Sub`, `Cmd`, `Sep`);
`BuildMenuBar` turns it into `HMENU`s with one recursive function. Benefits: corrections
are list edits (the project owner corrected all items against R3.1); the list is
unit-tested (the test caught a duplicate Alt-letter and later a duplicated command
name); it can later drive Customize UI.

### Commands by name
`CommandTable` assigns IDs from 40001 (clear of dialog/control IDs and of system
commands ≥ 0xF000) as the menu is built, and maps back:
```cpp
case WM_COMMAND:
    if (HIWORD(wp) <= 1 && lp == 0) {           // 0 = menu, 1 = accelerator
        const std::string& name = commands_.Name(LOWORD(wp));
        if (!name.empty()) { OnCommand(name); return 0; }
    }
```
`WM_COMMAND` is shared with controls (notification code in `HIWORD(wp)`, control handle
in `lp`), hence the `lp == 0` check.

`file.exit` posts `WM_CLOSE` (the same path as the close button), so a future "save
changes?" prompt covers both.

---

## 5. Tab panel (step 1.3)

### Why custom-drawn
R3's tabs: fixed 89-px width, bluish-grey `#A0A0B3` inactive fill, 1-px chamfered top
corners, a black right edge only on the active tab, the active tab open towards the
toolbar. The standard tab control cannot be made to look like this. When the look is
the point, drawing your own is less work than fighting a stock control.

### Geometry and drawing
`TabStrip.*` (pure): labels, `TabRect(i)`, `TabAtX(x)`. `TabPanel.*`: drawing and clicks.

The drawing is a row-by-row **pixel recipe**, written as a comment first, then coded one
`FillSolid` per line:
```
row 0      : highlight cols 2..w-4, dark at col w-3         (chamfer)
row 1      : highlight col 1, fill cols 2..w-3, dark col w-2
rows 2..h-2: highlight col 0, fill cols 1..w-2, dark col w-1
row h-1    : inactive -> highlight across; active -> col 0 only (open)
```
Positions scale with DPI; **lines stay 1 px** at every DPI (crisp, like classic Windows).

### Events and input
- `std::function<void(int)> onTabChanged`, set by `MainFrame` with a lambda capturing
  `this`. `SetActiveTab` does not fire it.
- Mouse coordinates: `GET_X_LPARAM(lp)` (from `<windowsx.h>`), never `LOWORD`: on
  multi-monitor setups coordinates can be negative.
- `MainFrame::Regions()` returns `win::Window*`, so layout and font loops work for every
  control type.

Result: 0 differing pixels in the tab panel (text masked).

---

## 6. Main toolbar (step 1.4a)

### The toolbar as data
`ToolbarSpec.cpp` lists items left to right. Kinds: `Button` (command, tooltip, icon,
optional flyout or radio group), `Separator` (6 px, etched line at +3/+4), `Gap`
(explicit spacing), `Combo`. `LayoutToolbar` adds widths from x = 4. Buttons 32 × 31 at
y = 4; icons 24 × 24 at (4, 3). A test checks all 27 buttons, 4 combos and 11 separator
lines against the screenshot.

### Icons as a resource strip
- One bitmap, 24 × 24 cells side by side (`res/toolbar_icons.bmp`).
- `res/resource.h` gives it an ID; `maxlike.rc` embeds it (`BITMAP`); the code loads it
  with `LoadImageW(..., IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)` (exact pixels, no
  conversion).
- **Key-colour transparency:** magenta `#FF00FF` marks transparent pixels;
  `TransparentBlt` copies everything except that colour (link `msimg32`). At 150% it
  stretches with nearest-neighbour scaling.
- A **memory DC** (`CreateCompatibleDC`) holds the strip during one paint: select, use,
  restore, delete.
- Icons are original art generated by `tools/make_icons.py` (Pillow, no anti-aliasing).
  Edit the script or paint replacements, keeping the grid, magenta and enum order.

### Mouse handling
1. **Hover.** `WM_MOUSEMOVE` → hit test → `hot_`; repaint only the two changed buttons
   (`InvalidateItem`). `WM_MOUSEMOVE` stops when the mouse leaves, so ask for
   `WM_MOUSELEAVE` with `TrackMouseEvent(TME_LEAVE)`; the request is one-shot
   (`trackingLeave_`).
2. **Press and capture.** `SetCapture(Hwnd())` on mouse-down: all mouse messages come to
   us even outside the window, so a drag-off release is still seen. The button is shown
   sunken only while `pressed_ == hot_`.
3. **A click completes on release** over the same button; dragging away cancels.
4. **`WM_CAPTURECHANGED`** resets the state if something else takes the mouse (a dialog).
5. **Radio groups** (`"mode"`, `"axis"`): activating a grouped button checks it and
   unchecks the rest of its group; only changed buttons repaint. A test enforces exactly
   one checked button per group.

The toolbar only shows which button is selected and reports the command; the "current
tool" will live in application state later.

### Measured button looks
| State | Look |
|---|---|
| Normal | flat, icon only |
| Hover | thin raised ring (unverified guess) |
| Pressed / checked | outer ring shadow/white, inner ring face/white (R3 corners); fill `#C0FFC0` for the active mode, `#E1E1E1` for other checked buttons, face while pressed |
| Flyout | embossed notch: white band + black triangle (sunken: black band + grey triangle), from a table of measured cells |

The notch is the clearest "measure, don't guess" case: a 3-pixel triangle was the first
guess; the comparison revealed the white band.

---

## 7. Combo boxes and tooltips (step 1.4b)

### Borrowing standard controls
R3's combos are exactly the classic Windows combo (`#A0A0A0`/white, `#696969`/`#E3E3E3`
edges). Real `COMBOBOX` children give the dropdown, keyboard navigation and
accessibility for free.
- `CBS_DROPDOWNLIST` (pick only) vs `CBS_DROPDOWN` (with an edit box).
- Control ID in the `(HMENU)` slot: `kComboIdBase + index`.
- Filled with `CB_ADDSTRING`; selected with `CB_FINDSTRINGEXACT` + `CB_SETCURSEL`.
- **Quirk:** the height passed to `SetWindowPos` is the *dropped-down* height; the closed
  height comes from the font (21 px with 8 pt MS Sans Serif, as measured).
- Notifications arrive as `WM_COMMAND` (ID in `LOWORD(wp)`, code in `HIWORD(wp)`):
  `CBN_SELCHANGE` (read text with `CB_GETLBTEXTLEN`/`CB_GETLBTEXT`), `CBN_CLOSEUP`
  (give focus back to the main window so shortcut keys keep working).

### Tooltips
A tooltip control registers **tools**: a *rectangle* of a window (`uId` = index, `rect`,
updated with `TTM_NEWTOOLRECTW` on relayout) or a *whole child window*
(`TTF_IDISHWND`, `uId` = its handle). `TTF_SUBCLASS` lets it watch those windows' mouse
messages itself. It is created with the toolbar as owner, so it dies with it.
`InitCommonControlsEx(ICC_WIN95_CLASSES)` registers the class; link `comctl32`.

**The size trap.** `ti.cbSize = TTTOOLINFOW_V2_SIZE`, not `sizeof(ti)`. The headers
describe the newest structure (a v6 field at the end); comctl32 v5, used for the classic
look, rejects the larger size and `TTM_ADDTOOL` fails silently: no tooltips at all.

---

## 8. Flyouts (step 1.4c)

- **Click vs. hold:** mouse-down on a flyout button starts a one-shot `SetTimer` (300 ms,
  unverified). Release first → `KillTimer`, normal click. Timer fires while still held →
  open the strip. Timers are messages (`WM_TIMER`); kill them to make them one-shot.
- **The strip is a popup** (`WS_POPUP`, screen coordinates via `ClientToScreen`) because
  it must extend outside the toolbar. `WS_EX_TOOLWINDOW` (no taskbar button),
  `WS_EX_TOPMOST`, `WS_EX_NOACTIVATE` + `MA_NOACTIVATE` in `WM_MOUSEACTIVATE` (never takes
  focus). For a popup the "parent" argument is the **owner**: always above it, destroyed
  with it.
- **The strip never touches the mouse.** The toolbar keeps capture for the whole gesture
  and tells the strip which cell is hot (`CellAt`, `SetHot`).
- **Re-entrancy trap:** `ReleaseCapture()` sends `WM_CAPTURECHANGED` *before it returns*,
  and that handler resets state. Read the chosen cell first, then release. Always ask
  "can this call re-enter my window procedure?"
- **Changing the face:** `ChooseAlternative` copies the alternative's command, tooltip and
  icon into the toolbar's own item. The tooltip stored a *pointer* to the old text, so
  it must be told (`TTM_UPDATETIPTEXTW`).

### Build dependency bug
New icons showed as a black square. `toolbar_icons.bmp` is read by `maxlike.rc`, but
CMake does not look inside `.rc` files, so it never rebuilt the resource; the `.exe` kept
the old 27-icon strip and drawing icon #30 read past its end. Fix:
```cmake
set_source_files_properties(res/maxlike.rc PROPERTIES OBJECT_DEPENDS
    ".../toolbar_icons.bmp;.../status_icons.bmp;.../maxlike.manifest;.../resource.h")
```
Lesson: if the build tool cannot see an input, it cannot rebuild when it changes. "I
changed it but nothing happened" → suspect a dependency; confirm with `--clean-first`.

---

## 9. Time slider and track bar (step 1.5a)

### Two directions of mapping
The `[<][ 0 / 100 ][>]` block (16 + 125 + 16 = 157 px) slides along a `#E1E1E1` trough.
- `PartsAt(frame)`: value → position, `(frame − start) × travel / range`, rounded.
- `FrameAtBlockLeft(x)`: position → value, rounded to the nearest frame, clamped.

A **round-trip test** checks every frame 0–100 maps to a position and back to itself.
Wrong rounding makes frames unreachable or makes the thumb jump on first touch.

### Interaction
- **Drag offset:** store `mouseX − blockLeft` on press; while dragging, the block goes to
  `mouseX − offset`, so the grabbed point stays under the cursor.
- **Trough click** sets the offset as if the thumb's centre had been grabbed, jumps, and
  becomes a thumb drag: two features, one code path.
- **Auto-repeat with one timer:** step once and `SetTimer(id, 400 ms)`; on each
  `WM_TIMER` step and `SetTimer(id, 50 ms)`. Calling `SetTimer` with an existing ID
  replaces the interval.
- **One exit path:** `WM_LBUTTONUP` only calls `ReleaseCapture()`; all cleanup (stop
  timer, clear state, repaint) lives in `WM_CAPTURECHANGED`, which every ending produces.

### Looks
Arrow buttons use the Windows corner convention (`DrawRing`: white/`#696969`,
`#E3E3E3`/`#A0A0A0`) with a 1-px chevron; the thumb uses the R3 convention with a
**black** bottom-right edge. Track bar: rows white, white, shadow, `#E1E1E1` band,
white, shadow; a `[` marker at the current frame (meaning unverified). Both bars:
0 differing pixels.

---

## 10. Status area, left (step 1.5b)

### Right-anchored layout
X/Y/Z, grid, lock and Time Tag keep a fixed distance from the right edge; prompt lines
stretch. `StatusLayout` stores right-side positions as offsets from the right
(`left = width − Scale(591)`). Pattern for any "fixed parts + stretchy part" layout:
pin each fixed part to its edge, let the stretchy part fill the gap.

### Subclassing a standard control
A single-line `EDIT` ignores Enter (it beeps). Replace its window procedure, handle the
one message, forward the rest:
```cpp
oldProc = (WNDPROC)SetWindowLongPtrW(edit, GWLP_WNDPROC, (LONG_PTR)&MyProc);
// in MyProc: if (msg == WM_CHAR && wp == VK_RETURN) { ...; return 0; }   // no beep
return CallWindowProcW(oldProc, h, msg, wp, lp);
```
The edit's own `GWLP_USERDATA` holds our `this`, as in the wrapper.

### Colouring a standard control: `WM_CTLCOLOR…`
Controls ask their parent for colours just before painting: read-only edits send
`WM_CTLCOLORSTATIC`, editable ones `WM_CTLCOLOREDIT`. Set colours on the DC, return a
background brush. The brush must live as long as the control can paint: create once,
delete in the destructor (creating one per message is a GDI leak).

### Cursor: `WM_SETCURSOR`
Over the splitter, set `IDC_SIZEWE` and return `TRUE`; otherwise let the default happen.

### DPI-proof dragging
The listener width is stored at 100% and mouse deltas are converted with
`dx × 96 / Dpi()`, so the proportion survives moving to another monitor.

### Comparison detail
Edit-box edges are drawn from system colours, and Wine's greys differ (`#A6A6A6` vs
`#A0A0A0`). The comparison maps Wine's colours to Windows 10's instead of skipping those
pixels. Result: 0 differing pixels in the 1.5b area. The empty bottom-row field was
identified by the project owner as **Time Tag**.

---

## 11. Status area, right: generalising the toolbar (step 1.5c)

### Generalise instead of copying
The status buttons needed everything the toolbar already did. Only *settings* differed:

| Setting | Main toolbar | Status groups |
|---|---|---|
| Layout | left-to-right flow | measured positions (`Item::place`) |
| Icons | 24 px, `toolbar_icons.bmp` | 16 px, `status_icons.bmp` |
| Frame edge columns | yes | no |
| Flyout notch | as measured | 1 px further right |
| Follows tab panel | yes | no |

These became `toolbar::Config`; `Toolbar::Create(parent, config)`. The behaviour code
did not change, so a fix there fixes all three toolbars (`MainToolbarConfig`,
`StatusSnapConfig`, `StatusAnimConfig`).

New item features, each a small addition to `Item`: `toggle` (click flips checked),
a text label instead of an icon with R3's 2-ring raised bevel (Animate), `alarm` (red
when on), and `Kind::Edit` (a borderless `EDIT` inside a frame the toolbar draws:
outer shadow/white, inner black/face, measured).

### One value, several views
The current frame can be changed from the slider, playback buttons and frame field, and
is shown by the slider label, track bar and field. Every change goes through the time
slider (which clamps) and `MainFrame::ShowFrame` updates all views, so typing 500 shows
100 everywhere. In Phase 3 the owner of the frame moves into the scene.

### Select on focus
Classic `EDIT`s have no Ctrl+A, so typing "42" into "100" gave "10042" → clamped to 100.
The field selects its contents on `WM_SETFOCUS`, **posted** (`PostMessageW(EM_SETSEL)`)
so it runs after the focusing click has placed the caret.

---

## 12. The whole-window check (step 1.6)

1. **Reproduce the reference conditions:** a Wine virtual desktop of exactly
   1920 × 1040, window maximised (the reference is a maximised window above a 40-px
   taskbar).
2. **Align by anchors:** Wine's title and menu bars are shorter, so top parts are
   aligned at the tab panel and bottom parts at the track bar, as the layout itself
   anchors them.
3. **Compare what should match, and record what was skipped:** text, icon art, Wine's
   system greys, and regions later phases replace.

Results: tab panel + toolbar 0/75,602; command panel frame 0/5,045; bottom bars
236/91,282 (229 = command panel rollouts, Phase 4; 7 = icon art). The first run found
two errors no single-region check could see: frame pixels in the gap above the tab
panel (owned by no control) and the command panel's outer frame (now a measured recipe,
`Edge::CommandPanel`).

The version became **0.1.0** in CMake, the `.rc` and the manifest.

---

## 13. Testing

### Unit tests
Six console programs, registered with CTest (`maxlike_add_test` in `CMakeLists.txt`):
`test_frame_layout`, `test_menu_spec`, `test_tab_strip`, `test_toolbar_spec`,
`test_time_slider`, `test_status_layout`. Run:
```sh
ctest --test-dir build -C Debug --output-on-failure
```
They check measured positions, well-formedness rules (unique commands/icons, one checked
per radio group, separators never doubled, unique mnemonics) and properties
(round trips, no negative sizes, no gaps at 150%).

### Pixel comparison (how it was done here)
Cross-compile with MinGW-w64, run under Wine on a virtual X display (Xvfb), capture with
`xwd`, compare in Python (Pillow) against the reference: align, mask, normalise, count,
then group differences by colour pair and position to see their cause. Popups (menus,
tooltips, combo lists, flyout strips) appear black on that display, so their contents
are verified on Windows.

---

## 14. Git lessons

| Situation | What to do |
|---|---|
| Apply a patch | `git am file.patch`; it keeps the patch's author, so `git commit --amend --no-edit --reset-author` **before pushing**. |
| Several unpushed commits to re-author | `git rebase origin/<branch> --exec "git commit --amend --no-edit --reset-author"` |
| `patch does not apply` | Your files differ from what the patch expects: uncommitted edits (`git stash`, apply, `git stash pop`), a commit the patch was not built on, or an old download with the same name (check `findstr /B "From " file.patch`). |
| `previous rebase directory … still exists` | A failed `git am` is still in progress: `git am --abort`. |
| `git am --abort` | Also moves the branch back to where that `am` started; may rewind a later reset. |
| Diverged after amending | Amending a pushed commit creates a new ID; never amend after pushing. Recover with `git fetch` and `git reset --hard origin/<branch>` once `git diff origin/<branch> HEAD --stat` shows nothing. |
| Run steps only on success | Chain with `&&`: `git am p.patch && git commit --amend ...`. |
| `origin/…` looks stale | It is a snapshot from the last fetch/push: `git fetch` first. |
| Rebase | Replays your commits on a new base with new IDs; the remote then needs `git push --force-with-lease` (safe force: refuses if someone else pushed). Never `git pull` after a rebase. |
| Identity | `git config user.name` / `user.email` inside the repo (the setup identity "maxlike setup" came with the initial zip). |
| Visual Studio cache | `*.aps` is ignored. |

---

## 15. Open items (unverified against R3.1)

- Hover and pressed looks of toolbar buttons; flyout strip look and hold delay.
- Tooltip texts and combo entries of the main toolbar; flyout alternatives.
- Names, tooltips and flyouts of all status-area buttons; Animate's "on" colour.
- Time Tag click menu (not implemented), track-bar `[` marker meaning, time-slider
  repeat timing and trough-click behaviour.
- Menu mnemonics (items are verified).
- Viewport frames (Phase 2); command panel tabs and rollouts (Phase 4).
- Tab-panel strips other than Main Toolbar: built with their features (decision B,
  `docs/spec/02-plan.md`).

---

## 16. Exercises from the lessons

| Step | Try |
|---|---|
| 1.1 | Change `CommandPanelWidth` to 200; `ctest` shows which rectangles moved. Remove `WS_CLIPCHILDREN` and resize fast. Comment out the `WM_SETFONT` loop. Breakpoint on `EndDeferWindowPos`. |
| 1.2 | Duplicate a command name; `test_menu_spec` fails. Alt+V, G opens a submenu. Add a menu item: it reports its name with no other code. |
| 1.3 | Set `kPitch` to 100 (test fails, clicks still correct). Remove the chamfer. |
| 1.4a | Cancel a click by dragging off. Make two axis buttons checked (test fails). Comment out `SetCapture` (button sticks). |
| 1.4b | Use `sizeof(ti)` instead of `TTTOOLINFOW_V2_SIZE` (no tooltips). Remove the focus-return line. Add a combo entry. |
| 1.4c | Change `kFlyoutDelayMs`. Add an alternative reusing an icon (test fails). Edit a pixel of the bitmap and watch the resource rebuild. |
| 1.5a | Remove `+ travel / 2` (round-trip test fails). Change the range to 0–30. Tune `kRepeatRateMs`. |
| 1.5b | Undo the subclass (Enter beeps). Recolour `kListenerBg`. Narrow the window. |
| 1.5c | Change the hover colour once, see three toolbars change. Swap two buttons' positions. Type 500 in the frame field. |
