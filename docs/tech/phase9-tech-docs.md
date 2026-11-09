# Phase 9 — Technical documentation

Phase 9 worked through what Phases 0–8 had deferred. It began with an inventory taken
from the repository itself, then built eleven steps: the File and Edit commands, the menus
and keyboard shortcuts checked against R3.1, Edit Mesh at every sub-object level with its
main buttons, the Transform Type-In, the reference coordinate systems and transform
centres, Mirror and Array. Step record: `docs/phases/phase-9-deferred.md`.

Code at the end of Phase 9: tag `phase-9-done` (version 0.9.0).

## Contents
1. Architecture and file map
2. An inventory from the repository, not from memory
3. Undo for changes that have already happened
4. Menus as data, checked against R3.1
5. Keyboard shortcuts that do not steal typing
6. Copies, subsets and summaries
7. Edit Mesh: every edit by input index
8. Topology: polygons, elements, edges
9. Welds, hiding, detaching
10. Exact values: the Transform Type-In
11. Reference frames from definitions
12. Mirrors and winding
13. Testing the instrument
14. Results
15. Open items
16. Exercises

---

## 1. Architecture and file map

New code keeps the project's rule: logic in pure, tested functions; the Windows layer thin.

| Area | Files | Role |
|---|---|---|
| Merge | `io/Merge.{h,cpp}` | Another file's objects into the scene, renamed where names clash |
| Copies, summary | `scene/Clone.{h,cpp}`, `scene/Summary.{h,cpp}` | Deep copies, cloning with hierarchy; Summary Info's totals |
| Topology | `scene/MeshTopology.{h,cpp}` | Polygons, elements, edges |
| Exact values | `scene/TypeIn.{h,cpp}` | Euler angles, world rotations, absolute and offset transforms |
| Mirror, Array | `scene/Arrange.{h,cpp}` | Mirror and array matrices; a node's transform after a world change |
| Frames | `scene/MoveMath.{h,cpp}` | `Frame`, `MakeFrame`, constraints, drag planes and rotation in a frame |
| Edit Mesh | `scene/Modifier.{h,cpp}` | `EditMeshModifier::State`, `Edited()` with maps, the editing operations |
| Picking | `view/Picking.{h,cpp}` | Faces (front-most), segments, rectangles |
| Menus | `ui/MenuSpec.{h,cpp}`, `ui/MenuBuilder.cpp`, `ui/RecentFiles.h` | R3.1's menus as data, `ShortcutsOf`, `MenuItemEnabled`, recent files |
| Dialogs | `ui/Dialogs.{h,cpp}` | Clone, Summary Info, Detach, Mirror, Array, the Type-In floater |
| Message loop | `win/App.{h,cpp}` | Two accelerator tables; a modeless dialog |

Five test programs were added (`test_merge`, `test_clone`, `test_edit_mesh`,
`test_typein`, `test_arrange`) and several extended; thirty-eight in all.

---

## 2. An inventory from the repository, not from memory

Phase 9 started by collecting every deferral from where it is actually written: the
plan's "Deferred" lines, each checklist's unticked items and "Unverified" notes, the
specs, the format notes, the code's `NOTE:` markers, and the menu commands that still
answered "not implemented yet" (105 of 113). A summary written from memory would have
missed most of these; the repository had them all.

The inventory sorted items by what they need, not by area:

- **A. Buildable now**: everything the project already knew enough to build.
- **B. Needs R3.1 evidence**: items whose look or behaviour cannot be known without a
  screenshot or a sample file. Listing them precisely makes the evidence easy to ask for:
  one request ("a screenshot of R3.1's menus open") settled a whole group in step 9.2.
- **C. Separate subsystems**: the renderer, Track View, MAXScript, the Material Editor…

The inventory became the phase's checklist, so it is ticked off as work lands.

---

## 3. Undo for changes that have already happened

Merge (9.1), Clone, Detach, Mirror with copies and Array all add nodes that are easiest
to build directly: an importer or `CloneNodes` creates them in the scene. The undo
system expects commands that make the change when executed. Rather than rewriting the
builders as commands, `AddedNodesCommand` records changes that have already happened:

- its first `Do` does nothing (the nodes are already there);
- `Undo` deletes the nodes, children first, each with a `DeleteNodeCommand`, which
  already knows how to restore a node exactly;
- `Redo` replays those deletions' undos, parents first.

Merging is thus the inverse of deleting, built entirely from existing parts. Combined with
`MacroCommand`, it gives one undo step for compound actions: Clone = the new nodes + the
selection of them; Detach = the new object + the faces removed from the original.

---

## 4. Menus as data, checked against R3.1

Phase 1 transcribed R3.1's menus from memory. Eleven screenshots of R3.1 with each menu
open (step 9.2) corrected about a dozen details: "New..." with an ellipsis, the missing
"Merge Animation...", "Properties..." rather than "File Properties...", Views' plain
"Undo" / "Redo", separators in six menus.

Everything the screenshots show became data, and the tests compare the data with the
screenshots:

- **The shortcut column** lives in the menu text after a tab (as Windows draws it), and
  `ShortcutsOf` parses it. The test lists all 23 of R3.1's shortcuts and checks each
  parses to the expected key, with no key used twice. Menu text and behaviour come from
  one source, so they cannot disagree.
- **Greying**: R3 greys 27 items in an empty scene. `MenuItemEnabled` encodes the rules,
  and the test checks that in an empty scene the greyed set is **exactly** the
  screenshots' set, across all eleven menus. Missing or extra greying both fail.
- **Ticked items**: Show Transform Gizmo and Update During Spinner Drag.

Notable fact from the screenshots: **Ctrl+A is Redo** in R3.1; later Max versions use it
for Select All.

---

## 5. Keyboard shortcuts that do not steal typing

R3.1 has single-key shortcuts: M (Material Editor), 1 (Redraw All Views), Delete,
Shift+Z. Windows translates accelerators before the focused control sees the key, so
with a single table, typing "m" in an object's name field would open the Material Editor.

The message loop uses two accelerator tables:

- **always**: shortcuts with Ctrl or Alt, and function keys, which typing never produces;
- **plain**: single keys and Shift+key, used only when the focused window is not an
  `Edit` control.

`Shortcut::WhileTyping()` decides which table each shortcut belongs in, and the test
checks the classification. In the running program, typing "Bm1x" into the Name field
produced exactly that text, while M with no field focused opened the Material Editor.

The Transform Type-In (9.8) added a **modeless** dialog, which needs the loop's help too:
Tab, Enter and Escape only work in a modeless dialog if the loop passes its messages
through `IsDialogMessage`.

---

## 6. Copies, subsets and summaries

**Deep copies** (`CopyObject`, 9.3) rebuild an object from its kind: a primitive from its
parameters, an Editable Mesh from its mesh, a Derived Object from a copied base and copied
modifiers. Two refinements:

- objects shared among the originals stay shared among the copies: copying seven
  instances gives seven boxes sharing **one** new object (R3's behaviour unverified);
- a clone whose parent is also cloned is linked under the parent's **clone**; otherwise
  it keeps the original parent. Linking at identity and then setting the original's local
  transform puts every clone exactly on its original.

**Save Selected** writes a subset. A selected child whose parent is left out must keep its
world position, so its world transform becomes its own, decomposed by the scene's own
rule (`Scene::SetWorldMatrix`) on a scratch node. Reusing the scene's decomposition, not
writing a second one, keeps the two consistent.

**Summary Info**'s numbers are checked against R3.1's own Summary Info dialog for the
seven samples it was shown with (Teapot01 (Teapot): 530 vertices, 1,024 faces, …).

---

## 7. Edit Mesh: every edit by input index

Before Phase 9, Edit Mesh stored only vertex moves, by vertex index. Deleting faces or
vertices renumbers a mesh: had Edit Mesh stored "face 5" in its **output** numbering, a
later deletion would silently change which face that means.

The design (9.4): every edit is stored by the index of the mesh coming **into** Edit Mesh,
which never changes. `Edited(input, vertexMap, faceMap)` applies the edits in a fixed
order and produces the output together with **maps** from output indices back to input
indices:

1. moves;
2. welds (9.6): each merged vertex follows its chain to the vertex that remains;
3. deletions: deleted faces, and any face using a deleted or merged-away vertex;
4. hidden faces (9.6);
5. faces collapsed by a weld (two corners now the same vertex).

The views draw and pick the output and translate through the maps, so a selection
stays correct after part of the mesh was deleted. The test checks the subtle case: after
deleting a vertex, a move recorded earlier by input index still lands on the right vertex
of the renumbered output.

All of Edit Mesh's editing state is one value, `State`. Undo (`EditMeshStateCommand`
records it before and after), `.mlk` files and copies each needed one change, and pick up
any later field automatically.

---

## 8. Topology: polygons, elements, edges

In R3, the Polygon and Element levels select **faces**:

- a **polygon** is the faces joined across edges hidden in both faces (a box side's two
  triangles share a hidden diagonal);
- an **element** is the faces joined by shared vertices (one connected piece).

Both are floods over the mesh's edges or vertices (`PolygonOf`, `ElementOf`). Edges are
named face × 3 + side, so an edge selection is a list of numbers like everything else.
Deleting an edge deletes the faces on both sides (`FacesOnEdge`).

Picking expands on the mesh **as displayed** (after Edit Mesh's own edits), then maps
back to input faces, so a polygon is what the user sees, even after deletions.

Structure: step 9.4 chose between vertices and faces with `if`s in several places. Rather
than multiply those for three more levels, two helpers, `SubObjectsAt` and
`SubObjectsIn`, answer "what is under the cursor / in this rectangle at the current
level" for all five levels, in Edit Mesh's own numbering.

---

## 9. Welds, hiding, detaching

- **Weld Selected** (9.6) groups the selected vertices into clusters within the threshold
  (union-find over pairs), keeps the lowest index of each cluster, moves it to the
  cluster's average, and records the others as merged into it. The test checks that
  welding two box corners collapses **exactly** the two faces that used both.
- **Weld Target** (9.7) merges one vertex into another, which stays where it is.
- **Hide** leaves faces out of Edit Mesh's output. R3 keeps them in the mesh with a flag;
  without a renderer the two look the same (noted).
- **Detach** (9.7) copies the selected faces out of the output into a compact new mesh
  (`Extract`: vertices renumbered, smoothing and material IDs kept), creates a node for
  it with the original's world placement, and removes the faces from the original: one
  `MacroCommand` of `AddedNodesCommand` and `EditMeshStateCommand`.

---

## 10. Exact values: the Transform Type-In

The Type-In (9.8) needs the inverse of Phase 6's `EulerXYZDegrees`: X first, then Y, then
Z means R = Rz·Ry·Rx, and since `Quat`'s `*` applies its right operand first, the
inverse is that product of three axis rotations.

Absolute rotation, offsets, Mirror and Array all change a node's **world** matrix and need
its new local transform. They share one approach: build the new world matrix, solve it
with `Scene::SetWorldMatrix` (the same solver Move and linking use), read the transform,
and restore the node. The functions change nothing themselves; their results go into
`TransformCommand`s. Parented objects come out right without special cases.

The tests check results by **measuring the world** after applying them (where the pivot
lands, where an axis points), not by repeating the formulas. A test that reuses the code's
formula can only confirm the formula agrees with itself.

---

## 11. Reference frames from definitions

Phase 6 built the transforms on **world** axes. Step 9.9 began by checking R3's documented
definition of its default system: *View: orthographic views use the Screen coordinate
system, perspective views use World.* So in the Front view, R3's "Y" is up the screen
(world Z); maxlike's was world Y, pointing into the screen, and a Y-restricted move did
nothing.

Phase 6's tests had passed because they encoded the same assumption, and its gizmo
measurements came from the Top and Perspective views, where the two definitions agree. The
correction came from re-deriving behaviour from the reference, not from the existing code.

The design: a `Frame` is three world-space axes and an origin. `MakeFrame` builds it per
system (View, Screen, World, Parent, Local, Grid) and per view. Move constrains and builds
its drag plane in the frame, Rotate turns about the frame's axis, the gizmo is drawn along
it. The transform centre (each pivot, the selection's bounding-box centre, or the frame's
origin) is captured when a drag starts, since a selection's centre moves as it turns.

---

## 12. Mirrors and winding

A mirror has a negative determinant. Phase 3 had already provided for it: the flip goes
into a negative X scale, since a reflection cannot be a rotation.

A reflection also **reverses the winding** of every face, and two parts of the viewport
decide "front" by winding:

- the wireframe culling (Phase 8): a mirrored object would hide its visible edges;
- the shader's two-sided lighting (`if (!gl_FrontFacing) n = -n;`): every face of a
  mirrored object counted as a back face, and it rendered dark.

Normals were never the problem: they are transformed with the inverse transpose, which
handles reflections correctly. I checked that before testing and concluded shading was
safe, missing the shader's own use of winding. The check in the running program caught
it: mirroring a symmetric box must change **nothing**, so any difference is a bug. The fix
is one predicate, `math::Mirrors(m)`, used to flip the culling test and to emit a mirrored
object's triangle corners in reverse order.

---

## 13. Testing the instrument

Phase 9 had more checks in the running program than any phase before, and several times
the **check** was wrong rather than the program. The recurring lesson: before concluding
"maxlike is broken" or "maxlike works", verify the instrument.

| Step | What looked wrong | What it was | Remedy |
|---|---|---|---|
| 9.2 | Ctrl+Z "did nothing" | a click "to focus a viewport" was itself an undoable deselection | treat every test action as an input |
| 9.2, 9.4 | numbers from an earlier run | the virtual display had stopped; the run did nothing | scripts start the display; wait for conditions, not for time |
| 9.3 | menu keys ignored after a dialog | X kept focus on the destroyed dialog (no window manager) | instrumented: the menu opened, no command arrived; send keys to the main window |
| 9.4 | no box found, wrong counts | a blank first capture during a slow start-up | wait until the window exists and the box is drawn |
| 9.7 | Weld Target "failed" | (a) real bug: code inserted inside the wrong block; (b) the check compared the Front view, which cannot see the change | debug output along the chain; a view that shows the change |
| 9.7 | panel clicks missed | a button below the panel's edge; mouse-wheel delivery unreliable | capture every step; collapse a rollout instead of scrolling |
| 9.10 | Array spacing wrong | Ctrl+A does not select an edit field's text here: values were appended | clear fields explicitly |

Two techniques settled the hard cases quickly: **temporary debug output** along a chain
of events (9.3, 9.7), removed afterwards by restoring a backup and diffing; and
**capturing every step** of a sequence instead of reading one number at the end.

Two real bugs were also of the "invisible to the compiler" kind:

- **9.7**: a script inserted code after "the first `{`" following a `case` label. That
  case has no brace of its own, so the code landed inside `if (moving_) {`, valid C++
  that could never run. Text-based edits need their anchor checked as carefully as their
  content.
- **9.8**: `windows.h` defines `GetObject` as a macro. The project stays consistent by
  including `<windows.h>` before the scene headers everywhere; a new header broke that
  order and `Node::GetObject` was renamed in some places only. The rule now has a comment
  where it matters.

---

## 14. Results

- 25 of 114 menu commands work (from 8 at the start), plus the toolbar's coordinate
  systems and centres. The menus, shortcuts and greying match eleven R3.1 screenshots.
- Edit Mesh works at all five levels: select (click, region, Ctrl / Alt), Move, Delete,
  Hide / Unhide All, Weld Selected and Target, Detach; everything undoable, kept in
  `.mlk` files and clones.
- Summary Info matches R3.1's dialog for seven samples; the Edit Mesh panel still matches
  R3.1 pixel for pixel; the default window is unchanged.
- Thirty-eight test programs; 0 warnings.

---

## 15. Open items

- Phase 10 (buildable now): the Group menu, Align, the modifier gizmo, viewports and the
  selection in `.mlk` files, Edit Mesh's Create and other buttons, Rotate and Scale at
  sub-object level, Soft Selection.
- Evidence needed: dialogs built from memory (New, Clone, Detach, Mirror, Array, Transform
  Type-In), Edit Mesh's look and wording at sub-object levels, the Select By / Region /
  Grids submenus, menu mnemonics, Pick, the perspective grid's extent, Bend / Taper /
  Twist and Backface Cull in files, materials.
- Simplifications to revisit: Local / Parent with several objects use the first object's
  axes; Scale always uses each object's own axes; hidden faces are left out of the mesh.

---

## 16. Exercises

| Step | Try |
|---|---|
| 9.1 | Open `box.max`, Merge `parent_child_link.max`: the incoming box becomes Box02, still under the sphere. Undo, Redo. |
| 9.2 | Click into an object's Name field and type "m1": it types. Click a viewport and press M. |
| 9.3 | Clone a parent and child together, then the child alone: compare where each clone is linked. |
| 9.4 | Delete one face of a box with and without isolated vertices; Undo each. |
| 9.5 | On two boxes collapsed into one Editable Mesh, compare Polygon and Element selection. |
| 9.6 | Weld two box corners: how many faces disappear, and why exactly those? |
| 9.8 | With a linked object selected, type an absolute rotation; then check its parent did not move. |
| 9.9 | In the Front view, restrict Move to Y under View, then under World: compare. |
| 9.10 | Mirror a box with No Clone, then switch to shaded view: it must look exactly as before. |
