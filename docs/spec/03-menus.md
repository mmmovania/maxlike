# Menus

Source of truth in code: `src/ui/MenuSpec.cpp` (data), built by `src/ui/MenuBuilder.cpp`.
Commands are referred to by name (`file.open`); `CommandTable` maps names to WM_COMMAND IDs.

## Status
Every menu was checked item by item against screenshots of R3.1 with each menu open
(`docs/reference/screenshots/r3.1-menu-*.png`, step 9.2): labels, order, separators, the
shortcut column, the ticked items, and which items R3 greys in an empty scene.

| Menu | Items, order, separators | Shortcuts | Greying | Underlined letters |
|---|---|---|---|---|
| File | ✔ (+ recent files 1–9 above Exit) | ✔ | ✔ | ○ |
| Edit | ✔ (Select By / Region contents ○) | ✔ | ✔ | ○ |
| Tools | ✔ | ✔ | ✔ | ○ |
| Group | ✔ | — | ✔ | ○ |
| Views | ✔ (Grids contents ○) | ✔ | ✔ | ○ |
| Rendering | ✔ | ✔ | ✔ | ○ |
| Track View | ✔ | — | ✔ | ○ |
| Schematic View | ✔ | — | ✔ | ○ |
| Customize | ✔ | — | ✔ | ○ |
| MAXScript | ✔ | ✔ | ✔ | ○ |
| Help | ✔ (About renamed for maxlike) | — | ✔ | ○ |

✔ verified against R3.1 · ○ unverified · — none in R3.1

Corrections the screenshots brought (Phase 1 had these from memory): "New...", "Merge
Animation..." (missing), "Properties..." (was "File Properties..."), "Align Camera...",
Group's first item without "...", Views' "Undo" / "Redo" (were "... View Change"), "View
Preview...", "Online Reference..." and "MAXScript Reference..." with "...", and the
separators in Edit, Views, Rendering, Customize, MAXScript and Help.

Shortcuts (from the shortcut column; `ui::ShortcutsOf`, Windows accelerators): Ctrl+N New,
Ctrl+O Open, Ctrl+S Save, Ctrl+Z Undo, **Ctrl+A Redo** (R3.1; later Max versions use it
for Select All), Alt+Ctrl+H Hold, Alt+Ctrl+F Fetch, Delete, F12 Transform Type-In, Alt+A
Align, Alt+N Normal Align, Ctrl+H Place Highlight, M Material Editor, Shift+I Spacing
Tool, Shift+Z / Shift+A view Undo / Redo, Alt+B Viewport Background, Alt+Shift+Ctrl+B
Update Background Image, Ctrl+C Match Camera to View, 1 Redraw All Views, Ctrl+X Expert
Mode, Ctrl+I Show Last Rendering, F11 MAXScript Listener. Ctrl / Alt and function-key
shortcuts work everywhere; single keys and Shift+key ("M", "1", Delete, Shift+A) not while
a text field has focus, so typing is never taken for a shortcut.

Greying (`ui::MenuItemEnabled`): Undo / Redo with nothing to undo or redo; Clone, Select
None, Properties, Save Selected, Export Selected and Group without a selection; Select All
and Select Invert without objects; Restore Active ... View until that view was saved; and
always, for things that cannot exist yet: Merge Animation, Views' Undo / Redo, the
background image, Match Camera to View, Add Default Lights, Show Last Rendering, Delete
Track / Schematic View, and Group's Open, Close, Ungroup, Explode, Detach, Attach. Edit >
Delete is never greyed (as in R3.1).

Ticked at start: Show Transform Gizmo (unticked, Move shows the plain tripod) and Update
During Spinner Drag (NOTE: no effect yet). Views > Save / Restore Active View are named
after the active viewport ("Save Active Top View"). Recent files: the nine most recently
opened or saved, newest first, kept in the registry (`HKCU\Software\maxlike\Recent File
List`; R3 keeps them in 3dsmax.ini).

## Rules (enforced by tests/test_menu_spec.cpp)
- Top-level titles match the reference, in order.
- Every item is exactly one of: separator, submenu, command.
- Command names are unique and look like `menu.item`.
- Separators are never first, last, or doubled.
- Mnemonic (underlined) letters are unique within each menu.
- Keyboard shortcut text is omitted until accelerators are implemented.

## Behaviour so far
32 of the 114 menu commands work after step 10.1 (25 at the end of Phase 9) (counted from `MainFrame` against
`MenuSpec`); the rest show the "not implemented yet" prompt below.

- `file.exit` posts `WM_CLOSE` (same path as the close button).
- `help.about` shows an About box with the version.
- `edit.undo` / `edit.redo` undo and redo scene changes (Phase 3). Before the Edit menu
  opens, their text becomes "Undo <name>" / "Redo <name>", greyed when there is nothing
  to undo or redo (R3's exact wording unverified). Undoable changes so far: creating
  objects (Phase 5), parameter edits, selecting, deleting, renaming, recolouring, Move /
  Rotate / Scale (Phase 6), adding, removing and switching off modifiers, modifier
  parameters, vertex selection and vertex moves (Phase 7).
- `edit.delete` deletes the selection as one undo step (Phase 6; also the Delete key).
- `file.new` replaces the scene with an empty one, keeping the views (R3's New All;
  its Keep Objects options are not built); `file.reset` also restores the startup views,
  after asking; `file.merge` adds another file's objects (`.max` or `.mlk`) as one
  undoable step, renumbering names already in the scene (Phase 9, step 9.1).
- New, Reset, Open, dropping a file and closing the window first ask to save a modified
  scene (Yes saves, Cancel stops); the startup scene counts as saved.
- `edit.select_all`, `edit.select_none`, `edit.select_invert`: one undoable "Select" step
  each; `views.redraw_all` redraws every viewport (9.1).
- `edit.clone` asks Copy / Instance and a name (one object), then adds the clones as one
  undoable step and selects them; a clone's parent is the parent's clone when both are
  cloned. `file.save_selected` writes only the selection to a `.mlk` (a node whose parent
  is left out keeps its world position). `file.summary_info` shows R3's Summary Info
  dialog: scene and mesh totals, memory, the objects table, Description, Save to File...
  (step 9.3).
- `tools.transform_type_in` (F12) opens the modeless Transform Type-In: Absolute:World
  (one object) and Offset:World X / Y / Z for the active transform tool; Enter applies the
  group typed in as one undoable step (step 9.8).
- `tools.mirror` and `tools.array` mirror (axis or plane of the reference system, about
  the transform centre; No Clone / Copy / Instance) or array (incremental move, rotate,
  scale; 1D / 2D / 3D) the selection as one undoable step (9.10).
- `views.save_active` / `views.restore_active` keep one view per view type, named after
  the active viewport; `views.show_gizmo` switches Move's gizmo for the plain tripod;
  `views.update_during_spinner_drag` is only a tick for now (9.2).
- `file.recent.1` … `file.recent.9`: the recent files, inserted above Exit when the File
  menu opens (9.2).
- Toolbar (not menus): `tool.ref_coord` (the reference coordinate system) and
  `tool.pivot_point` / `tool.pivot_selection` / `tool.pivot_transform` (the transform
  centre) work since step 9.9.
- The Group menu (10.1): `group.group` asks a name ("Group01") and links the topmost
  selected objects under a new group head; `group.ungroup` / `group.explode` dissolve
  the selected groups (Explode: nested ones too); `group.open` / `group.close`;
  `group.detach` takes members out of an open group; `group.attach` links the selection
  to the group picked next. Each is one undoable step; greyed by the selection.
- `file.save` writes the scene to its `.mlk` file, or asks for a name (Save As) if it has
  none or was opened from a `.max` file; `file.save_as` always asks (Phase 8, step 8.7).
  Saving marks the scene clean.
- `file.open` opens a `.max` file (Phase 8) with the standard Open dialog ("3D Studio MAX
  (*.max)"); the same happens for a file dropped on the window or named on the command
  line. The scene is replaced only if the file can be read; undo is cleared; the title
  becomes the file's name; what could not be imported is listed in a message (R3's own
  dialogs for missing plug-ins are not reproduced).
- Every other command shows "<command>  (not implemented yet)" on the status area's
  second prompt line.
