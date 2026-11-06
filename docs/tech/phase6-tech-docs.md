# Phase 6 — Technical documentation

Phase 6 made objects editable after creation: the tab panel's Objects strip, selection
(click, region, Ctrl/Alt, Delete), Name and Color for the selection, Move / Rotate /
Scale with R3's axis constraints and its measured on-screen axes, and the status bar's
X/Y/Z. Every change is undoable. This document collects the explanations given while
building it. Step record: `docs/phases/phase-6-select-transform.md`; measurements:
`docs/spec/01-ui-look.md`.

Code at the end of Phase 6: tag `phase-6-done` (version 0.6.0).

## Contents
1. Architecture and file map
2. The Objects strip: a measurement reveals a reuse
3. Selection as document state
4. Picking what is drawn
5. Identity versus labels
6. The axis tripod and the Move gizmo: reverse-engineering a rasteriser
7. Moving: rays, drag planes and parent space
8. Rotating and scaling
9. The status bar's X/Y/Z, and a plan corrected by measurement
10. Results
11. Open items
12. Exercises

---

## 1. Architecture and file map

| Part | Files | Windows? |
|---|---|---|
| Objects strip data | `ToolbarSpec` (`ObjectsToolbarSpec/Config`), `res/objects_icons.bmp` | no |
| Selection, Select/Delete commands | `Scene` (selection), `Undo` (`SelectCommand`, `DeleteSelectionCommand`) | no |
| Picking | `src/view/Picking.*` | no |
| Tripod and Move gizmo pixels | `src/view/TransformAxes.*` | no |
| Move / Rotate / Scale maths, status values | `src/scene/MoveMath.*` | no |
| Picking, drawing, drags in the views | `Viewport` (`WorldToWindow`, `PickAt/Rect`, `DrawBrackets`, `DrawTransformAxes`, `MouseRay`, `MoveDelta`) | yes |
| Name typing, `SetCreateType` | `CommandPanel` | yes |
| Tools, constraints, wiring | `MainFrame` | yes |
| Tests | `test_picking`, `test_transform_axes`, `test_move_math` (+ `test_undo`, `test_creation_tool`, `test_toolbar_spec`) | |

---

## 2. The Objects strip: a measurement reveals a reuse

Every button and separator of R3.1's Objects strip sits exactly where the main toolbar's
geometry puts them (32-px buttons from x 4, 6-px separators with the line at +3/+4), so
the strip is a second `Toolbar` built from data, shown in the main toolbar's place with
the Objects tab. Its ten Standard Primitive buttons call `CommandPanel::SetCreateType`,
the same path as the panel's own buttons. The strip was deferred from Phase 5 until a
screenshot existed; it turned out to have 29 buttons, not the ten a guess would have
produced. Result: tab row and strip 0 differing pixels of 81,384.

---

## 3. Selection as document state

The selection lives in the `Scene` (node ids, in selection order) and changes through
`SelectCommand`, as in R3's Edit > Undo Select; a click that changes nothing records
nothing. `DeleteSelectionCommand` remembers what was selected and restores it, selected,
on undo; `Scene::DeleteNode` drops a node from the selection, so the selection never
refers to a missing node. A new object is selected when created. Rules: a click replaces
the selection, Ctrl adds, Alt removes, empty space clears; a press becomes a region once
the mouse moves more than 2 px.

---

## 4. Picking what is drawn

`Viewport::WorldToWindow` uses the drawing's own projection (the grid formula in ortho,
the camera matrices in perspective), so clicks land on what is drawn. Wireframe views
pick the nearest visible edge within 4 px (the inside of an outline is empty space);
shaded views pick the nearest triangle under the cursor (same-side test, either winding);
regions select objects with any edge touching the rectangle (Liang–Barsky clipping).
Selected objects draw white; shaded views add brackets at the bounding-box corners, a
quarter of each edge long.

---

## 5. Identity versus labels

The creation tool first remembered its last object by name, which broke the moment it
was renamed; it now keeps the node id. `MainFrame::NameColorTarget` answers "whose name
and colour does the panel show?" in one place (the creation target, else the single
selected object), used for both display and editing. Names cross from UTF-16 (Windows)
to UTF-8 (the scene) through `WideCharToMultiByte` / `MultiByteToWideChar`.

---

## 6. The axis tripod and the Move gizmo: reverse-engineering a rasteriser

From three R3.1 screenshots: the **axis tripod** (Select, creation) on the object's own
axes, `#C82323` for constrained axes and `#666666` for the others; the **Move gizmo**
(Move, Rotate and Scale, confirmed by the project owner) on the world axes, with yellow
for the constraint, filled heads in the axis colours and plane corners. Both use the
viewport tripod's glyphs and its "9 rows above the tip" rule, a sign of one shared R3
routine. The gizmo's rules became simple once the pivot was read correctly (x 433, not
434): shafts O + round(17·d) to O + round(60·d), letters at O + round(84.5·d), corners
at O + round(84·(dₐ + d_b)). R3 rasterises the heads as triangles; a parameter search
over six measured heads (and a second family of rasteriser rules, which fitted worse)
gave tip 84.5, base 62, half-width 3.75, pivot +0.5 px down, leaving about 10 % of each
head's edge pixels different. Aligned per view, 68–83 % of R3's gizmo pixels match.

---

## 7. Moving: rays, drag planes and parent space

In ortho views the mouse moves on the view plane and the constraint drops the other
axes. In perspective, two mouse rays meet a plane through the pivot: a two-axis
constraint's own plane, or for one axis the plane containing it that faces the camera
(normal = axis × (axis × view)). Objects are placed at start + total movement on every
mouse move, so nothing drifts and cancelling is exact; the release records one "Move".
New world positions are converted into each parent's space (Cramer's rule on the 3×3
part); a child of a selected parent is skipped so it does not move twice.

---

## 8. Rotating and scaling

Rotate turns each object about its own pivot, about the constrained axis (a plane: its
normal), 0.5° per pixel. A world rotation becomes a local one by a change of basis:
parent⁻¹ × delta × parent × start (the inverse of a unit quaternion is its conjugate).
Scale: uniform; non-uniform on the constrained axes; squash keeps the volume (the other
axes scale by f^(−n/(3−n))); a 1 % floor. Each drag is one undo step named after its
tool.

---

## 9. The status bar's X/Y/Z, and a plan corrected by measurement

The fields show the world point under the cursor (view plane in ortho, ground in
perspective; only for points inside the view) and, while dragging, the movement, the
angle or the scale percentage. The plan had said "type-in", but the measured fields
(plain sunken boxes, no spinners) and R3's design (typed transforms through the
Transform Type-In dialog) say display-only, so the step and the plan were changed and
the dialog listed for later; its Euler conversion is written and tested.

---

## 10. Results

- Objects strip and tab row: 0 differing pixels of 81,384.
- Move: a 100-px drag moves 100 px; constrained drags move only along allowed axes; a
  perspective drag slides along the ground (height unchanged).
- Rotate: 180 px = 90°, about the centre; Scale: 100 px up = ×2; each one undo step.
- X/Y/Z: 46.377 for a 100-px move (100 ÷ 2.156), 90.000 for a 180-px rotation.
- With nothing selected the window is unchanged since Phase 5.
- Twenty-four test programs; 0 warnings in a clean build.

---

## 11. Open items

- Selection filter; reference coordinate systems other than View; transform centres other
  than each pivot; the Transform Type-In dialog.
- Unverified: pick tolerance, crossing as the default region mode, Ctrl/Alt rules, the
  selection rectangle's look, bracket length, "N Objects Selected", drag speeds and
  directions, R3's number format in X/Y/Z, gizmo arrowhead edges.
- Code: `scene::move::InverseTransformPoint` duplicates `math::AffineInverse`; both are
  tested and agree; merge them when that code is next touched.

---

## 12. Exercises

| Step | Try |
|---|---|
| 6.1 | Handle `create.hedra` in `OnCommand` with a prompt of your own. |
| 6.2 | Raise the pick tolerance to 8 px; switch regions to "window" (all edges inside). |
| 6.3 | Rename to "Würfel"; break the id tracking by returning `name_` in `TargetName`. |
| 6.4a | Widen the arrowheads to 4.5 px and compare with `r3.1-move-selected.png`. |
| 6.4b | Move a child and its parent together; try a Y-only move in the Front view. |
| 6.4c | Squash a sphere along Z; rotate a child of a rotated parent. |
| 6.4d | Watch X/Y/Z along a grid line; non-uniform scale with X only. |
