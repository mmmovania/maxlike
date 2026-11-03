// Unit tests for the tab panel geometry. Expected values measured from the
// R3.1 reference screenshot (client x coordinates at 100%).
#include "ui/TabStrip.h"
#include <cstdio>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::tabstrip;

static void LabelsMatchReference() {
    const wchar_t* expected[] = {L"Main Toolbar", L"Objects", L"Shapes", L"Compounds",
                                 L"Lights & Cameras", L"Particles", L"Helpers", L"SpaceWarps",
                                 L"Modifiers", L"Modeling", L"Rendering"};
    CHECK(Tabs().size() == std::size(expected));
    for (size_t i = 0; i < Tabs().size() && i < std::size(expected); ++i)
        CHECK(Tabs()[i].label == expected[i]);
}

static void RectsMatchReference() {
    // First tab: highlight column at x=4, dark edge at x=92 -> [4, 93).
    const auto first = TabRect(0, 15, 96);
    CHECK(first.left == 4 && first.right == 93 && first.top == 0 && first.bottom == 15);
    // Second tab starts right after, no gap.
    CHECK(TabRect(1, 15, 96).left == 93);
    // Last (11th) tab ends at x=983.
    CHECK(TabRect(10, 15, 96).right == 983);
}

static void HitTesting() {
    const int n = static_cast<int>(Tabs().size());
    CHECK(TabAtX(3, n, 96) == -1);   // left margin
    CHECK(TabAtX(4, n, 96) == 0);
    CHECK(TabAtX(92, n, 96) == 0);
    CHECK(TabAtX(93, n, 96) == 1);
    CHECK(TabAtX(982, n, 96) == 10);
    CHECK(TabAtX(983, n, 96) == -1); // empty space right of the tabs
}

static void ScalesWithoutGaps() {
    // At 150% every tab must start exactly where the previous one ended.
    for (int i = 1; i < 11; ++i)
        CHECK(TabRect(i, 23, 144).left == TabRect(i - 1, 23, 144).right);
    CHECK(TabRect(0, 23, 144).left == 6);
}

int main() {
    LabelsMatchReference();
    RectsMatchReference();
    HitTesting();
    ScalesWithoutGaps();
    if (failures == 0) std::printf("tab strip: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
