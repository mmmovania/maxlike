# Phase 4 — Technical documentation

Phase 4 replaced the command panel's placeholder with R3.1's Create panel: six tabs,
the category row, the Standard Primitives dropdown, and the rollup with its Object Type
and Name and Color rollouts. The panel matches the reference screenshot pixel for pixel
(text and icons aside). This document collects the explanations given while building
it. Measurements: `docs/spec/01-ui-look.md`; step record:
`docs/phases/phase-4-command-panel.md`.

Code at the end of Phase 4: tag `phase-4-done` (version 0.4.0).

## Contents
1. Architecture and file map
2. Measuring a control
3. The tab row
4. The category row and dropdown
5. The rollup and rollouts
6. Name and Color
7. Painted controls vs. real controls
8. Results
9. Open items
10. Exercises

---

## 1. Architecture and file map

| Part | Files |
|---|---|
| Panel geometry (pure, tested) | `src/ui/CommandPanelLayout.*`: tabs, categories, dropdown, Object Type buttons, Name and Color boxes, category lists |
| Rollup layout (pure, tested) | `src/ui/Rollup.*`: title/frame positions, collapse, scroll limits, hit testing |
| The window | `src/ui/CommandPanel.*`: painting, clicks, scrolling, the dropdown, the colour picker |
| Icons | `res/panel_tab_icons.bmp` (22 × 20), `res/panel_category_icons.bmp` (16 × 16), drawn by `tools/make_icons.py` |
| Tests | `test_command_panel_layout`, `test_rollup` |

`MainFrame` owns a `CommandPanel` where the placeholder was; the panel keeps calling
`DrawFrameEdge(Edge::CommandPanel)` for its outer frame (verified since Phase 1) and
draws everything inside it. Callbacks for later phases: `onTabChanged`,
`onCategoryChanged`, `onSubcategoryChanged`, `onCreateTypeChanged`,
`onObjectColorChanged`.

---

## 2. Measuring a control

The same method as Phase 1, applied to smaller things:
- **Colour runs** along chosen rows and columns give every edge position.
- **Letter maps** (`W` highlight, `l` light, `s` shadow, `d` dark shadow, `k` black,
  `.` face, `*` other) turn a control into a readable grid: the pressed category button,
  the dropdown's ends and the AutoGrid checkbox were decoded this way.
- **Find the rule**, then write it as code: all six tabs follow one outline rule; all ten
  buttons one rectangle rule; every rollout one spacing rule.
- **Trust surprising values** and name them: the `#C0C0C0` tab join, the `#E1E1E1`
  pressed face, the black inner shadow of the name field.

---

## 3. The tab row

- Unselected tabs: 29 px wide from x = 2, rows 3–27, 2-px chamfered top corners;
  highlight and light on the left, shadow and dark shadow on the right.
- The selected tab: the same outline 2 px wider each side and 2 px taller, drawn last
  (its shadow hides the next tab's left edge), joined to the body by two `#C0C0C0` rows.
- Icons: 22 × 20 at (tab left + 3, 6), unmoved by selection.
- Hit testing checks the selected tab first, because it is drawn on top.

---

## 4. The category row and dropdown

- Seven buttons on a 24-px pitch from x = 9. The pressed one: 1-px shadow top and left,
  2-px highlight right and bottom, a face-coloured inner line, `#E1E1E1` inside, icon 1 px
  down and right. Unpressed buttons draw only their icon.
- The dropdown matched the classic Windows combo box pixel for pixel, so it **is** one
  (`CBS_DROPDOWNLIST`): Windows draws R3's look and supplies the list, keyboard and wheel.
- **Fonts flow down** (`WM_SETFONT` from the parent, forwarded to the combo, giving the
  measured 21-px height); **notifications flow up** (`CBN_SELCHANGE` in `WM_COMMAND`,
  turned into a C++ callback).
- The lists per category are data in one function, marked as remembered except
  "Standard Primitives".

---

## 5. The rollup and rollouts

### Layout engine (`rollup::Layout`)
Container x 4–178, rows 94–885; first title at row 99; each title 16 rows, x 11–168;
frame from title + 8 to title bottom + content height; next title 7 rows below. Pure
arithmetic, so the test checks the measured rollouts exactly and also the behaviour that
cannot be measured yet (collapse moves the next rollout up; scrolling stops when the
last rollout reaches the bottom; hit testing while scrolled).

### Near-standard isn't standard
The frame looked like `DrawEdge(EDGE_ETCHED)`; it differed in 63 corner pixels. R3 draws
a shadow rectangle, then a highlight rectangle 1 px down and right over it, and stops the
top edge 1 px short of the title button. The last pixel (the container's bottom-left
corner) was chased to zero, so the comparison stays a sharp tool.

### Clipping
Rollouts scroll, so everything inside is drawn with `IntersectClipRect` limited to the
container's interior, bracketed by `SaveDC`/`RestoreDC`.

### Interaction
A title click collapses/expands; a primitive button checks/unchecks (one at a time);
dragging empty space (hand cursor) or the wheel scrolls, only when there is overflow.

---

## 6. Name and Color

- Name field: a measured, non-standard frame (sunken edge, black inner shadow, white
  inner border thicker at the sides, face interior while disabled).
- Swatch: 1-px sunken edge filled with the colour, `#8D073A` as in the reference.
- Colours are `0xRRGGBB` inside the program (like wire colours since Phase 3) and become
  `COLORREF` (`0x00BBGGRR`) only at Windows calls.
- The swatch opens `ChooseColorW` (comdlg32, now linked) as a labelled stand-in for R3's
  own Object Color dialog.
- Spinners moved to Phase 5: the reference has none to measure.

---

## 7. Painted controls vs. real controls

| Use a real control when… | Paint it yourself when… |
|---|---|
| Windows already draws the exact look (classic combo box) | the look is custom (R3's buttons, frames, tabs) |
| it brings behaviour worth having (lists, keyboard, wheel) | it must scroll or move with other painted content |
| | there would be many of them (ten buttons = ten windows to move on every scroll) |

The panel paints tabs, categories, frames, buttons, the checkbox and the swatch, hit-tests
them from the same layout functions, and uses one real `COMBOBOX`.

---

## 8. Results

At 1920 × 998 under Wine, compared with the R3.1 reference:

| Region | Compared | Differing |
|---|---|---|
| Tab panel + main toolbar | 75,602 | 0 |
| Viewport frames | 30,138 | 0 |
| Top, Front, Left views | 1,089,324 | 0 |
| **Command panel** | **143,519** | **0** |
| Time slider + track bar | 55,702 | 0 |
| Status area | 29,508 | 7 (icon art) |

Masked: text (Wine has no MS Sans Serif), icons (original art), Wine-drawn control
interiors, viewport labels; the Perspective view is checked separately (Phase 2).
Seventeen test programs; 0 warnings in a clean build.

---

## 9. Open items (unverified against R3.1)

- A selected tab other than Create; tab and category tooltips; category hover look.
- The dropdown lists other than "Standard Primitives".
- A checked primitive button; the collapsed "+"; spacing after a collapsed rollout;
  scrolling details (hand-drag, wheel step); which rollouts other categories show.
- The name field's enabled look (Phase 6); R3's Object Color dialog.
- Spinners (Phase 5, from a screenshot of the Parameters rollout).

---

## 10. Exercises

| Step | Try |
|---|---|
| 4.1 | Start on the Modify tab; recolour an icon in `make_icons.py`; change `kTabJoin`. |
| 4.2 | Start on Cameras; change `kPressedFace`; add "Doors" to Geometry's list. |
| 4.3 | Start with Object Type collapsed; set `kObjectTypeContent` to 900 to see scrolling; compare with `DrawEdge(EDGE_ETCHED)`. |
| 4.4 | Change the starting colour; swap the colour bytes to see the red-blue bug. |
