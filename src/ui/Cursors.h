#pragma once
#include <windows.h>

// The viewport's cursors for the tools (Phase 12.2): Move is Windows' four-way arrow (as
// R3's); Rotate, Scale and Select are drawn here, 32 x 32 monochrome (NOTE: R3's shapes
// drawn from memory).
namespace ui::cursors {
HCURSOR ForTool(int kind);   // 0 Select, 1 Move, 2 Rotate, 3 Scale
}
