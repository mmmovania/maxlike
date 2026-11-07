# Phase 7 — Modify panel

Branch: `phase/7-modify`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 7.

## Steps
- [x] **7.1 Bend, Taper, Twist (geometry)**: deformation modifiers with parameter blocks
      (`scene::BendModifier`, `TaperModifier`, `TwistModifier`), about the pivot along a
      chosen axis, with limits; `--demo-scene deform`
- [x] **7.2a Measured and verified**: four R3.1 screenshots (Modify tab with a box, with
      Bend, with Edit Mesh at Vertex level; the Modifiers list); Bend's geometry checked
      against R3.1's bent box (both edges within half a pixel; a permanent test); the
      Modify panel's layout measured (`01-ui-look.md`)
- [x] **7.2b The Modify panel**: name field above the rollup (`rollup::kModifyContainer`),
      Modifiers rollout (Bend, Taper, Twist add a modifier; the others report "not
      implemented yet" or are disabled as in R3), Modifier Stack rollout (the stack list,
      on/off, remove, Sub-Object), the base object's Parameters; every change undoable
      (`AddModifierCommand`, `RemoveModifierCommand`, `EnableModifierCommand`,
      `SetParamCommand` for modifiers)
- [x] **7.3 Bend, Taper, Twist rollouts**: rows with their own field, checkbox and
      decimals, radio rows and group boxes (`params::Row`, `params::Group`); Bend's rollout
      measured (`ModifierParametersFor`), Taper's and Twist's in the same layout; every
      change undoable
- [ ] 7.3b The modifier gizmo in the views (orange in R3)
- [x] **7.4 Edit Mesh at Vertex level**: More... dialog (R3's list; Edit Mesh works);
      Sub-Object (pressed yellow) and its levels; vertices drawn as R3's diamonds (red
      selected, blue otherwise) from the stack evaluated up to the Edit Mesh; click /
      region / Ctrl / Alt vertex selection and Move drags of the selected vertices, each one
      undo step (`SelectVerticesCommand`, `MoveVerticesCommand`)
- [x] **7.4b Edit Mesh's rollouts**, measured: Selection (level icons, checkboxes, Planar
      Thresh, Hide / Unhide All, Copy / Paste, "N Vertices Selected"), Soft Selection
      (collapsed), Edit Geometry down to the Weld group; new row kinds (buttons, labels,
      level icons), disabled controls, several rollouts per modifier
- [x] **7.4c Scrolled**: the rest of Edit Geometry (Weld Target, Tessellate, Explode,
      Remove Isolated Vertices, View / Grid Align, Make Planar / Collapse) and Surface
      Properties' place, from R3.1 scrolled by 270; the rollup's scroll indicator
      (`rollup::ScrollThumb`), measured at two scroll positions
- [ ] Soft Selection's and Surface Properties' contents; Edit Mesh's button actions; Face level
- [ ] 7.5 Wrap-up

## Step 7.1 verification
- [x] Builds warning-free (clean rebuild); twenty-four test programs pass (MinGW + Wine)
- [x] `test_modifiers`: zero settings change nothing; Bend 90° puts the top of a
      100-unit column at (r, 0, r), keeps every point on the arc and the arc length at 100;
      Direction 90° bends towards +Y; limits continue straight on; Twist 90° turns the top
      corner a quarter turn; Taper amount and curve; a bend along X; a box through the
      modifier stack
- [x] Under Wine, `--demo-scene deform`: a quarter-arc bend standing on its pivot, a
      90° twist, an inward taper with a slight bow
- [ ] On Windows: `ctest`; `build\Debug\maxlike.exe --demo-scene deform`
- [ ] Unverified: parameter indices (they matter for `.max` import), Twist bias and
      Taper curve formulas (Bend's direction, extent and centre: verified in 7.2a)

## Step 7.2a verification
- [x] `test_modifiers`: R3.1's Box (62.572 wide, 60.718 tall) bent 30.5° matches R3's
      drawn corners within one pixel (right corner (42.79, 41.94); the left edge through
      three measured points)
- [x] Bend's direction (+X), radius rule (height ÷ angle) and centre (the pivot) are
      therefore verified, no longer from memory
- [ ] On Windows: `ctest`

## Step 7.2b verification
- [x] Builds warning-free (clean rebuild); twenty-four test programs pass (MinGW + Wine)
- [x] `test_undo`: adding a modifier wraps a plain object in a stack (undo unwraps it);
      a second one stacks on top; a modifier parameter change; removing restores at the
      same place; on/off
- [x] `test_rollup`: the Modify container reproduces the measured titles (64, 250, 378)
- [x] Under Wine, compared with `r3.1-modify-box.png`: the Modify panel differs in 58
      pixels of 59,367 (text, icons and the swatch masked), all inside the disabled
      selection-level combo's drop button, which Wine draws with a white highlight where
      Windows draws light grey (the enabled dropdown matches exactly)
- [x] The selected tab's join, measured with Modify selected: the body's top edge starts
      at x 1, and the join's second row spans the tab from its left edge; the Create
      panel is unchanged (window identical to the end of Phase 5)
- [x] Under Wine: Bend added (stack "Bend", bulb on, Sub-Object enabled, "Gizmo"); Box
      picked in the list (its Parameters return); remove and undo
- [ ] On Windows: the same; the disabled combo's button should match R3 there
- [ ] Unverified: the stack icons (original drawings, approximate positions), the icon
      button's icon, what More... and Sets open (not implemented)

## Step 7.3 verification
- [x] Builds warning-free (clean rebuild; `Row` got a constructor so brace lists keep
      their new defaults without MinGW's missing-initializer warnings); twenty-four test
      programs pass (MinGW + Wine)
- [x] `test_parameters_spec`: Bend's measured groups, spinner fields (x 81 and 87),
      radios (x 40, 81, 120), checkbox (x 87), one decimal, content height; every row of
      Bend, Twist and Taper refers to a real parameter; Create rows unchanged
- [x] Under Wine, compared with `r3.1-modify-bend.png`: the whole Modify panel with Bend,
      0 differing pixels of 67,108 (text, icons, the swatch and the disabled level combo
      masked); the plain-box panel still 58 (Wine's disabled combo button); the default
      window unchanged
- [x] Under Wine: Angle 90 bends a box; Bend Axis X changes the bend; Undo restores it
- [ ] On Windows: edit Bend, Taper and Twist with spinners, radios and Limit Effect; undo
- [ ] Unverified: Taper's and Twist's rollouts (laid out like Bend's: labels, the axis
      group's extra rows, label gaps), spinner speeds

## Step 7.4 verification
- [x] Builds warning-free (clean rebuild); twenty-four test programs pass (MinGW + Wine)
- [x] `test_undo`: vertex selection sorted and unique, undoable; moves accumulate per
      vertex and undo in one step; the stack evaluated up to the Edit Mesh ignores modifiers
      above it
- [x] `test_picking`: nearest point within the tolerance; points in a rectangle, any direction
- [x] Under Wine: More... → Edit Mesh; Sub-Object (yellow, "Vertex"); four 13-px diamonds
      at the box's corners; a rectangle selects the right-hand corners (red); a Move drag
      of 50 px moves exactly those vertices 50 px; Undo restores them
- [x] A crash found and fixed while testing: More..., Sets and the icon button reported
      indices 10–12 into a ten-name table
- [x] No regressions: default window unchanged; the Modify panel with Bend still 0
      differing pixels against R3.1
- [ ] On Windows: the same, including the dialog's look (not compared: this test display
      has no window manager, so the dialog is not painted there)
- [ ] Unverified: the dialog's layout and the entries after "STL-Check"; vertex pick
      tolerance; Rotate and Scale at vertex level (not implemented)

## Step 7.4b verification
- [x] Builds warning-free (clean rebuild); twenty-four test programs pass (MinGW + Wine)
- [x] `test_parameters_spec`: Edit Mesh's three rollouts and their measured geometry
- [x] Under Wine, compared with `r3.1-modify-editmesh-vertex.png`: the whole Modify panel
      with Edit Mesh at Vertex level, 0 differing pixels of 78,099, down to the
      container's bottom edge (text, icons, the swatch, the level combo and the scroll
      indicator column masked); plain box still 58 (Wine's disabled combo), Bend 0; the
      Create panel unchanged
- [x] Found while comparing: the Modify container's bottom edge is row 884 (Create: 885);
      content stops one row above it; with an overflowing column the container's lines
      leave a 4-px column for the scroll indicator; a checked box's tick is the classic
      7 × 7 pattern (Wine draws another; now drawn from the measured pattern)
- [x] "N Vertices Selected" follows the selection; level icons enter sub-object mode
- [ ] On Windows: the same; checked boxes' ticks (enabled: black, assumed)
- [ ] Unverified: the level icons (original drawings), R3's wording for one or no
      vertices, Edit Geometry below Weld and its content height, Soft Selection's contents

## Step 7.4c verification
- [x] Builds warning-free (clean rebuild); twenty-four test programs pass (MinGW + Wine)
- [x] `test_rollup`: the indicator's grey part 60..587 unscrolled and 232..760 at 270, as
      measured; it reaches the last row at the end of the column
- [x] `test_parameters_spec`: four Edit Mesh rollouts, Edit Geometry's measured height
- [x] Under Wine, compared with R3.1, scroll indicator included: Edit Mesh unscrolled 0
      differing pixels of 82,389; scrolled by 270, 1 of 86,174 (one pixel of a digit in
      Wine's font); Bend 0; plain box 58 (Wine's disabled combo)
- [x] Found while comparing: controls inside a rollout scrolled above the container's top
      were hidden (a title above the top was treated as "not shown"; now `kNoTitle`);
      labels paint their own background ("pixels" covers the Target arrows' last column);
      disabled radios show a grey dot; the indicator's end rounds up
- [ ] On Windows: scroll the Edit Mesh panel (wheel or drag) and watch the indicator
- [ ] Unverified: Surface Properties' height (inferred from the indicator: the column
      scrolls 463 rows) and contents; Soft Selection's contents

## Notes
- No user interface yet: the modifiers are visible through the demo scene; the Modify
  panel comes in step 7.2.
