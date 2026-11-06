# Phase 4 — Command panel

Branch: `phase/4-command-panel`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 4.

## Steps
- [x] **4.1 Tabs**: measured tab row and body edge (`cmdpanel::TabBox`, unit-tested);
      `CommandPanel` replaces the placeholder; six original tab icons; click to select
- [ ] 4.2 Category row (seven icon buttons) and the Standard Primitives dropdown
- [ ] 4.3 Rollouts and the rollup (collapsible sections, scrolling column); Object Type
- [ ] 4.4 Name and Color (name field, colour swatch); check buttons; spinners
- [ ] 4.5 Pixel check of the whole panel, `docs/tech/phase4-tech-docs.md`, version 0.4.0

## Step 4.1 verification
- [x] Builds warning-free; sixteen test programs pass (MinGW + Wine)
- [x] `test_command_panel_layout`: the six measured tab outlines, icon positions, hit
      testing where the selected tab overlaps its neighbour
- [x] Under Wine at the reference size: panel frame and tabs 0 differing pixels of 7,722
      (icons masked; body interior is for the next steps)
- [x] Clicking a tab selects it (raised, joined to the body)
- [ ] On Windows: the same; tab icons look reasonable
- [ ] Unverified: the look of a selected tab other than Create; R3's tab tooltips

## Notes
- Tab icons are original art in the spirit of R3's (cursor with sparkle, bent pipe,
  hierarchy, wheel, monitor, hammer), as for all icons in this project.
