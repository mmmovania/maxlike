// Unit tests for the rollup layout against the R3.1 reference: the Create panel shows
// Object Type (title rows 99-114, frame 107-241) and Name and Color (title 248-263,
// frame 256-299) in a container x 4..178, rows 94..885.
#include "ui/Rollup.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::rollup;

int main() {
    const std::vector<Item> create = {{127, true}, {36, true}};   // Object Type, Name and Color
    const auto p = Layout(create, 0);
    CHECK(p[0].titleTop == 99 && p[0].titleBottom == 114 && p[0].frameTop == 107 && p[0].frameBottom == 241);
    CHECK(p[1].titleTop == 248 && p[1].titleBottom == 263 && p[1].frameTop == 256 && p[1].frameBottom == 299);
    CHECK(MaxScroll(create) == 0);                            // fits: nothing to scroll

    // Collapsing the first moves the second up; reopening restores it.
    std::vector<Item> c = create;
    c[0].open = false;
    const auto q = Layout(c, 0);
    CHECK(q[0].frameBottom == q[0].titleBottom && q[1].titleTop == 114 + kGap);
    c[0].open = true;
    CHECK(Layout(c, 0)[1].titleTop == 248);

    // A tall column scrolls by exactly the overflow, and everything moves with it.
    const std::vector<Item> tall = {{127, true}, {36, true}, {400, true}, {300, true}};
    const int max = MaxScroll(tall);
    CHECK(max > 0 && max == ContentHeight(tall) - (884 - 99 + 1 - kBottomMargin));
    CHECK(Layout(tall, max)[0].titleTop == 99 - max);
    CHECK(Layout(tall, max).back().frameBottom == 884 - kBottomMargin);   // last rollout ends just above the bottom

    // Title hit testing, with scrolling.
    CHECK(TitleAt(create, 0, 50, 105) == 0 && TitleAt(create, 0, 50, 250) == 1);
    CHECK(TitleAt(create, 0, 50, 130) == -1 && TitleAt(create, 0, 5, 105) == -1);
    CHECK(TitleAt(tall, 20, 50, 105 - 20) == 0);
    if (failures == 0) std::printf("rollup: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
