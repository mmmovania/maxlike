// Unit tests for the command panel's tab row against the R3.1 reference (Create tab
// selected): measured outlines and icon positions.
#include "ui/CommandPanelLayout.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::cmdpanel;

static bool Is(const Box& b, int l, int t, int r, int bt) { return b.left == l && b.top == t && b.right == r && b.bottom == bt; }

int main() {
    // Measured: selected Create tab x 0..32 from row 1 (joining the body at rows 28-29);
    // unselected tabs 31..59, 60..88, 89..117, 118..146, 147..175 from row 3 to row 27.
    CHECK(Is(TabBox(kCreate, true), 0, 1, 32, 29));
    CHECK(Is(TabBox(kModify, false), 31, 3, 59, 27));
    CHECK(Is(TabBox(kHierarchy, false), 60, 3, 88, 27));
    CHECK(Is(TabBox(kMotion, false), 89, 3, 117, 27));
    CHECK(Is(TabBox(kDisplay, false), 118, 3, 146, 27));
    CHECK(Is(TabBox(kUtilities, false), 147, 3, 175, 27));
    // Icons: 22 x 20 cells at x = tab left + 3, y = 6 (measured: 5, 35.., 63, 92, 121, 152..).
    CHECK(IconLeft(kCreate) == 5 && IconLeft(kHierarchy) == 63 && IconLeft(kMotion) == 92 && IconLeft(kDisplay) == 121);
    // Hit testing: the selected tab wins where it overlaps its neighbour.
    CHECK(TabAt(32, 10, kCreate) == kCreate);   // Create's right edge, over Modify's left
    CHECK(TabAt(32, 10, kModify) == kModify);
    CHECK(TabAt(70, 10, kCreate) == kHierarchy);
    CHECK(TabAt(70, 1, kCreate) == -1);          // above unselected tabs
    CHECK(TabAt(178, 10, kCreate) == -1);        // right of the last tab
    if (failures == 0) std::printf("command panel layout: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
