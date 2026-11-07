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
- Every other command shows "<command>  (not implemented yet)" on the status area's
  second prompt line.
