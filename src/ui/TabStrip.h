#pragma once
#include "ui/FrameLayout.h" // ui::Rect
#include <string>
#include <vector>

// Geometry and contents of the R3 tab panel, with no Windows dependency
// (unit-tested in tests/test_tab_strip.cpp).
//
// Measured from the reference screenshot at 100%: 11 tabs of fixed width 89 px,
// starting 4 px from the left, 15 px high, regardless of label length.
namespace ui::tabstrip {

inline constexpr int kLeft  = 4;   // x of the first tab
inline constexpr int kPitch = 89;  // every tab is this wide; no gap between tabs

struct Tab {
    std::wstring label;  // shown text ('&' is literal here, not a mnemonic)
    std::string  id;     // stable name, e.g. "tab.objects"
};

// The 11 tabs of the R3.1 tab panel, left to right.
const std::vector<Tab>& Tabs();

// Rectangle of tab `index` inside a tab panel of `height` px, at `dpi`.
Rect TabRect(int index, int height, unsigned dpi);

// Index of the tab under x (panel coordinates), or -1 if none.
int TabAtX(int x, int count, unsigned dpi);

} // namespace ui::tabstrip
