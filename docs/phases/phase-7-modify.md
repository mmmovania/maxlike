# Phase 7 — Modify panel

Branch: `phase/7-modify`

Acceptance criteria: see `docs/spec/02-plan.md`, Phase 7.

## Steps
- [x] **7.1 Bend, Taper, Twist (geometry)**: deformation modifiers with parameter blocks
      (`scene::BendModifier`, `TaperModifier`, `TwistModifier`), about the pivot along a
      chosen axis, with limits; `--demo-scene deform`
- [ ] 7.2 The Modify panel: modifier stack list, Modifiers buttons, the selected
      object's or modifier's Parameters (needs R3.1 screenshots of the Modify tab)
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
- [ ] Unverified: parameter indices (they matter for `.max` import), bend direction
      sign, Twist bias and Taper curve formulas, the extent used (the mesh's bounding box
      along the axis), the modifier centre (the pivot)

## Notes
- No user interface yet: the modifiers are visible through the demo scene; the Modify
  panel comes in step 7.2.
