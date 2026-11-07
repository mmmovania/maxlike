# Phase 7 — Technical documentation

Phase 7 gave maxlike R3's Modify panel: the modifier stack, the Bend, Taper and Twist
modifiers with their rollouts, and Edit Mesh at Vertex level with its rollouts. With a
box, with Bend and with Edit Mesh (unscrolled and scrolled), the panel was compared with
R3.1 screenshots pixel for pixel. This document collects the explanations given while
building it. Step record: `docs/phases/phase-7-modify.md`; measurements:
`docs/spec/01-ui-look.md`.

Code at the end of Phase 7: tag `phase-7-done` (version 0.7.0).

## Contents
1. Architecture and file map
2. Deformers as coordinate maps
3. Verifying geometry from a screenshot
4. The modifier stack and undo of structure
5. The Modify panel: generalising with defaults
6. Rollouts as data
7. Edit Mesh: editing at a level of the stack
8. Two observations make a model: the scroll indicator
9. Bugs between pieces, and sentinels
10. Results
11. Open items
12. Exercises

---

## 1. Architecture and file map

| Part | Files | Windows? |
|---|---|---|
| Bend, Taper, Twist; Edit Mesh selection and moves; stack evaluation up to a level | `src/scene/Modifier.*` | no |
| Add / remove / on-off modifiers; modifier parameters; vertex selection and moves | `src/scene/Undo.*` | no |
| Point picking (vertices) | `src/view/Picking.*` | no |
| Rollout contents as data (rows, groups, several rollouts per modifier) | `src/ui/ParametersSpec.*` | no |
| Rollup layout with a container parameter; the scroll indicator | `src/ui/Rollup.*` | no |
| The Modify tab: name field, Modifiers, Modifier Stack, rollouts | `src/ui/CommandPanel.*` | yes |
| The More… dialog (in-memory template) | `src/ui/ModifiersDialog.*` | yes |
| Vertex diamonds, vertex picking and drags in the views | `src/ui/Viewport.*` | yes |
| Wiring (stack info, commands, sub-object state) | `src/ui/MainFrame.*` | yes |

---

## 2. Deformers as coordinate maps

A deformer moves every vertex by a rule; faces follow because they connect the same
vertices. All three share a frame: h along the chosen axis (0 X, 1 Y, 2 Z) and the two
perpendicular directions in cyclic order. Twist turns the cross-section by angle × h / L,
Taper scales it by 1 + amount × t + curve × 4t(1 − t), and Bend wraps the (a, h) plane
round a circle of radius r = L / angle, so lengths along the axis are kept. With limits,
the effect stops at the limit and the rest follows rigidly along the tangent.

---

## 3. Verifying geometry from a screenshot

R3.1's Bend screenshot (a 62.572 × 60.718 box bent 30.5°) let the maths be checked, not
just the look: with the Front view's scale from the unbent box, our bent corners land on
R3's drawn edges within half a pixel at nine heights. The white lines (the mesh's real
edges) were measured, not the orange gizmo. A permanent test holds R3's corner positions;
its first version had no base vertices, so zero height, and Bend correctly did nothing.

---

## 4. The modifier stack and undo of structure

A node holds a plain object until its first modifier; `AddModifierCommand` then wraps the
object in a Derived Object and remembers doing so, so undo restores the plain object.
`RemoveModifierCommand` remembers the position, so undo puts the modifier back in place.
`SetParamCommand` holds whichever owns the parameters (an object or a modifier) and a
pointer to its block, so a box's height and a Bend's angle are edited and undone the
same way.

---

## 5. The Modify panel: generalising with defaults

The Modify panel's rollouts follow the Create panel's rules with a container from row 59
(bottom at 884). The rollup functions take the container as a parameter defaulting to
the Create panel's, so existing callers and tests did not change. Selecting the Modify
tab revealed two tab-join rules invisible with Create selected; the new combos needed the
panel's font, without which Windows sized them larger.

---

## 6. Rollouts as data

Rows gained their own field position and width, arrow position, checkbox position,
decimals and an enabled flag, all defaulting to the first measured layout, plus new kinds:
radios, push buttons, labels (which paint their own background), the level icons; and
group boxes with their own frame and label gap. A modifier can have several rollouts
(Edit Mesh: Selection, Soft Selection, Edit Geometry, Surface Properties). Row ids
encode the rollout (`slot × 100 + row`). A brace-initialised struct gaining fields made
MinGW warn about missing initialisers; a constructor with the original parameters keeps
every list compiling without them.

---

## 7. Edit Mesh: editing at a level of the stack

Edit Mesh shows and edits the mesh as it is at its own level (`EvaluateUpTo`), so a Bend
above it does not move the vertices being edited. Its vertex selection is undoable and
kept sorted; a drag converts the world movement into the object's space (the inverse of
the object matrix) and is recorded once. Vertices are R3's 5-px diamonds, red when
selected and blue otherwise. Edit Mesh is added through the More… dialog, R3's list of
modifiers not on the button set.

---

## 8. Two observations make a model: the scroll indicator

Unscrolled, R3's indicator (x 174–177) is grey for rows 60–587 and black below; scrolled by
270, black 60–231, grey 232–760, black again. Visible part grey, hidden parts black, in
proportion: from 60 + ⌊scroll·H/C⌋ to 60 + ⌈(scroll+H)·H/C⌉ − 1 with H = 824 and
C = H + the maximum scroll. Rounding the end up, not to nearest, is what fits both. The
indicator also implied the height of the unseen Surface Properties rollout (the column
scrolls 463 rows).

---

## 9. Bugs between pieces, and sentinels

- A crash: the panel reported More…, Sets and the icon button as 10–12 into a table of
  ten names. Shared numbering belongs in one place.
- A rollout scrolled above the container had a negative title row, which the code took as
  "not shown", hiding its visible combo. `kNoTitle`, far outside any real position, now
  means "not shown"; the mechanical replacement of the old tests matched `slot < 0` and a
  tab index too, caught only by reading the diff.
- An edit script that writes nothing unless every replacement applies made a failed run
  harmless.

---

## 10. Results

- Bend's geometry matches R3.1's bent box within half a pixel.
- Modify panel against R3.1: with a box 58 differing pixels (Wine's disabled combo), with
  Bend 0 of 67,108, with Edit Mesh 0 of 82,389, scrolled by 270: 1 of 86,174 (a digit).
- The Create panel unchanged; twenty-four test programs; 0 warnings.

---

## 11. Open items

- Edit Mesh's other levels and its button actions; Soft Selection's and Surface
  Properties' contents; Rotate and Scale at vertex level; the modifier gizmo; the other
  modifiers in the More… list; the Modifiers and Modeling tab strips.
- Unverified: Taper's and Twist's rollouts (laid out like Bend's), modifier parameter
  indices (Phase 8 will check them against files), the More… dialog's layout, the level
  icons (original drawings), the wheel's scroll step.

---

## 12. Exercises

| Step | Try |
|---|---|
| 7.1 | Bend 360° (a ring); Limit Effect with an upper limit; Bend along X. |
| 7.2 | Stack Bend, Twist, Taper; remove the middle one and undo; add Bend to an instance. |
| 7.3 | Taper with Symmetry; Twist with Bias. |
| 7.4 | Edit Mesh, Sub-Object, move vertices; add Bend above it and keep editing. |
| 7.4b | Watch "N Vertices Selected"; click the level icons. |
| 7.4c | Scroll the Edit Mesh panel and watch the indicator; compare with the formula. |
