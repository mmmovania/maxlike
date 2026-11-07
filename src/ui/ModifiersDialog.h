#pragma once
#include <windows.h>
#include <string>
#include <vector>

// The Modify panel's "More..." dialog: R3's list of modifiers not on the button set
// (screenshot r3.1-modifiers-dialog.png; entries after "STL-Check" from memory, NOTE:
// unverified). Returns the chosen modifier's name, or "" if cancelled.
namespace ui {

const std::vector<std::wstring>& ModifierListEntries();   // as listed, headings included
std::wstring ModifierNameFromEntry(const std::wstring& entry);   // "  * Camera Map" -> "Camera Map"; headings -> ""
std::wstring ShowModifiersDialog(HWND parent);

} // namespace ui
