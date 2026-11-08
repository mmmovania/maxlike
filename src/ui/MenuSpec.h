#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ui {

// One entry of a menu, described as data. Three shapes:
//   separator : text empty
//   submenu   : children not empty  (command empty)
//   command   : command not empty   ("file.open")
struct MenuItem {
    std::wstring text;              // with '&' before the mnemonic letter, e.g. L"&Open..."
    std::string command;
    std::vector<MenuItem> children;
    bool checked = false;           // ticked at start (R3: Show Transform Gizmo, Update During Spinner Drag)

    bool IsSeparator() const { return text.empty(); }
    bool IsSubmenu()   const { return !children.empty(); }
};

// The 3ds Max R3.1 menu bar. See docs/spec/03-menus.md for status.
const std::vector<MenuItem>& MainMenuSpec();

// A keyboard shortcut from the menus' shortcut column (the text after a tab).
// `key` is a Windows virtual-key code: 'A'..'Z', '0'..'9', 0x70.. for F1.., 0x2E Delete.
struct Shortcut {
    std::string command;
    uint16_t key = 0;
    bool ctrl = false, alt = false, shift = false;
    // Works while a text field has focus (Ctrl or Alt, or a function key); a plain or
    // Shift-only key would interrupt typing ("M", "1", Shift+A).
    bool WhileTyping() const { return ctrl || alt || (key >= 0x70 && key <= 0x87); }
};
// Every shortcut in a menu spec; `unparsed` collects shortcut texts it cannot read.
std::vector<Shortcut> ShortcutsOf(const std::vector<MenuItem>& spec, std::vector<std::wstring>* unparsed = nullptr);

// R3's greying (measured in the r3.1-menu-*.png screenshots of an empty scene): whether a
// command is available. Commands for things that cannot exist yet (a background image,
// cameras, renderings, saved Track or Schematic Views, groups) stay greyed.
struct MenuState {
    bool canUndo = false, canRedo = false;
    size_t objects = 0, selected = 0;
    bool savedPerspective = false;   // Views > Save Active Perspective View was used
};
bool MenuItemEnabled(const std::string& command, const MenuState& state);

// Text without '&' and without anything after a tab, e.g. L"&Open...\tCtrl+O" -> L"Open...".
std::wstring PlainMenuText(const std::wstring& text);

} // namespace ui
