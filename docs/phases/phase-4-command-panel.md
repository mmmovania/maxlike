# Phase 4 — Command panel

Branch: `phase/4-command-panel`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 4.

## Steps
- [x] **4.1 Tabs**: measured tab row and body edge (`cmdpanel::TabBox`, unit-tested);
      `CommandPanel` replaces the placeholder; six original tab icons; click to select
- [x] **4.2 Category row and dropdown**: seven category buttons (measured pressed look),
      original 16 × 16 icons, a classic combo box listing each category's types
- [x] **4.3 Rollouts and the rollup**: measured container, title buttons and etched
      frames (`rollup::Layout`, unit-tested); Object Type with AutoGrid and ten buttons;
      collapse/expand, check a primitive, scroll by hand-drag or wheel when needed
- [x] **4.4 Name and Color**: measured name field (disabled until a selection exists)
      and colour swatch (`#8D073A`, as in the reference), colour picker on click;
      spinners moved to Phase 5 (none in the reference to measure)
- [x] **4.5 Wrap-up**: whole-window check, `docs/tech/phase4-tech-docs.md`, version 0.4.0

## Step 4.1 verification
- [x] Builds warning-free; sixteen test programs pass (MinGW + Wine)
- [x] `test_command_panel_layout`: the six measured tab outlines, icon positions, hit
      testing where the selected tab overlaps its neighbour
- [x] Under Wine at the reference size: panel frame and tabs 0 differing pixels of 7,722
      (icons masked; body interior is for the next steps)
- [x] Clicking a tab selects it (raised, joined to the body)
- [x] On Windows: the same; tab icons look reasonable
- [ ] Unverified: the look of a selected tab other than Create; R3's tab tooltips

## Step 4.2 verification
- [x] Builds warning-free; sixteen test programs pass (MinGW + Wine)
- [x] `test_command_panel_layout`: category buttons (pressed Geometry x 9–32, rows 37–59;
      icons at 37, 61, …, 157), hit testing, the dropdown's 143 × 21 frame, lists per category
- [x] Under Wine: panel rows 0–93 identical to the reference (10,270 pixels; icons and
      the dropdown's interior masked)
- [x] Clicking a category presses it and refills the dropdown; other tabs hide both
- [x] On Windows: the same; the dropdown opens and lists its entries
- [ ] Unverified: the dropdown lists other than "Standard Primitives"; category tooltips;
      the hover look of category buttons

## Step 4.3 verification
- [x] Builds warning-free; seventeen test programs pass (MinGW + Wine)
- [x] `test_rollup`: Object Type (title 99–114, frame 107–241) and Name and Color (title
      248–263, frame 256–299) exactly; collapsing, scrolling by the overflow, hit testing
- [x] Under Wine: the whole command panel identical to the reference, 138,011 pixels
      (text, icons, dropdown interior and Name and Color's contents masked)
- [x] Clicking a primitive checks it (again: unchecks); clicking a title collapses/expands
- [x] On Windows: the same; text looks like R3's
- [ ] Unverified: see `01-ui-look.md` (checked look, "+", scrolling details)

## Step 4.4 verification
- [x] Builds warning-free; seventeen test programs pass (MinGW + Wine)
- [x] `test_command_panel_layout`: name field x 15–146 and swatch x 150–166, rows 271–290
- [x] Under Wine: the whole command panel identical to the reference, now including Name
      and Color: 0 differing pixels of 143,519 (text, icons, dropdown interior masked)
- [x] Clicking the swatch opens a colour dialog; the chosen colour shows in the swatch
- [x] On Windows: the same
- [ ] Unverified: R3's own Object Color dialog; the name field's enabled look

## Step 4.5 verification (whole client area at 1920 × 998, Wine)
- [x] Clean rebuild: 0 warnings; seventeen test programs pass
- [x] Tab panel + toolbar, viewport frames, Top, Front, Left, command panel, time slider +
      track bar: 0 differing pixels; status area: 7 (icon art, as before). Total 7 of
      1,423,793 (text, icons, Wine-drawn control interiors, labels and the Perspective
      view masked; Perspective checked in Phase 2)
- [x] On Windows: `ctest`; the panel next to R3.1

## Notes
- R3's etched frame is not `DrawEdge(EDGE_ETCHED)`: the corners differ (two offset
  rectangles, the highlight drawn last).
- Tab icons are original art in the spirit of R3's (cursor with sparkle, bent pipe,
  hierarchy, wheel, monitor, hammer), as for all icons in this project.
