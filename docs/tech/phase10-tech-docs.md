# Phase 10 — Technical documentation

Phase 10 finished list A of Phase 9's inventory: groups, Align, the modifier gizmo, views
and selection in `.mlk` files, Rotate and Scale at sub-object level, Soft Selection, and
Edit Mesh's Edit Geometry buttons. Step record: `docs/phases/phase-10-groups.md`.

Code at the end of Phase 10: tag `phase-10-done` (version 0.10.0).

## Contents
1. Architecture and file map
2. Groups: one rule for what a click selects
3. Undo order inside compound commands
4. Align
5. The modifier gizmo: wrapping verified code without changing it
6. Views and selection in files
7. Sub-object Rotate and Scale through world space
8. Soft Selection
9. Edit Mesh that adds geometry
10. Topology operations, tested by structure
11. Slicing and cutting: one splitting rule
12. Testing notes
13. Results
14. Open items
15. Exercises

---

## 1. Architecture and file map

| Area | Files | Role |
|---|---|---|
| Groups | `scene/Groups.{h,cpp}` | `SelectableFor`, members, bounds; Group / Ungroup / Explode / Detach / Attach builders |
| Align, Mirror, Array | `scene/Arrange.{h,cpp}` | `Aligned`, `FrameScale`, `SubObjectDeltas` |
| Deformers | `scene/Modifier.{h,cpp}` | `DeformModifier`: gizmo, centre, `GizmoLines` |
| Soft Selection | `scene/SoftSelection.{h,cpp}` | Max's affect-region curve, per-vertex weights |
| Edit Mesh | `scene/Modifier.{h,cpp}` | added geometry, `Extended` / `Positions`, the Edit Geometry operations |
| Files | `io/NativeFile.{h,cpp}` | group flags, gizmos, views, selection, added geometry |
| Undo | `scene/Undo.{h,cpp}` | `GroupOpenCommand`, `GizmoCommand`; group flags kept by `DeleteNodeCommand` |
| Panel | `ui/ParametersSpec.cpp`, `ui/CommandPanel.cpp` | Soft Selection's rollout; Edit Geometry by level and Slice mode |
| Views | `ui/Viewport.{h,cpp}` | group outlines, the gizmo, soft weights, tool clicks, the slice plane |

Two test programs were added (`test_groups`, `test_soft_selection`) and several extended;
forty in all.

---

## 2. Groups: one rule for what a click selects

A group is an object-less node, the head, with its members linked under it, so moving a
group is ordinary parent linking. Everything about selecting through groups follows from
one function, `SelectableFor(node)`: the outermost closed group above a node, else the
node. Clicks, rectangles, Select All and Invert all map through it; an open group lets its
members be picked individually. Operations that act on "the selection" (Delete, Clone,
Save Selected, Mirror, Array) use `WithMembers` to include a group's members.

The commands are assembled from existing undo commands (`AddedNodesCommand`,
`LinkCommand`, `DeleteNodeCommand`, `SelectCommand`), so their undo needed no new
machinery; only the group flags joined `DeleteNodeCommand`'s snapshot.

---

## 3. Undo order inside compound commands

A `MacroCommand` undoes its parts in reverse order. Ungroup first freed the members,
deleted the head, and then selected the members. Undoing it restored the old selection
(the group) before the group existed again, and a selection cannot hold a missing node:
after undo, nothing was selected. The fix was to change the selection first. A test now
checks the selection after the undo, and was confirmed to fail on the old order.

Rule: in a compound command, change the selection before removing what it refers to, and
after creating what it refers to.

---

## 4. Align

`Aligned(node, target, options)` computes the node's new transform: orientation first
(about its own pivot), then scale, then position, because the points being aligned
(minimum, centre, maximum of the world bounding box, or the pivot) depend on the first two.
Groups use their members' bounds. The target is picked with a one-shot pick mode, the same
mechanism Group > Attach introduced.

---

## 5. The modifier gizmo: wrapping verified code without changing it

Bend, Taper and Twist were verified against R3.1 in Phase 7. The gizmo had to be added
without any risk to those results. The new base class `DeformModifier` wraps the existing
algorithms: points go into gizmo space (relative to the centre), the unchanged algorithm
deforms them, and they come back. With the default gizmo the transformation is skipped
entirely, so the old results cannot change. Phase 7's tests were run against the code
both before and after the change.

The lattice is drawn by the same deformation (the input's bounding box, subdivided, passed
through the modifier), so it cannot drift from the object; a test checks that a bent
lattice's corners land on the bent box's corners.

---

## 6. Views and selection in files

The view state is plain data in the Windows-free `view/` layer (`OrthoView`,
`PerspectiveView`), so `.mlk` files take it directly through an optional `NativeViews`.
Older files lack the chunks and open as before. The test fills every field of four views
with distinctive values and checks each one comes back.

---

## 7. Sub-object Rotate and Scale through world space

Each vertex moves through world space: object space to world, through the turn or scale
about the centre, back to object space (`SubObjectDeltas`). Rotating in object
coordinates would skew vertices on a non-uniformly scaled object; the test checks rigidity
on an object scaled 2x. The result is stored as Edit Mesh's ordinary per-vertex moves, so
undo, files and copies needed no change.

---

## 8. Soft Selection

The falloff is Max's documented "affect region" curve:
u = (falloff − d) / falloff; weight = (3u·bubble·(1−u) + 3u²·(1−pinch))·(1−u) + u³,
which with Pinch and Bubble at 0 is 3u² − 2u³. The test checks it against hand-computed
values. A drag widens to every vertex with a positive weight and multiplies each vertex's
offset by its weight; Edge Distance limits the effect by a breadth-first walk over edges.

---

## 9. Edit Mesh that adds geometry

Phase 9 stored every edit by the input's indices. Creating geometry needed new indices that
cannot collide: added vertices and faces are numbered after the input's
(input.size() + k). `Edited()` appends them first, so moves, welds, deletion, hiding,
selection and the output maps all apply to them unchanged. Operations work on
`Extended(input)` (input plus added) or `Positions(input)` (with the moves applied).

Faces an operation changes are deleted and added again, rather than edited in place: the
existing deletion machinery then does the bookkeeping.

---

## 10. Topology operations, tested by structure

Counting vertices and faces would let a wrongly wound wall or a duplicated face pass. The
tests use a structural property instead: a closed, consistently oriented mesh uses every
directed edge exactly once with its reverse also present, and satisfies Euler's formula
V − E + F = 2. A box starts that way; Turn, Extrude, Chamfer (one and two corners), Slice
and Cut must keep it so, and Break (which opens the mesh) must not, which shows the check
can fail.

Chamfer had a flaw found in review before testing: it scanned the faces as they were
before the operation, so a second selected vertex re-cut faces the first had already
replaced. Each vertex now sees the mesh as left by the previous ones; a test chamfers two
corners that share faces.

---

## 11. Slicing and cutting: one splitting rule

Given new points on some edges, `SplitByEdgePoints` splits every face with points on two
edges into three triangles along the line between them, and every face with a point on one
edge into two triangles to the opposite corner. Because neighbouring faces split at the
same shared points, a closed mesh stays closed. Slice puts the points where the plane
crosses edges (Split gives the positive side its own copies); Cut puts them on two edges of
one face, and the faces across those edges split by the same rule.

---

## 12. Testing notes

- **Wait for conditions, again.** The Edit Mesh panel comparison still used a fixed start-up
  wait; after a display restart it captured an empty window and reported 25,590
  differences. It now waits for the window to be drawn, like the other scripts since 9.4.
- **Test the thing, not the dialog.** Saving views was first tested through Wine's Save
  dialog, which is not painted on the test display. Four attempts failed (one through a
  path whose backslashes had been doubled by escaping). An already-saved `.mlk` saves with
  Ctrl+S without a dialog; the starting file came from the tested I/O code.
- **Know what overlaps.** After Slice, the cut line was not found in the Front view because
  the slice plane, seen edge-on, was drawn exactly over it; and a created vertex is
  selected, so it is red, not blue. Both "failures" were the measurement.
- **Line endings.** A text insertion matched `\nint main` in a CRLF file and left a stray
  `\r`; packaging compares the applied patch with the tree and caught it.

---

## 13. Results

- 33 of 114 menu commands; the Group menu and Align added. Edit Mesh: all five levels,
  every Edit Geometry button except Create Shape from Edges, Soft Selection, sub-object
  Rotate and Scale. Deformer gizmos. Views and selection kept in `.mlk` files.
- The Edit Mesh panel still matches R3.1 pixel for pixel at Vertex level; the default
  window is unchanged.
- Forty test programs; 0 warnings.

---

## 14. Open items

- Not built or simplified: Create Shape from Edges (no shapes), Extrude and Chamfer at Edge
  level, Extrude by dragging, Cut across several faces in one stroke, Tessellate's Tension,
  Affect Backfacing, the falloff curve display, undoable slice-plane moves.
- From memory, needing R3.1 evidence: the button availability above Vertex level, the
  dialogs (Group name, Align, Detach, Explode), Soft Selection's layout and colours, the
  gizmo colours, open groups' look, the prompts.
- Groups and gizmos in `.max` files are not read.

---

## 15. Exercises

| Step | Try |
|---|---|
| 10.1 | Group three boxes, open the group, move one box, close it; undo each step. |
| 10.2 | Align a box's minimum X to another's maximum X, then orientation on one axis only. |
| 10.3 | On the bent box of `--demo-scene deform`, move the gizmo, then the centre: compare. |
| 10.4 | Save with a selection and a zoomed view; reopen. |
| 10.5 | On a box scaled 200 % in X, rotate the top polygon: it stays square. |
| 10.6 | On the teapot, soft-select one spout vertex; change Pinch and Bubble and watch the colours. |
| 10.8 | Extrude a box's top twice, then Chamfer one of the new corners. |
| 10.9 | Slice a sphere with Split on, then move one half's faces at Element level. |
