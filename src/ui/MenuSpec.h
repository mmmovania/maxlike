#pragma once
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

    bool IsSeparator() const { return text.empty(); }
    bool IsSubmenu()   const { return !children.empty(); }
};

// The 3ds Max R3.1 menu bar. See docs/spec/03-menus.md for status.
const std::vector<MenuItem>& MainMenuSpec();

// Text without '&' and without anything after a tab, e.g. L"&Open...\tCtrl+O" -> L"Open...".
std::wstring PlainMenuText(const std::wstring& text);

} // namespace ui
