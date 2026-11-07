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
- [ ] 7.3 Bend, Taper, Twist rollouts; adding and removing modifiers through undo
- [ ] 7.4 Edit Mesh at vertex and face level (sub-object selection, moving vertices)
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

## Notes
- No user interface yet: the modifiers are visible through the demo scene; the Modify
  panel comes in step 7.2.
