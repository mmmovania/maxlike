# Phase 5 — Technical documentation

Phase 5 made maxlike a modelling tool: all ten Standard Primitives are created by
dragging in any viewport or by typing values, each as one undo step, with the Create
panel's creation rollouts (Creation Method, Keyboard Entry, Parameters with spinners)
measured from an R3.1 screenshot. This document collects the explanations given while
building it. Step record: `docs/phases/phase-5-creation.md`; measurements:
`docs/spec/01-ui-look.md`.

Code at the end of Phase 5: tag `phase-5-done` (version 0.5.0).

## Contents
1. Architecture and file map
2. Interaction as a state machine
3. Construction planes and the mouse
4. Live versus recorded: one undo step per object
5. Behaviour versus appearance: the spinner
6. The creation rollouts, measured
7. One mechanism for two lists
8. Results
9. Open items
10. Exercises

---

## 1. Architecture and file map

| Part | Files | Windows? |
|---|---|---|
| Creation steps, construction planes | `src/scene/Creation.*` | no |
| Creating in the scene, undo, names, colours | `src/scene/CreationTool.*` | no |
| Spinner behaviour | `src/ui/SpinnerModel.*` | no |
| Rollout contents as data | `src/ui/ParametersSpec.*` | no |
| Rollup layout (collapsed frames corrected) | `src/ui/Rollup.*` | no |
| Mouse to plane, events | `Viewport::PointOnPlane`, `CreationPlane`, `CreateEvent` | yes |
| Panel painting and interaction | `src/ui/CommandPanel.*` | yes |
| Wiring | `MainFrame` (creation tool, Parameters source, undo) | yes |
| Tests | `test_creation`, `test_creation_tool`, `test_spinner`, `test_parameters_spec` (+ `test_rollup`, `test_ortho_grid`) | |

---

## 2. Interaction as a state machine

Creating a box is a conversation: press, drag the base, release, move for the height,
click. `scene::Creator` holds a list of steps per primitive (`DragRect`, `DragRadius`,
`MoveHeight`, `MoveRadius2`); drags end on release, moves end on a click. After every
event, `Params()` holds the object so far (by Max index) and `CenterU/V` its pivot on
the plane. Rules: a click without a drag cancels; height follows vertical mouse
movement (up = positive, below = negative); the pivot is the base centre; the torus's
tube radius is measured from the ring. The sequences are from memory of R3 (unverified)
and live in one switch, so correcting one is a one-line change.

---

## 3. Construction planes and the mouse

- A plane is (u, v, normal), right-handed; `NodeTransform` turns a point on it into a
  node transform whose rotation maps local X, Y, Z onto u, v, normal. An ortho view's
  plane is its own right and up (normal towards the viewer), so every ortho view works
  the same way and objects stand facing the view they were created in. Perspective
  uses the ground (XY).
- Ortho: the grid's pixel formula reversed (`UOfColumn`, `VOfRow`; `+ 0.5` aims at the
  pixel centre); a round-trip test checks it.
- Perspective: a ray from the eye through the pixel (forward × focal length + right ×
  dx − up × dy) hits z = 0; behind the camera means no point.
- While one viewport builds, the others are ignored (owner), and the mouse is captured.

---

## 4. Live versus recorded: one undo step per object

- While building, `CreationTool` adds a live node directly to the scene (all views draw
  it) without touching the undo history; cancelling simply removes it.
- Finishing removes the live node and executes one `CreateNodeCommand` that recreates
  it (same name, same shared object, colour, transform): one Undo removes the object.
- Keyboard Entry's Create (`CreateDirect`) goes through the same command, naming and
  colour rules.
- Parameter edits afterwards: a spinner drag previews on the object; on release
  `MainFrame` restores the start value and executes one `SetParamCommand` with the final
  value, so undo returns to the value before the drag.
- Colours (measured): the swatch shows the object just created; a new random colour is
  picked when the next object starts; a colour chosen in the swatch is used as-is.

---

## 5. Behaviour versus appearance: the spinner

`SpinnerModel` holds a spinner's behaviour without its look: limits, integer rounding,
arrow steps, a drag measured as total movement from its start (Ctrl ×10, Alt ×0.1),
right-click reset (0, or the nearest limit) and drag cancel, strict typed entry, and
display rounding. `printf` rounds exact halves to the even digit ("3.25" → "3.2"), so
the model rounds halves away from zero first. The behaviour was finished and tested
before the screenshot arrived; step 5.3b only added the measured drawing.

---

## 6. The creation rollouts, measured

From `docs/reference/screenshots/r3.1-box-created.png`:
- Rollouts while creating: Object Type, Name and Color, Creation Method (title 306),
  Keyboard Entry (collapsed, 356), Parameters (376); a collapsed rollout keeps a small
  frame (title + 4 … + 13) and the next starts 7 rows below it.
- The checked type button: pressed outline around `#C0FFC0`; AutoGrid and the name field
  enabled; values with three decimals.
- Spinner: label to x 87, field x 90–143, arrows x 146–157 (up rows 0–7, down 8–14, a
  dark bottom row), 5-px triangles.
- Radio buttons drawn from their measured 12 × 12 pattern: Wine's `DrawFrameControl`
  draws a different circle than Windows; checkboxes matched, radios did not.
- A 4-px gutter inside the container's right edge appears with the creation rollouts.
- Parameters per primitive and Creation Method labels are data (`ui::params`); Box is
  measured, the rest follow the same rule (unverified lists).

The first comparison found 158 differing pixels, then 48 (exactly 8 per spinner: the
lower arrow half is one row shorter), then 0.

---

## 7. One mechanism for two lists

Keyboard Entry needs the same spinners as Parameters, but stores its values until Create.
Every row has one id (Parameters 0…, Keyboard Entry from 1000), and four helpers hide
the differences: `RowAt`, `RowTitleTop`, `RowValue`, `RowChanged`. The rest of the
spinner code (painting, clicks, drags, right-clicks, typing) serves both. Keyboard
Entry's sizes are derived from `ParametersFor` (lengths and radii only), so the two
cannot drift apart.

---

## 8. Results

- The command panel while creating a Box matches R3.1's screenshot: 0 differing pixels of
  122,771 (text, icons, the dropdown's interior and the swatch's random colour masked).
- With nothing created, the window is unchanged since Phase 4 (7 differing pixels of
  1,423,793, icon art).
- A Keyboard Entry box (X 50, 40 × 30 × 20) lands exactly where the grid says.
- Twenty-one test programs; 0 warnings in a clean build.

---

## 9. Open items

- Each primitive's creation steps; how radius 2 and the torus tube follow the mouse.
- Parameters and Creation Method lists other than Box's; non-default creation methods
  (not implemented); Keyboard Entry's layout and defaults.
- Spinner speeds, a pressed arrow's look, the gutter's exact condition.
- The Objects tab strip (needs an R3.1 screenshot of the Objects tab).
- R3's creation cursor, random colour palette, Object Color dialog.

---

## 10. Exercises

| Step | Try |
|---|---|
| 5.1 | Give the Sphere a height step; set `kMinDragPixels` to 0; swap a plane's u and v. |
| 5.2 | Record the live node in the undo history (test fails); create above the horizon. |
| 5.3a | Format with `value_` instead of `shown`; accumulate drags (tests fail). |
| 5.3b | Change `#C0FFC0`; draw radios with `DrawFrameControl` and compare under Wine. |
| 5.4 | Start Keyboard Entry open; default its fields to 10; create in another view. |
