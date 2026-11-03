#pragma once
#include "ui/CommandTable.h"
#include "ui/MenuSpec.h"
#include <windows.h>
#include <vector>

namespace ui {

// Builds a real Win32 menu bar from a MenuSpec, registering every command in
// `commands` so WM_COMMAND IDs can be mapped back to names.
// The returned HMENU is owned by the window it is attached to.
HMENU BuildMenuBar(const std::vector<MenuItem>& spec, CommandTable& commands);

} // namespace ui
