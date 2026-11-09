# Phase 10 — Groups and the rest of list A

Branch: `phase/10-groups` (from `main` after Phase 9).

Phase 10 continues Phase 9's inventory (`docs/phases/phase-9-deferred.md`, list A) with the
items still buildable without new R3.1 evidence.

## Steps
- [x] **10.1** Groups: Group, Ungroup, Open, Close, Explode, Detach, Attach (Group menu),
      each one undo step; picking selects a closed group through any member; selected
      groups draw their members white and get one bracket box; open groups show an
      outline; Delete, Clone, Save Selected, Mirror and Array take whole groups; group
      flags kept in `.mlk` files and clones; the Group menu's greying follows the selection
- [x] **10.2** Tools > Align (Alt+A): pick the target, then align position (minimum,
      centre, pivot or maximum of the current and target objects, per world axis),
      orientation (local axes) and scale; one undo step; groups by their members' bounds
- [x] **10.3** The modifier gizmo: Bend, Taper and Twist deform in their gizmo's space
      about its centre; the gizmo is drawn as the input's box deformed by the modifier
      (orange; yellow at its sub-object level); Sub-Object levels Gizmo (Move, Rotate,
      Scale) and Center (Move), one undo step per drag; kept in `.mlk` files and clones
- [ ] Viewports and the selection in `.mlk` files
- [ ] Edit Mesh: Create, Chamfer and the other Edit Geometry buttons; Rotate and Scale at
      sub-object level; Soft Selection
- [ ] Wrap-up

## Step 10.3 verification
- [x] Builds warning-free; thirty-nine test programs pass (MinGW + Wine)
- [x] `test_modifiers`: Phase 7's deformation tests pass unchanged (run against the code
      before and after the change); a centre offset equals moving the gizmo by it; a
      moved gizmo equals shifting, deforming and shifting back; the lattice is the box's
      twelve edges, and bent, its corners are exactly the bent box's corners; the centre
      in object space follows the gizmo. `test_native_file`: a gizmo survives saving and
      loading, the loaded object evaluating bit-identically
- [x] Under Wine with `--demo-scene deform`: the bent box in Modify shows an orange cage
      bent like it; at Sub-Object the cage is yellow; dragging the gizmo 60 px changes the
      bend; Ctrl+Z restores every view pixel for pixel
- [ ] On Windows: Rotate and Scale of the gizmo, Center level, Taper and Twist, undo
- [ ] Unverified: R3's gizmo colours (orange, yellow), its lattice's subdivisions, where a
      press must be to drag the gizmo (anywhere here), rotation about the centre; gizmos
      in `.max` files are not read

## Step 10.2 verification
- [x] Builds warning-free; thirty-nine test programs pass (MinGW + Wine)
- [x] `test_arrange`: current Minimum X to target Maximum X puts the object flush against
      the target's side; centres and pivots on all axes; orientation on all three axes
      gives the target's rotation with the pivot unmoved, on one axis turns that axis onto
      the target's; scale per axis
- [x] Under Wine on `instances.max`: the leftmost box aligned to the rightmost (X Position,
      Center to Center) lands on exactly its extent (x 738..819); the dialog is titled
      "Align Selection (Box07)"
- [ ] On Windows: every option combination; a group as target and as selection; undo
- [ ] Unverified: the dialog's layout (from memory), R3's live preview and Apply button
      (not built), its rule for one or two orientation axes, the pick prompt

## Step 10.1 verification
- [x] Builds warning-free; thirty-nine test programs pass (MinGW + Wine)
- [x] `test_groups`: grouping keeps every member in place and puts the head at their
      centre; picking any member selects the group, an open group's members one by one;
      moving the head moves the group; undo / redo of Group, Open, Explode (nested groups),
      Ungroup, Detach, Attach, with positions checked in the world; undoing restores the
      selection; Attach refuses a group into itself. `test_native_file`: closed and open
      heads survive saving and loading. `test_menu_spec`: the Group menu's rules, and R3's
      empty-scene greying unchanged
- [x] Under Wine on `instances.max`: Select All, Group: one object, all seven boxes white;
      clicking one box selects the whole group; Ungroup: seven objects; Ctrl+Z: the group,
      selected; Group > Open: the outline spans exactly the boxes (x 60..819)
- [x] Found: Ungroup's macro selected the freed members last, so undo restored the old
      selection (the group) before the group existed again, and nothing was selected. The
      selection is now changed first; a new check fails on the old order and passes on
      the new
- [ ] On Windows: every Group command with nested groups; Attach by clicking; Delete,
      Clone, Mirror and Array of a group; saving and reopening
- [ ] Unverified: R3's group dialog, its pink display of open groups (here an outline in
      #FF80FF), the head's pivot (here the members' bounding-box centre), whether a group
      keeps its members' common parent (here top level), the status wording for a
      selected group, Attach's prompt; groups in `.max` files are not read yet
