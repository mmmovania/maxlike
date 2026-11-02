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
- `help.about` shows an About box.
- Every other command shows its name in the status-area placeholder.
