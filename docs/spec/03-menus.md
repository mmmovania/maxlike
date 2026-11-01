# Menus

Source of truth in code: `src/ui/MenuSpec.cpp` (data), built by `src/ui/MenuBuilder.cpp`.
Commands are referred to by name (`file.open`); `CommandTable` maps names to WM_COMMAND IDs.

## Status
| Menu | Title | Items |
|---|---|---|
| File | ✔ (reference screenshot) | ○ from memory |
| Edit | ✔ | ○ |
| Tools | ✔ | ○ |
| Group | ✔ | ○ |
| Views | ✔ | ○ |
| Rendering | ✔ | ○ |
| Track View | ✔ | ○ |
| Schematic View | ✔ | ○ |
| Customize | ✔ | ○ |
| MAXScript | ✔ | ○ |
| Help | ✔ | ○ ("About" renamed for maxlike) |

✔ verified against R3.1 · ○ unverified

To verify a menu: open it in R3.1, take a screenshot, and correct `MenuSpec.cpp`
(text, order, separators, submenus, underlined letter). Then mark it ✔ here.

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
