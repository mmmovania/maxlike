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
    // Category row (measured): the pressed Geometry button spans x 9..32, rows 37..59;
    // the other icons start at x 37, 61, 85, 109, 133, 157 (button left + 4).
    CHECK(Is(CategoryBox(kGeometry), 9, 37, 32, 59));
    const int iconLefts[] = {37, 61, 85, 109, 133, 157};
    for (int c = kShapes; c < kCategoryCount; ++c) CHECK(CategoryIconLeft(c, false) == iconLefts[c - 1]);
    CHECK(CategoryAt(20, 45) == kGeometry && CategoryAt(170, 45) == kSystems && CategoryAt(5, 45) == -1);
    CHECK(CategoryAt(100, 62) == -1);            // below the row
    // The dropdown's sunken frame: x 21..163, rows 66..86 (143 x 21).
    CHECK(kDropdown.right - kDropdown.left + 1 == 143 && kDropdown.bottom - kDropdown.top + 1 == 21);
    // Every category lists something; Geometry starts with the reference's entry.
    for (int c = 0; c < kCategoryCount; ++c) CHECK(!Subcategories(c).empty() && !CategoryName(c).empty());
    CHECK(Subcategories(kGeometry).front() == L"Standard Primitives");
    // Object Type buttons (title top 99): Box x 17..85 rows 135..152, Cone x 95..163,
    // Teapot (index 8) rows 219..236.
    CHECK(Is(TypeButtonBox(0, 99), 17, 135, 85, 152) && Is(TypeButtonBox(1, 99), 95, 135, 163, 152));
    CHECK(Is(TypeButtonBox(8, 99), 17, 219, 85, 236) && StandardPrimitiveNames()[8] == L"Teapot");
    if (failures == 0) std::printf("command panel layout: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
