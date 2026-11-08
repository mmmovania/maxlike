# Menus

Source of truth in code: `src/ui/MenuSpec.cpp` (data), built by `src/ui/MenuBuilder.cpp`.
Commands are referred to by name (`file.open`); `CommandTable` maps names to WM_COMMAND IDs.

## Status
| Menu | Title | Items, order, separators | Underlined letters |
|---|---|---|---|
| File | ✔ | ✔ | ○ |
| Edit | ✔ | ✔ | ○ |
| Tools | ✔ | ✔ | ○ |
| Group | ✔ | ✔ | ○ |
| Views | ✔ | ✔ | ○ |
| Rendering | ✔ | ✔ | ○ |
| Track View | ✔ | ✔ | ○ |
| Schematic View | ✔ | ✔ | ○ |
| Customize | ✔ | ✔ | ○ |
| MAXScript | ✔ | ✔ | ○ |
| Help | ✔ | ✔ (About renamed for maxlike) | ○ |

✔ verified against R3.1 · ○ unverified

Items were corrected against R3.1 by the project owner. Underlined (mnemonic)
letters still need checking: press Alt in R3.1 and compare.

## Rules (enforced by tests/test_menu_spec.cpp)
- Top-level titles match the reference, in order.
- Every item is exactly one of: separator, submenu, command.
- Command names are unique and look like `menu.item`.
- Separators are never first, last, or doubled.
- Mnemonic (underlined) letters are unique within each menu.
- Keyboard shortcut text is omitted until accelerators are implemented.

## Behaviour so far
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
